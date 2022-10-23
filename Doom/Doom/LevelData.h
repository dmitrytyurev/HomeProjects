#include <vector>
#include <string>

struct FPoint2D
{
	float x;
	float z;
};

struct DPoint2D
{
	double x;
	double z;
};

struct Edge
{
	int firstInd = 0;  // Индекс первой вершины ребра в verts. Индекс второй вершины ребра берём из следующего ребра в edges
	int adjPolyN = -1;  // Индекс смежного полигона в polies, либо -1, если смежного полигона нет
	int adjEdgeN = 0;  // Индекс смежного ребра в edges смежного полигона
	// Текстурные координаты стен
	float u[2] = {};    // U-координаты в первой и второй точках ребра
	float vCeil = 0;     // V-координата а уровне потолка
	float vCeilAdd = 1;  // Приращение V-координаты (в единицах [0..1] на 1 метр сдвига вниз)
	float vFloor = 0;    // V-координата а уровне пола
	float vFloorAdd = 1; // Приращение V-координаты (в единицах [0..1] на 1 метр сдвига вниз)
	// Текстурные координаты пола/потолка
	float uFloorCeil = 0;
	float vFloorCeil = 0;
};

struct Poly
{
	float yRoof = 0;
	float yCeil = 0;
	float yFloor = 0;
	std::vector<Edge> edges;
};

struct Texture
{
	Texture(const std::string& fileName);
	~Texture() { if (buf) delete[]buf; }

	int sizeX = 0;   // Размер текстуры по X (разрешается только степени двойки)
	int sizeY = 0;   // Размер текстуры по Y (разрешается только степени двойки)
	int xPow2 = 0;   // Сама степень двойки для sizeX
	unsigned char* buf = nullptr;  // Буфер пикселей по 4 байта на пиксел
};

void FillLevelData(std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures);
