
#include "LevelData.h"
#include "bmp.h"
#include "Utils.h"

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

		// Заполнение у рёбер vCeil и vCeilAdd
		if (feelCeilV) {
			float y = poly.yCeil;
			double k = (double(v2) - v1) / (double(y2) - y1);
			poly.edges[edge].vWallCeil = (y - y1) * k + v1;

			int adjPolyN = poly.edges[edge].adjPolyN;
			if (adjPolyN == -1) 
				ExitMsg("adjPolyN == -1");
			Poly& adjPoly = polies[adjPolyN];
			y = adjPoly.yCeil;
			poly.edges[edge].vCeilAdd = (y - y1) * k + v1;
			PrintConsole("");
		}

		// Заполнение у рёбер vFloor и vFloorAdd
		if (feelFloorV) {
			float y = poly.yFloor;
			double k = (double(v2) - v1) / (double(y2) - y1);
			poly.edges[edge].vWallFloor = (y - y1) * k + v1;

			int adjPolyN = poly.edges[edge].adjPolyN;
			if (adjPolyN == -1)
				ExitMsg("adjPolyN == -1");
			Poly& adjPoly = polies[adjPolyN];
			y = adjPoly.yFloor;
			poly.edges[edge].vFloorAdd = (y - y1) * k + v1;
			PrintConsole("");
		}
	}
}



void FillLevelData(std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	verts.resize(84);

	verts[0] = {464,840};
	verts[1] = { 468,840 };
	verts[2] = { 628,840 };
	verts[3] = { 634,840 };
	verts[4] = { 106,988 };
	verts[5] = { 140,1012 };
	verts[6] = { 190,1012 };
	verts[7] = { 464,988 };
	verts[8] = { 468,1012 };
	verts[9] = { 628,1012 };

	verts[10] = { 634,988 };
	verts[11] = { 910,1012 };
	verts[12] = { 960,1012 };
	verts[13] = { 990,988 };
	verts[14] = { 310,1192 };
	verts[15] = { 390,1192 };
	verts[16] = { 718,1192 };
	verts[17] = { 796,1192 };
	verts[18] = { 310,1398 };
	verts[19] = { 390,1398 };

	verts[20] = { 718,1398 };
	verts[21] = { 796,1398 };
	verts[22] = { 106,1586 };
	verts[23] = { 140,1586 };
	verts[24] = { 190,1586 };
	verts[25] = { 342,1586 };
	verts[26] = { 388,1586 };
	verts[27] = { 718,1586 };
	verts[28] = { 764,1586 };
	verts[29] = { 910,1586 };

	verts[30] = { 960,1586 };
	verts[31] = { 990,1586 };
	verts[32] = { 342,1632 };
	verts[33] = { 364,1632 };
	verts[34] = { 388,1632 };
	verts[35] = { 718,1632 };
	verts[36] = { 742,1632 };
	verts[37] = { 764,1632 };
	verts[38] = { 388,1670 };
	verts[39] = { 718,1670 };

	verts[40] = { 388,1709 };
	verts[41] = { 718,1709 };
	verts[42] = { 388,1746 };
	verts[43] = { 718,1746 };
	verts[44] = { 742,1746 };
	verts[45] = { 106,1943 };
	verts[46] = { 140,1916 };
	verts[47] = { 190,1916 };
	verts[48] = { 364,1916 };
	verts[49] = { 370,1943 };

	verts[50] = { 388,1916 };
	verts[51] = { 718,1916 };
	verts[52] = { 730,1943 };
	verts[53] = { 742,1916 };
	verts[54] = { 910,1916 };
	verts[55] = { 960,1916 };
	verts[56] = { 990,1943 };
	verts[57] = { 364,1746 };

	verts[58] = { 628, 818 };
	verts[59] = { 693, 818};

	verts[60] = { 852, 818 };
	verts[61] = { 901, 818 };
	verts[62] = { 852, 840};
	verts[63] = { 1074, 818 };
	verts[64] = { 1074, 658 };
	verts[65] = { 901, 658 };
	verts[66] = { 852, 658 };
	verts[67] = { 693, 658};
	verts[68] = { 628, 658 };
	verts[69] = { 468, 658 };

	verts[70] = { 453, 658 };
	verts[71] = { 453, 818 };
	verts[72] = { 468, 818 };
	verts[73] = { 693, 609};
	verts[74] = { 852, 609 };
	verts[75] = { 693, 451};
	verts[76] = { 852, 451 };
	verts[77] = { 870, 451 };
	verts[78] = { 852, 359 };
	verts[79] = { 750, 340};

	verts[80] = { 292 , 451 };
	verts[81] = { 270, 550};
	verts[82] = { 292, 609 };
	verts[83] = { 693, 359};


	polies.resize(84);

	polies[0] = { 0,0,1252,
		{{0,{0,0}, {""}, 0, 0, {""}, 0, 0},
		 {1,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		 {8,{0,0}, {""}, 0, 0, {""}, 0, 0},
		 {7,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[1] = { 1252,1120,1000,
		{{1,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{2,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{9,{0,1}, {"gray_wall"}, 0, 1.25, {""}, 0, 0},
		{8,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[1].edges[2].wallBrightsUp[0] = 0.4f;
	polies[1].edges[2].wallBrightsUp[1] = 0.4f;

	polies[2] = { 0,0,1252,
		{{2,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{3,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{10,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{9,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2}} };

	polies[3] = { 0,0,1252,
		{{4,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{7,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{8,{2,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{6,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[3].edges[2].wallBrightsDown[0] = 0.4f;
	polies[3].edges[2].wallBrightsDown[1] = 0.7f;

	polies[4] = { 0,0,1252,
		{{10,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{13,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{11,{2,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{9,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[4].edges[2].wallBrightsDown[0] = 0.7f;
	polies[4].edges[2].wallBrightsDown[1] = 0.4f;

	polies[5] = { 0,1250,1092,
		{{13,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{12,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1},
		{11,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[5].edges[1].wallBrightsDown[0] = 0.7f;
	polies[5].edges[1].wallBrightsDown[1] = 0.4f;

	polies[6] = { 0,1250,1092,
		{{12,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{13,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{31,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{30,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1}} };

	polies[7] = { 0,1250,1092,
		{{30,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{31,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{56,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{55,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1}} };

	polies[8] = { 0,1250,1092,
		{{56,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1},
		{55,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[8].edges[1].wallBrightsDown[0] = 0.4f;
	polies[8].edges[1].wallBrightsDown[1] = 0.7f;

	polies[9] = { 0,0,1252,
		{{53,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{56,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{52,{0,0}, {""}, 0, 0, {""}, 0, }} };
	polies[9].edges[0].wallBrightsDown[0] = 0.2f;
	polies[9].edges[0].wallBrightsDown[1] = 0.5f;

	polies[10] = { 0,0,1252,
		{{53,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{52,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{51,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[11] = { 0,0,1162,
		{{51,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{52,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{49,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{50,{2,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 1}} };

	polies[12] = { 0,0,1252,
		{{50,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{49,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{48,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[13] = { 0,0,1252,
		{{48,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{49,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{45,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{47,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2}} };
	polies[13].edges[3].wallBrightsDown[0] = 0.5f;
	polies[13].edges[3].wallBrightsDown[1] = 0.2f;

	polies[14] = { 0,1250,1092,
		{{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{45,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{46,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1}} };
	polies[14].edges[2].wallBrightsDown[0] = 0.7f;
	polies[14].edges[2].wallBrightsDown[1] = 0.4f;

	polies[15] = { 0,1250,1092,
		{{22,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{23,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1},
		{46,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{45,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[16] = { 0,1250,1092,
		{{4,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{5,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1},
		{23,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{22,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[17] = { 0,1250,1092,
	{{4,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{6,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1},
		{5,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[17].edges[1].wallBrightsDown[0] = 0.4f;
	polies[17].edges[1].wallBrightsDown[1] = 0.7f;

	polies[18] = { 1252,1090,1035,
		{{5,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{6,{0,2}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1},
		{24,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{23,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"panel_light"}, {"panel"} };

	polies[19] = { 0,1250,1000,
		{{6,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{8,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{15,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{14,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[19].edges[1].brightFloor = 100;
	polies[19].edges[2].brightFloor = 130;

	polies[20] = { 0,1250,1000,
		{{8,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{9,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{16,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{20,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{19,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{15,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[20].edges[0].brightFloor = 100;
	polies[20].edges[1].brightFloor = 100;
	polies[20].edges[2].brightFloor = 130;
	polies[20].edges[3].brightFloor = 130;
	polies[20].edges[4].brightFloor = 130;
	polies[20].edges[5].brightFloor = 130;

	polies[21] = { 0,1250,1000,
		{{9,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{11,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{17,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{16,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[21].edges[0].brightFloor = 100;
	polies[21].edges[3].brightFloor = 130;

	polies[22] = { 0,1250,1000,
		{{11,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{21,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{17,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[23] = { 1252,1120,1000,
		{{16,{0.5f,0.25f}, {"monitors"}, 0, 1, {""}, 0, 0, 0, 0},
		{17,{0,1}, {"monitors"}, 0, 1, {""}, 0, 0, 1, 0},
		{21,{0.25f,0.5f}, {"monitors"}, 0, 1, {""}, 0, 0, 1 ,3},
		{20,{1,0}, {"monitors"}, 0, 1, {""}, 0, 0, 0, 3}}, {"ceil_lights"}, {"hexa_floor"} };
	polies[23].edges[0].brightFloor = 130;
	polies[23].edges[3].brightFloor = 130;

	polies[24] = { 0,1250,1000,
		{{21,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{28,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[25] = { 0,1250,1000,
		{{20,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{21,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{28,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[25].edges[0].brightFloor = 130;

	polies[26] = { 1252,1090,1035,
		{{11,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{12,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{30,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,2}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1}}, {"panel_light"}, {"panel"} };

	polies[27] = { 1252,1090,1035,
		{{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{30,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{55,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{1,0}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1}}, {"panel_light"}, {"panel"} };

	polies[28] = { 0,1250,1000,
		{{28,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{37,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[28].edges[2].brightFloor = 60;
	polies[28].edges[3].brightFloor = 150;

	polies[29] = { 0,1250,1000,
		{{37,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{36,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[29].edges[0].brightFloor = 150;
	polies[29].edges[1].brightFloor = 60;
	polies[29].edges[2].brightFloor = 60;
	polies[29].edges[3].brightFloor = 150;

	polies[30] = { 0,1250,1000,
		{{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{53,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[30].edges[0].brightFloor = 60;
	polies[30].edges[1].brightFloor = 60;
	polies[30].edges[2].brightFloor = 60;
	polies[30].edges[2].brightCeil = 60;

	polies[31] = { 0,0,1252,
		{{43,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{44,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{53,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{51,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0}} };
	polies[31].edges[1].wallBrightsDown[0] = 0.4f;
	polies[31].edges[1].wallBrightsDown[1] = 0.2f;

	polies[32] = { 0,0,1252,
		{{41,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{43,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0}} };

	polies[33] = { 0,0,1252,
		{{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{41,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{39,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[34] = { 0,0,1252,
		{{36,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{39,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{35,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[34].edges[0].wallBrightsDown[0] = 0.7f;
	polies[34].edges[0].wallBrightsDown[1] = 0.4f;

	polies[35] = { 0,0,1252,
		{{27,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{36,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{35,{1,0}, {""}, 0, 0, {"metal_column"}, 0, 2}} };
	polies[35].edges[2].wallBrightsDown[0] = 0.6f;
	polies[35].edges[2].wallBrightsDown[1] = 0.6f;

	polies[36] = { 0,0,1252,
		{{28,{0.234f,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{37,{0.063f,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{36,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0.234f}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2}} };
	polies[36].edges[3].wallBrightsDown[0] = 0.6f;
	polies[36].edges[3].wallBrightsDown[1] = 0.6f;

	polies[37] = { 1252,1160,1040,
		{{42,{2,0}, {""}, 0, 0, {"steps_light"}, 0, 1},
		{43,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{51,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{50,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };

	polies[38] = { 1252,1160,1030,
		{{40,{2,0}, {""}, 0, 0, {"steps_light"}, 0, 1},
		{41,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{43,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{42,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };

	polies[39] = { 1252,1160,1020,
		{{38,{2,0}, {""}, 0, 0, {"steps_light"}, 0, 1},
		{39,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{41,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{40,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };

	polies[40] = { 1252,1160,1010,
		{{34,{2,0}, {"cyber_wall_blue"}, 0, 1, {"steps_light"}, 0, 1},
		{35,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{39,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{38,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };
	polies[40].edges[0].wallBrightsUp[0] = 0.6f;
	polies[40].edges[0].wallBrightsUp[1] = 0.6f;

	polies[41] = { 0,1250,1000,
		{{26,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{35,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{34,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[42] = { 0,1250,1000,
		{{19,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{20,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{26,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[42].edges[0].brightFloor = 130;
	polies[42].edges[1].brightFloor = 130;

	polies[43] = { 1252,1120,1000,
		{{14,{0.5f,0.75f}, {"monitors"}, 0, 1, {""}, 0, 0 , 0, 0},
		{15,{1,0}, {"monitors"}, 0, 1, {""}, 0, 0, 1, 0},
		{19,{0.25f,0.5f}, {"monitors"}, 0, 1, {""}, 0, 0, 1, 3},
		{18,{1,0}, {"monitors"}, 0, 1, {""}, 0, 0, 0, 3}}, {"ceil_lights"}, {"hexa_floor"} };
	polies[43].edges[1].brightFloor = 130;
	polies[43].edges[2].brightFloor = 130;

	polies[44] = { 0,1250,1000,
		{{18,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{19,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{26,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{25,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[44].edges[1].brightFloor = 130;

	polies[45] = { 0,0,1252,
		{{25,{0.234f,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{26,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{33,{0,0.063f}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{32,{0,0.234f}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2}} };
	polies[45].edges[0].wallBrightsDown[0] = 0.6f;
	polies[45].edges[0].wallBrightsDown[1] = 0.6f;

	polies[46] = { 0,0,1252,
		{{26,{0,1}, {""}, 0, 0, {"metal_column"}, 0, 2},
		{34,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{33,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[46].edges[0].wallBrightsDown[0] = 0.6f;
	polies[46].edges[0].wallBrightsDown[1] = 0.6f;

	polies[47] = { 0,0,1252,
		{{34,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{38,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{33,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[47].edges[2].wallBrightsDown[0] = 0.4f;
	polies[47].edges[2].wallBrightsDown[1] = 0.7f;

	polies[48] = { 0,0,1252,
		{{57,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{38,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{40,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[49] = { 0,0,1252,
		{{40,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{42,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[50] = { 0,0,1252,
		{{57,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{42,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{50,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{48,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2}} };
	polies[50].edges[3].wallBrightsDown[0] = 0.2f;
	polies[50].edges[3].wallBrightsDown[1] = 0.4f;

	polies[51] = { 0,1250,1000,
		{{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{48,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[51].edges[0].brightFloor = 60;
	polies[51].edges[1].brightFloor = 60;
	polies[51].edges[2].brightFloor = 60;
	polies[51].edges[2].brightCeil = 60;

	polies[52] = { 0,1250,1000,
		{{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{32,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{33,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[52].edges[0].brightFloor = 60;
	polies[52].edges[1].brightFloor = 150;
	polies[52].edges[2].brightFloor = 150;
	polies[52].edges[3].brightFloor = 60;

	polies[53] = { 0,1250,1000,
		{{24,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{25,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{32,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{47,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[53].edges[2].brightFloor = 150;
	polies[53].edges[3].brightFloor = 60;

	polies[54] = { 0,1250,1000,
		{{18,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{25,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{24,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[55] = { 0,1250,1000,
		{{6,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{14,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{18,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{24,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[56] = { 1252,1090,1035,
		{{23,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{24,{1,0}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1},
		{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{46,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"panel_light"}, {"panel"} };


	// ========= Коридор ========

	//polies[0] = { 0,0,1000,
	//	{{0,{0,0}, {""}, 0, 0, {""}, 0, 0},
	//	 {1,{1,0}, {""}, 0, 0, {""}, 0, 0},
	//	 {8,{0,0}, {""}, 0, 0, {""}, 0, 0},
	//	 {7,{0,0}, {""}, 0, 0, {""}, 0, 0}} };


	// Заполнение  poly.yCeil и poly.yRoof

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


	// Заполнение текстурных координат 

	for (int pn = 0; pn < polies.size(); ++pn) {
		Poly& poly = polies[pn];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {

			if (poly.edges[en].texUp.texFileName.empty() && poly.edges[en].texDown.texFileName.empty()) {
				poly.edges[en].u[0] = 0;

				int nextInd = (en + 1) % vertsNum;
				double dx = verts[poly.edges[en].firstInd].x - verts[poly.edges[nextInd].firstInd].x;
				double dz = verts[poly.edges[en].firstInd].z - verts[poly.edges[nextInd].firstInd].z;
				double dist = sqrt(dx * dx + dz * dz);
				poly.edges[en].u[1] = float(dist * 0.01);

				poly.edges[en].vWallCeil = 1;
				poly.edges[en].vCeilAdd = 0.01f;
				poly.edges[en].vWallFloor = 0;
				poly.edges[en].vFloorAdd = 0.01f;
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


	for (int pn = 0; pn < polies.size(); ++pn) {
		Poly& poly = polies[pn];
		int vertsNum = (int)poly.edges.size();
		for (int en = 0; en < vertsNum; ++en) {
			if (!poly.edges[en].texUp.texFileName.empty() || !poly.edges[en].texDown.texFileName.empty()) {
				if (poly.edges[en].adjPolyN >= 0) {
					float dy = poly.yFloor - polies[poly.edges[en].adjPolyN].yFloor;
					if (dy > 0) {
						float repeatsN = poly.edges[en].vFloorAdd - poly.edges[en].vWallFloor;
						poly.edges[en].vFloorAdd = repeatsN / dy;
					}

					dy = polies[poly.edges[en].adjPolyN].yCeil - poly.yCeil;
					if (dy > 0) {
						float repeatsN = poly.edges[en].vCeilAdd - poly.edges[en].vWallCeil;
						poly.edges[en].vCeilAdd = -repeatsN / dy;
					}

				}
			}
		}
	}


}
