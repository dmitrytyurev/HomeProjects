
#include "LevelData.h"
#include "bmp.h"
#include "Utils.h"
#include "Level1Room.h"
#include "Level2Corridor.h"

const double TEX_DENS = 2.048;  // Рекомендуемая плотностью текселей на единицу размера глобальной системы координат 


bool IsPow2(int value, int* pow)
{
	int mask = 1;
	for (int i = 0; i < 20; ++i) {
		if (value == mask) {
			if (pow)
				*pow = i;
			return true;
		}
		mask = mask << 1;
	}
	return false;
}

int getTextureIndex(const std::string& name, std::vector<Texture>& textures)
{
	if (name.empty()) {
		return 0;
	}
	for (int i = 0; i < textures.size(); ++i) {
		if (textures[i].name == name)
			return i;
	}
	ExitMsg("Texture not loaded:  %s\n", name.c_str());
	return 0;  // Сюда управление не приходит
}

Texture::Texture(const std::string& name_)
{
	name = name_;
	std::string fileName = "../Textures/" + name + ".bmp";

	int bitDepth = 0;
	give_bmp_size(fileName.c_str(), &sizeX, &sizeY, &bitDepth);
	if (bitDepth != 24 || !IsPow2(sizeX, &xPow2) || !IsPow2(sizeY, nullptr)) {
		ExitMsg("Texture file %s params is wrong %d %d %d", fileName.c_str(), sizeX, sizeY, bitDepth);
	}

	unsigned char* tmpBuf = new unsigned char[sizeX * sizeY * 3];
	read_bmp24(fileName.c_str(), tmpBuf);

	unsigned char* src = tmpBuf;
	buf = new unsigned char[sizeX * sizeY * 4];
	unsigned char* dst = buf;

	for (int i = 0; i < sizeX * sizeY; ++i) {
		unsigned char b = *src++;
		unsigned char g = *src++;
		unsigned char r = *src++;
		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
		*dst++ = 0;
	}

	delete[]tmpBuf;
}


void project_uv_ceil(float scale, std::vector<int> pols, std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	float xMin = 1000000;
	float xMax = -1000000;
	float zMin = 1000000;
	float zMax = -1000000;
	for (int pn =0; pn <(int)pols.size(); ++pn) {
		Poly& poly = polies[pols[pn]];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {
			const FPoint2D& point = verts[poly.edges[en].firstInd];
			if (point.x < xMin) xMin = point.x;
			if (point.x > xMax) xMax = point.x;
			if (point.z < zMin) zMin = point.z;
			if (point.z > zMax) zMax = point.z;
		}
	}
	double texelsNumX = (xMax - xMin) * TEX_DENS;
	double texelsNumZ = (zMax - zMin) * TEX_DENS;
	int repeatsNumX = int(texelsNumX / textures[polies[pols[0]].ceilTex.texIndex].sizeX + 0.5);
	int repeatsNumZ = int(texelsNumZ / textures[polies[pols[0]].ceilTex.texIndex].sizeY + 0.5);
	for (int pn = 0; pn < (int)pols.size(); ++pn) {
		Poly& poly = polies[pols[pn]];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {
			const FPoint2D& point = verts[poly.edges[en].firstInd];
			poly.edges[en].uCeil = (double(point.x) - xMin) / (double(xMax) - xMin) * repeatsNumX / scale;
			poly.edges[en].vCeil = (double(point.z) - zMin) / (double(zMax) - zMin) * repeatsNumZ / scale;
		}
	}
}

void project_uv(bool isFloor, float repeatsNumX, float repeatsNumZ, std::vector<int> pols, std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	float xMin = 1000000;
	float xMax = -1000000;
	float zMin = 1000000;
	float zMax = -1000000;
	for (int pn = 0; pn < (int)pols.size(); ++pn) {
		Poly& poly = polies[pols[pn]];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {
			const FPoint2D& point = verts[poly.edges[en].firstInd];
			if (point.x < xMin) xMin = point.x;
			if (point.x > xMax) xMax = point.x;
			if (point.z < zMin) zMin = point.z;
			if (point.z > zMax) zMax = point.z;
		}
	}
	for (int pn = 0; pn < (int)pols.size(); ++pn) {
		Poly& poly = polies[pols[pn]];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {
			const FPoint2D& point = verts[poly.edges[en].firstInd];
			float u = (double(point.z) - zMin) / (double(zMax) - zMin) * repeatsNumX;
			float v = (double(point.x) - xMin) / (double(xMax) - xMin) * repeatsNumZ;
			if (isFloor) {
				poly.edges[en].uFloor = u;
				poly.edges[en].vFloor = v;
			}
			else {
				poly.edges[en].uCeil = u;
				poly.edges[en].vCeil = v;
			}
		}
	}
}

void project_u_wall_x(float x1, float u1, float x2, float u2, std::vector<int> polsEdges, std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	int polsNum = polsEdges.size() / 2;
	for (int pn = 0; pn < polsNum; ++pn) {
		// Заполнение у рёбер u[0] и u[1]
		Poly& poly = polies[polsEdges[pn * 2]];
		int edge = polsEdges[pn * 2 + 1];
		double k = (double(u2) - u1) / (double(x2) - x1);
		float x = verts[poly.edges[edge].firstInd].x;
		poly.edges[edge].u[0] = (double(x) - x1) * k + u1;
		x = verts[poly.edges[(edge + 1) % poly.edges.size()].firstInd].x;
		poly.edges[edge].u[1] = (double(x) - x1) * k + u1;
	}
}

void project_u_wall_z(float z1, float u1, float z2, float u2, std::vector<int> polsEdges, std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	int polsNum = polsEdges.size() / 2;
	for (int pn = 0; pn < polsNum; ++pn) {
		// Заполнение у рёбер u[0] и u[1]
		Poly& poly = polies[polsEdges[pn * 2]];
		int edge = polsEdges[pn * 2 + 1];
		double k = (double(u2) - u1) / (double(z2) - z1);
		float z = verts[poly.edges[edge].firstInd].z;
		poly.edges[edge].u[0] = (double(z) - z1) * k + u1;
		z = verts[poly.edges[(edge+1)%poly.edges.size()].firstInd].z;
		poly.edges[edge].u[1] = (double(z) - z1) * k + u1;
	}
}

void project_v_wall(bool feelCeilV, bool feelFloorV, float y1, float v1, float y2, float v2, std::vector<int> polsEdges, std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	int polsNum = polsEdges.size() / 2;
	for (int pn = 0; pn < polsNum; ++pn) {
		Poly& poly = polies[polsEdges[pn * 2]];
		int edge = polsEdges[pn * 2 + 1];

		// Заполнение у рёбер vCeil и vWallUpAdd
		if (feelCeilV) {
			float y = poly.yCeil;
			double k = (double(v2) - v1) / (double(y2) - y1);
			poly.edges[edge].vWallUp = (y - y1) * k + v1;

			int adjPolyN = poly.edges[edge].adjPolyN;
			if (adjPolyN == -1) 
				ExitMsg("adjPolyN == -1");
			Poly& adjPoly = polies[adjPolyN];
			y = adjPoly.yCeil;
			poly.edges[edge].vWallUpAdd = (y - y1) * k + v1;
			PrintConsole("");
		}

		// Заполнение у рёбер vFloor и vWallDownAdd
		if (feelFloorV) {
			float y = poly.yFloor;
			double k = (double(v2) - v1) / (double(y2) - y1);
			poly.edges[edge].vWallDown = (y - y1) * k + v1;

			int adjPolyN = poly.edges[edge].adjPolyN;
			if (adjPolyN == -1)
				ExitMsg("adjPolyN == -1");
			Poly& adjPoly = polies[adjPolyN];
			y = adjPoly.yFloor;
			poly.edges[edge].vWallDownAdd = (y - y1) * k + v1;
			PrintConsole("");
		}
	}
}

//----------------------------------------------------------------------------------------------------

void FillLevelData(std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	FillLevel1Room(verts, polies, textures);
	FillLevel2Corridor(verts, polies, textures);

	// Заполнение незаполненных poly.yCeil и poly.yRoof

	for (int i = 0; i < polies.size(); ++i) {
		Poly& poly = polies[i];
		if (poly.yCeil < poly.yFloor)
			poly.yCeil = poly.yFloor + 10;
		if (poly.yRoof < poly.yCeil)
			poly.yRoof = poly.yCeil + 10;
	}

	// Заполнение инфы о смежности
	for (int pn1 = 0; pn1 < polies.size(); ++pn1) {
		Poly& poly1 = polies[pn1];
		int vertsNum1 = (int)poly1.edges.size();
		for (int en1 = 0; en1 < vertsNum1; ++en1) {
			poly1.edges[en1].adjPolyN = -1;
		}
	}

	for (int pn1 = 0; pn1 < polies.size(); ++pn1) {
		Poly& poly1 = polies[pn1];
		int vertsNum1 = (int)poly1.edges.size();
		for (int en1 = 0; en1 < vertsNum1; ++en1) {
			int vertInd1 = poly1.edges[en1].firstInd;
			int vertInd2 = poly1.edges[(en1+1)% vertsNum1].firstInd;
			for (int pn2 = pn1+1; pn2 < polies.size(); ++pn2) {
				Poly& poly2 = polies[pn2];
				int vertsNum2 = (int)poly2.edges.size();
				for (int en2 = 0; en2 < vertsNum2; ++en2) {
					if (poly2.edges[en2].firstInd == vertInd2 && poly2.edges[(en2 + 1) % vertsNum2].firstInd == vertInd1) {
						poly1.edges[en1].adjPolyN = pn2;
						poly1.edges[en1].adjEdgeN = en2;
						poly2.edges[en2].adjPolyN = pn1;
						poly2.edges[en2].adjEdgeN = en1;
						goto m1;
					}
				}
			}
m1:;
		}
	}

	// Разворачиваем систему координат, чтобы был обход по часовой стрелке
	for (FPoint2D& vert: verts) {
		vert.z = -vert.z;
	}

	// Загрузка текстур
	textures.emplace_back("checker");
	textures.emplace_back("gray_wall");
	textures.emplace_back("hexa_floor");
	textures.emplace_back("ceil_lights");
	textures.emplace_back("stone_wall");
	textures.emplace_back("blue_hexa_floor");
	textures.emplace_back("panel");
	textures.emplace_back("panel_light");
	textures.emplace_back("cyber_wall_blue");
	textures.emplace_back("metal_column");
	textures.emplace_back("steps_light");
	textures.emplace_back("monitors");
	textures.emplace_back("big_wall");
	textures.emplace_back("metal_plates2");
	textures.emplace_back("panel2");

	
	
	// Прописываем индексы текстур в полигоны и рёбра
	for (int pn1 = 0; pn1 < polies.size(); ++pn1) {
		Poly& poly1 = polies[pn1];
		poly1.ceilTex.texIndex = getTextureIndex(poly1.ceilTex.texFileName, textures);
		poly1.floorTex.texIndex = getTextureIndex(poly1.floorTex.texFileName, textures);
		int vertsNum1 = (int)poly1.edges.size();
		for (int en1 = 0; en1 < vertsNum1; ++en1) {
			Edge& edge = poly1.edges[en1];
	
			edge.texUp.texIndex = getTextureIndex(edge.texUp.texFileName, textures);
			edge.texDown.texIndex = getTextureIndex(edge.texDown.texFileName, textures);
		}
	}

	// --------------- Заполнение текстурных координат ------------------
	// На данный момент часть текстурных координат простенков, пола, потолка уже заполны в методах FillLevel*

	for (int pn = 0; pn < polies.size(); ++pn) {
		Poly& poly = polies[pn];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {
			// Если текстура верхнего и нижнего простенка не заданы (там будет отображаться чекер), то
			// заполнить U и V координаты простенков для отображения чеккера
			if (poly.edges[en].texUp.texFileName.empty() && poly.edges[en].texDown.texFileName.empty()) {
				poly.edges[en].u[0] = 0;

				int nextInd = (en + 1) % vertsNum;
				double dx = verts[poly.edges[en].firstInd].x - verts[poly.edges[nextInd].firstInd].x;
				double dz = verts[poly.edges[en].firstInd].z - verts[poly.edges[nextInd].firstInd].z;
				double dist = sqrt(dx * dx + dz * dz);
				poly.edges[en].u[1] = float(dist * 0.01);

				poly.edges[en].vWallUp = 1;
				poly.edges[en].vWallUpAdd = 0.01f;
				poly.edges[en].vWallDown = 0;
				poly.edges[en].vWallDownAdd = 0.01f;
			}

			// Заполняем незаполненые ранее текстурные координаты ПОТОЛКА с универсальной плотностью текселей/метр (должно хватить для большинства случаев)
			if (poly.edges[en].uCeil == -1.f)
				poly.edges[en].uCeil = float(verts[poly.edges[en].firstInd].x * TEX_DENS / textures[poly.ceilTex.texIndex].sizeX) * 0.5;
			if (poly.edges[en].vCeil == -1.f)
				poly.edges[en].vCeil = float(verts[poly.edges[en].firstInd].z * TEX_DENS / textures[poly.ceilTex.texIndex].sizeX) * 0.5;

			// Заполняем незаполненые ранее текстурные координаты ПОЛА с универсальной плотностью текселей/метр (должно хватить для большинства случаев)
			if (poly.edges[en].uFloor == -1.f)
					poly.edges[en].uFloor = float(verts[poly.edges[en].firstInd].x * TEX_DENS / textures[poly.floorTex.texIndex].sizeX);
			if (poly.edges[en].vFloor == -1.f)
				poly.edges[en].vFloor = float(verts[poly.edges[en].firstInd].z * TEX_DENS / textures[poly.floorTex.texIndex].sizeX);
		}
	}

	// Делаем мэппинг заданных групп плокостей по единым правилам

	project_uv_ceil(1, {37,38,39,40}, verts, polies, textures);
	project_uv(true, 2, 1, { 26,27}, verts, polies, textures);
	project_uv(true, 2, 1, { 18,56 }, verts, polies, textures);
	project_uv(false, 5, 1, { 26,27 }, verts, polies, textures);
	project_uv(false, 5, 1, { 18,56 }, verts, polies, textures);

	project_u_wall_z(1632, 0, 1916, 2, { 34, 2, 33, 1, 32, 2, 31, 3 }, verts, polies, textures);
	project_v_wall(false, true, 1160, 0, 1010, 1, { 34, 2, 33, 1, 32, 2, 31, 3 }, verts, polies, textures);

	project_u_wall_z(1632, 0, 1916, 2, { 47, 0, 48, 1, 49, 0, 50, 1 }, verts, polies, textures);
	project_v_wall(false, true, 1160, 0, 1010, 1, { 47, 0, 48, 1, 49, 0, 50, 1 }, verts, polies, textures);

	project_u_wall_x(468, 0, 628, 1, { 3, 2, 1, 2, 4, 2 }, verts, polies, textures);
	project_v_wall(true, true, 1120, 0, 1000, 1, { 3, 2, 1, 2, 4, 2 }, verts, polies, textures);

	// Если текстура верхнего или нижнего простенка заданы (значит выше для них не были заполнены текстурные координаты под чеккер с параметромами *Add уже в виде приращения), то
	// параметры *Add содержат текстурную координату, которую сейчас пересчитаем в приращение

	for (int pn = 0; pn < polies.size(); ++pn) {
		Poly& poly = polies[pn];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {
			if (!poly.edges[en].texUp.texFileName.empty() || !poly.edges[en].texDown.texFileName.empty()) {
				if (poly.edges[en].adjPolyN >= 0) {
					float dy = poly.yFloor - polies[poly.edges[en].adjPolyN].yFloor;
					if (dy > 0) {
						float repeatsN = poly.edges[en].vWallDownAdd - poly.edges[en].vWallDown;
						poly.edges[en].vWallDownAdd = repeatsN / dy;
					}

					dy = polies[poly.edges[en].adjPolyN].yCeil - poly.yCeil;
					if (dy > 0) {
						float repeatsN = poly.edges[en].vWallUpAdd - poly.edges[en].vWallUp;
						poly.edges[en].vWallUpAdd = -repeatsN / dy;
					}

				}
			}
		}
	}


}
