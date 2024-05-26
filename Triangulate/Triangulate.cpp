#include <iostream>
#include <vector>

#include "Utils.h"

// !!! Важно! Начальные точки не должны быть совсем рандомными, а должны быть примерно в правильных местах
// Если точки перехлестнутся (возникнут самопересечения), то итеративный алгоритм может остановиться и не оптимизироваться дальше

struct Point
{
	double x = 0.;
	double y = 0.;
};

struct Edge
{
	int ind[2] = {};
	double targetLen = 0.;
};

Point offs[9] = { {0,0}, {-1,-1}, {0,-1}, {1,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, {1,1}};

std::vector<Point> points;
std::vector<Edge> edges;

// Индексы точек для поворота вертикально
int indexUp = 0;
int indexDown = 0;

int indexShift = 0;
double xShift = 0;
double yShift = 0;


const double step = 0.01;

// =====================================================================================================================


double calcMetric()
{
	double metric = 0;

	for (const Edge& edge: edges) {
		Point& point1 = points[edge.ind[0]];
		Point& point2 = points[edge.ind[1]];
		double dx = point1.x - point2.x;
		double dy = point1.y - point2.y;
		double currentLen = sqrt(dx * dx + dy * dy);
		double diff = edge.targetLen - currentLen;
		metric += diff * diff;
	}

	return metric;
}

double calcMaxDiff()
{
	double maxDiff = 0;

	for (const Edge& edge : edges) {
		Point& point1 = points[edge.ind[0]];
		Point& point2 = points[edge.ind[1]];
		double dx = point1.x - point2.x;
		double dy = point1.y - point2.y;
		double currentLen = sqrt(dx * dx + dy * dy);
		double diff = fabs(edge.targetLen - currentLen);

		if (diff > maxDiff) {
			maxDiff = diff;
		}

	}

	return maxDiff;
}


void optimize()
{
	while(true) {
		bool isMoved = false;   // Была ли сдвинута хоть одна точка за текущую итерацию оптимизации

		for (Point& point: points) {
			double bestMetric = 0;
			int indexOfBestMetric = -1;
			Point tmp = point;
			for (int i=0; i<9; ++i)	{
				point.x = tmp.x + offs[i].x * step;
				point.y = tmp.y + offs[i].y * step;

				double curMetric = calcMetric();
				if (indexOfBestMetric == -1 || curMetric < bestMetric) {
					bestMetric = curMetric;
					indexOfBestMetric = i;
				}
			}
			point.x = tmp.x + offs[indexOfBestMetric].x * step;
			point.y = tmp.y + offs[indexOfBestMetric].y * step;
			if (fabs(offs[indexOfBestMetric].x) + fabs(offs[indexOfBestMetric].y) > 0) {
				isMoved = true;
			}
		}

		std::cout << "maxDiff:" << calcMaxDiff() << "\n";      // Печатаем максимальное среди все рёбер отклонение целевой длины ребра от текущей длины ребра

		if (!isMoved) {
			break;
		}
	}
}

void calcEdgesLens()
{
	for (Edge& edge : edges) {
		Point& point1 = points[edge.ind[0]];
		Point& point2 = points[edge.ind[1]];
		double dx = point1.x - point2.x;
		double dy = point1.y - point2.y;
		edge.targetLen = sqrt(dx * dx + dy * dy);
	}

}


void printPoints()
{
	std::cout << "\n";
	for (Point& point : points) {
		std::cout << point.x << "  " << point.y << "\n";
	}
}

void rotatePoint(double* xNew, double* yNew, double x, double y, double alpha)
{
	*xNew = x * cos(alpha) - y * sin(alpha);
	*yNew = x * sin(alpha) + y * cos(alpha);
}

void turnPointsToAlignVertical(std::vector<Point>& points, int indexUp, int indexDown)
{
	double alpha = atan2(points[indexDown].y - points[indexUp].y, points[indexUp].x - points[indexDown].x) - 3.14159265 / 2;
	for (auto& point: points)
	{
		double xNew = 0;
		double yNew = 0;
		rotatePoint(&xNew, &yNew, point.x, point.y, alpha);
		point.x = xNew;
		point.y = yNew;
	}
}

void shiftPoints(std::vector<Point>& points, int index, double x, double y)
{
	double xOffs = points[index].x;
	double yOffs = points[index].y;

	for (auto& point : points)
	{
		point.x += x - xOffs;
		point.y += y - yOffs;
	}
}


void initPoints()
{
	// Индексы точек для поворота вертикально
	//indexUp = 6;
	//indexDown = 3;
	indexUp = 7;
	indexDown = 6;

	// Индекс точки для сдвига на указанную координату после поворота
	//indexShift = 6;
	//xShift = 0;
	//yShift = 0;
	indexShift = 7;
	xShift = 10;
	yShift = 10;

	// Координаты точек, с которых начинаем оптимизацию. Заданы так, чтобы примерно иметь топологию похожую на реальную
	//points = { {130,-160}, {130,-110}, {150,-110}, {10,-10}, {-200,-70}, {20,-70}, {20,-130} };
	points = { {120,20}, {120,70}, {120,110}, {120,140}, {80,140}, {80,160}, {20,160},{20,20} };
}

void initEdges()
{
	//edges = { {{0,1}, 0},
	//	{{0,3}, 0},
	//	{{0,5}, 0},
	//	{{0,6}, 0},
	//	{{1,2}, 0},
	//	{{1,3}, 0},
	//	{{1,5}, 0},
	//	{{1,6}, 0} ,
	//	{{2,3}, 0},
	//	{{2,5}, 0},
	//	{{3,4}, 0},
	//	{{3,5}, 0},
	//	{{4,5}, 0},
	//	{{5,6}, 0} };

	edges = { {{0,1}},
{{0,2}},
{{0,3}},
{{0,4}},
{{0,7}},
{{1,2}},
{{1,3}},
{{1,4}},
{{1,6}},
{{1,7}},
{{2,3}},
{{2,4}},
{{2,6}},
{{2,7}},
{{3,4}},
{{3,7}},
{{4,5}},
{{4,6}},
{{4,7}},
{{5,6}},
{{5,7}},
{{6,7}} };


}

void initTestData()
{
	// Реальные координаты точек (используются, чтобы расчитать замеренные (целевые) длины рёбер. Считаем, что погрешности измерений нет)

	//points = { {280,-340}, {290,-240}, {340,-240}, {40,-40}, {30,-180}, {50,-180}, {40,-300} };
	points = { {140,20}, {140,60}, {130,90}, {130,120}, {90,120}, {80,150}, {10,160}, {10,10} };

	calcEdgesLens();
}

int main()
{
	initEdges();
	initTestData();  // !!!При реальной триагнуляции закомментировать!
	initPoints();

	optimize();
	turnPointsToAlignVertical(points, indexUp, indexDown);
	shiftPoints(points,indexShift, xShift, yShift);
	printPoints();
}

