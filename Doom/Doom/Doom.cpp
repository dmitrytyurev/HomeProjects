// Doom.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Doom.h"

#include <iostream>
#include <vector>
#include <string>
#include <xutility>
#include "LevelData.h"
#include "Utils.h"

#define MAX_LOADSTRING 100

void InitGameLogic();


// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// -------------------------------------------------------------------

void Update();
void Draw(HWND hWnd);

const int bufSizeX = 500;
const int bufSizeY = 340;
const double zNear = 1;

// -------------------------------------------------------------------

// Описание игрового уровня
std::vector<FPoint2D> verts;
std::vector<Poly> polies;
std::vector<Texture> textures;

// Параметры камеры
float xCam, yCam, zCam;  // Позиция камеры в мире
float alCam;  // Угол вращения камеры. Если 0, то смотрит вдоль оси OZ
float horizontalAngle = 90.0;  // Горизонтальный угол обзора камеры в градусах

// -------------------------------------------------------------------
// Переменные отрисовки текущего кадра
unsigned char buf[bufSizeY][bufSizeX][3];
double horCamlAngleRad;
double dz;
double kProj;
int startingPoly;

double ceilNearU = 0;
double ceilNearV = 0;
double ceilNearBr = 0;
double floorNearU = 0;
double floorNearV = 0;
double floorNearBr = 0;

// -------------------------------------------------------------------

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DOOM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	InitGameLogic();

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DOOM));

    MSG msg;
		
    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }

    return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DOOM));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DOOM);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Разобрать выбор в меню:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
		//PAINTSTRUCT ps;
            //HDC hdc = BeginPaint(hWnd, &ps);
            //// TODO: Добавьте сюда любой код прорисовки, использующий HDC...
            //EndPaint(hWnd, &ps);

		Update();
		Draw(hWnd);

        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_KEYDOWN:
		//PrintConsole("WM_KEYDOWN %d %d", int(wParam), int(lParam));
		if (wParam == 27)
			PostQuitMessage(0);
		break;
	default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


// -------------------------------------------------------------------


void DrawFrameBuf(HWND hWnd)
{
	int size = bufSizeX * bufSizeY * 3;

	HDC dc = GetDC(hWnd);

	BITMAPINFO info;
	ZeroMemory(&info, sizeof(BITMAPINFO));
	info.bmiHeader.biBitCount = 24;
	info.bmiHeader.biWidth = bufSizeX;
	info.bmiHeader.biHeight = bufSizeY;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biSizeImage = size;
	info.bmiHeader.biCompression = BI_RGB;

	StretchDIBits(dc, 0, 0, bufSizeX * 2, bufSizeY * 2, 0, 0, bufSizeX, bufSizeY, buf, &info, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(hWnd, dc);
}


// -------------------------------------------------------------------

void DrawOneColumn(double scanAngle, int columnN)
{
	constexpr int MAX_VERTS_IN_POLY = 100;
	static bool leftFlags[MAX_VERTS_IN_POLY] = {};      // Флаги xn < 0 для вершин текущего полигона
	static DPoint2D localVerts[MAX_VERTS_IN_POLY] = {};  // Координаты полигона в системе координат, в которой текущий секущий луч (для текущего столбца пикселов) является осью x=0

	const float turnAngle = -scanAngle + alCam; // Угол, на который надо повернуть вершины полигонов, чтобы текущий секущий луч (для текущего столбца пикселов) являлся осью x=0
	const double co = cos(turnAngle);
	const double si = sin(turnAngle);
	const double coCam = cos(scanAngle);        // Последующий поворот на этот угол, переведвёт точку из системы секущего луча в систему камеры (в ней нам нужна z-координата точки для проекции Y на экран)

	const double co2 = cos(alCam);
	const double si2 = sin(alCam);
	const double co3 = cos(-scanAngle);
	const double si3 = sin(-scanAngle);

	int curPolyN = startingPoly;
	int edgeNComeFrom = -1;     // Номер ребра в текущем полигоне через которое мы пришли из предыдущего полигона
	double zSlicePrev = -1;  // Z-координата предыдущей точки сечения в системе координат камеры
	double interpEdgePrev = 0;  // [0..1] Пропорция в которой поделено предыдущее ребро
	

	int lastDrawedY1 = -1;         // Самый нижний отрисованный пиксел сверху
	int lastDrawedY2 = bufSizeY;   // Самый верхний отрисованный пиксел снизу

	while(true)	{
		Poly& poly = polies[curPolyN];
		int vertsInPoly = poly.edges.size();

		// Переводим вершины полигона в систему, где секущий луч это x=0
		for (int i = 0; i < vertsInPoly; ++i) {
			FPoint2D& curPoint = verts[poly.edges[i].firstInd];
			double x = double(curPoint.x) - xCam;
			double z = double(curPoint.z) - zCam;
			localVerts[i].x = x * co + z * si;
			localVerts[i].z = -x * si + z * co;
		}

		// Для вершин полигона заполняем флаги xn < 0
		for (int i = 0; i < vertsInPoly; ++i) {
			leftFlags[i] = localVerts[i].x < 0;
		}
		// Расчитаем пересечения рёбер полигона с осью x=0, пропуская рёбра если они с одной стороны от оси или если мы через это ребро пришли из предыдущего полигона
		// Выберём ребро с максимальным z-пересечения с осью
		double maxZ = -100000;
		int edgeWithMaxZ = -1;
		double interpEdge = 0;
		for (int i = 0; i < vertsInPoly; ++i) {       // Цикл по рёбрам полигона
			if (i == edgeNComeFrom) {                 // Через это ребро мы пришли из предыдущего полигона
				continue;
			}
			int nextInd = (i + 1) % vertsInPoly;
			if (leftFlags[i] == leftFlags[nextInd]) {  // Ребро лежит по одну сторону от секущего луча и значит пересекать его не может
				continue;
			}
			double zIntersect = localVerts[i].z - localVerts[i].x * (localVerts[nextInd].z - localVerts[i].z) / (localVerts[nextInd].x - localVerts[i].x);
			if (zIntersect > maxZ) {
				maxZ = zIntersect;
				edgeWithMaxZ = i;
				interpEdge = -localVerts[i].x / (localVerts[nextInd].x - localVerts[i].x);  // [0..1]  в какой пропорции проделено ребро секущим лучом. Испльзуем это для расчёт U-координаты стен
			}
		}
		if (edgeWithMaxZ == -1)	{
			std::string str = "Error: edgeWithMaxZ == -1.\n";
			str += "columnN:" + std::to_string(columnN) + "\n";
			str += "zCam:" + std::to_string(zCam) + "\n";
			str += "alCam:" + std::to_string(alCam) + "\n";
			ExitMsg(str.c_str());
		}

		// Поворачиваем найденную точку сечения ребра (0, maxZ) обратно, но не полностью - в систему камеры (где ось взгляда камеры - 0Z)
		// Точнее мы поворачиваем не всею точку, а только её Z-координату. Только она нам нужна дальше - для проекции Y на экран
		double zSliceCur = maxZ * coCam;     // Z-координата полученной точки сечения в системе координат камеры
		if (zSliceCur >= zNear) {
			if (zSlicePrev < zNear - 0.001)	{
				// Текущий отрезок залезает в zNear, поэтому предыдущей точкой считаем (poly.yFloor; zNear) и (poly.yCeil; zNear)
				// Соответственно в zSlicePrev запишем zNear, а floorCeilNearU, floorCeilNearV расчитаем так:
				// Сначала найдём сечение текущего полигона прямой z=zNear, потом найдём сечение полученного отрезка текущим секущим лучом
				zSlicePrev = zNear;

				// Переводим вершины полигона в систему, где секущая прямая это z=zNear
				for (int i = 0; i < vertsInPoly; ++i) {
					FPoint2D& curPoint = verts[poly.edges[i].firstInd];
					double x = double(curPoint.x) - xCam;
					double z = double(curPoint.z) - zCam;
					localVerts[i].x = x * co2 + z * si2;
					localVerts[i].z = -x * si2 + z * co2 - zNear;
				}
				// Для вершин полигона заполняем флаги zn < 0
				for (int i = 0; i < vertsInPoly; ++i) {
					leftFlags[i] = localVerts[i].z < 0;
				}
				// Расчитаем пересечения рёбер полигона с осью z=zNear, пропуская рёбра если они с одной стороны от оси
				double maxX = -100000;
				int edgeWithMaxX = -1;
				double interpEdgeMaxX = 0;
				double minX = 100000;
				int edgeWithMinX = -1;
				double interpEdgeMinX = 0;
				for (int i = 0; i < vertsInPoly; ++i) {       // Цикл по рёбрам полигона
					int nextInd = (i + 1) % vertsInPoly;
					if (leftFlags[i] == leftFlags[nextInd]) {  // Ребро лежит по одну сторону от секущей прямой и значит пересекать его не может
						continue;
					}
					double xIntersect = localVerts[i].x - localVerts[i].z * (localVerts[nextInd].x - localVerts[i].x) / (localVerts[nextInd].z - localVerts[i].z);
					double interp = -localVerts[i].z / (localVerts[nextInd].z - localVerts[i].z);  // [0..1]  в какой пропорции проделено ребро секущим лучом. Испльзуем это для расчёт UV-координат краёв полученного отрезка
					if (xIntersect > maxX) {
						maxX = xIntersect;
						edgeWithMaxX = i;
						interpEdgeMaxX = interp;
					}
					if (xIntersect < minX) {
						minX = xIntersect;
						edgeWithMinX = i;
						interpEdgeMinX = interp;
					}
				}
				// Рассчитаем UV на концах отрезка полученного рассечением полигона прямой z=zNear
				if (edgeWithMaxX == -1 && edgeWithMinX == -1) {
					ExitMsg("Error: edgeWithMaxX == -1 && edgeWithMinX == -1 %f %f %f %d %f\n", xCam, yCam, zCam, columnN, alCam);
				}
				if (edgeWithMaxX == -1)	{
					edgeWithMaxX = edgeWithMinX;
					interpEdgeMaxX = interpEdgeMinX;
				}
				else
					if (edgeWithMinX == -1) {
						edgeWithMinX = edgeWithMaxX;
						interpEdgeMinX = interpEdgeMaxX;
					}
				double u1Ce = interpEdgeMinX * (poly.edges[(edgeWithMinX + 1) % vertsInPoly].uCeil - poly.edges[edgeWithMinX].uCeil) + poly.edges[edgeWithMinX].uCeil;
				double v1Ce = interpEdgeMinX * (poly.edges[(edgeWithMinX + 1) % vertsInPoly].vCeil - poly.edges[edgeWithMinX].vCeil) + poly.edges[edgeWithMinX].vCeil;
				double u2Ce = interpEdgeMaxX * (poly.edges[(edgeWithMaxX + 1) % vertsInPoly].uCeil - poly.edges[edgeWithMaxX].uCeil) + poly.edges[edgeWithMaxX].uCeil;
				double v2Ce = interpEdgeMaxX * (poly.edges[(edgeWithMaxX + 1) % vertsInPoly].vCeil - poly.edges[edgeWithMaxX].vCeil) + poly.edges[edgeWithMaxX].vCeil;

				double br1Ce = interpEdgeMinX * (poly.edges[(edgeWithMinX + 1) % vertsInPoly].brightCeil - poly.edges[edgeWithMinX].brightCeil) + poly.edges[edgeWithMinX].brightCeil;
				double br2Ce = interpEdgeMaxX * (poly.edges[(edgeWithMaxX + 1) % vertsInPoly].brightCeil - poly.edges[edgeWithMaxX].brightCeil) + poly.edges[edgeWithMaxX].brightCeil;

				double u1Fl = interpEdgeMinX * (poly.edges[(edgeWithMinX + 1) % vertsInPoly].uFloor - poly.edges[edgeWithMinX].uFloor) + poly.edges[edgeWithMinX].uFloor;
				double v1Fl = interpEdgeMinX * (poly.edges[(edgeWithMinX + 1) % vertsInPoly].vFloor - poly.edges[edgeWithMinX].vFloor) + poly.edges[edgeWithMinX].vFloor;
				double u2Fl = interpEdgeMaxX * (poly.edges[(edgeWithMaxX + 1) % vertsInPoly].uFloor - poly.edges[edgeWithMaxX].uFloor) + poly.edges[edgeWithMaxX].uFloor;
				double v2Fl = interpEdgeMaxX * (poly.edges[(edgeWithMaxX + 1) % vertsInPoly].vFloor - poly.edges[edgeWithMaxX].vFloor) + poly.edges[edgeWithMaxX].vFloor;

				double br1Fl = interpEdgeMinX * (poly.edges[(edgeWithMinX + 1) % vertsInPoly].brightFloor - poly.edges[edgeWithMinX].brightFloor) + poly.edges[edgeWithMinX].brightFloor;
				double br2Fl = interpEdgeMaxX * (poly.edges[(edgeWithMaxX + 1) % vertsInPoly].brightFloor - poly.edges[edgeWithMaxX].brightFloor) + poly.edges[edgeWithMaxX].brightFloor;

				// Полученный отрезок повернуть так, чтобы текущий секущий луч был прямой x=0
				double x1 = minX * co3 + zNear * si3;
				double z1 = -minX * si3 + zNear * co3;
				double x2 = maxX * co3 + zNear * si3;
				double z2 = -maxX * si3 + zNear * co3;
				if (x2 < x1) {
					ExitMsg("Error: x2 < x1\n");
				}
				if (x1 > 0.01 || x2 < -0.01) {
					ExitMsg("Error: x1 > 0.01 || x2 < -0.01\n");
				}
				if (x2 - x1 < 0.001)
				{
					ceilNearU = u1Ce;
					ceilNearV = v1Ce;
					ceilNearBr = br1Ce;
					floorNearU = u1Fl;
					floorNearV = v1Fl;
					floorNearBr = br1Fl;
				}
				else {
					if (x1 >= 0) {
						ceilNearU = u1Ce;
						ceilNearV = v1Ce;
						ceilNearBr = br1Ce;
						floorNearU = u1Fl;
						floorNearV = v1Fl;
						floorNearBr = br1Fl;
					}
					else {
						if (x2 <= 0) {
							ceilNearU = u2Ce;
							ceilNearV = v2Ce;
							ceilNearBr = br2Ce;
							floorNearU = u2Fl;
							floorNearV = v2Fl;
							floorNearBr = br2Fl;
						}
						else {
							double interp = -x1 / (x2 - x1);
							ceilNearU = (u2Ce - u1Ce) * interp + u1Ce;
							ceilNearV = (v2Ce - v1Ce) * interp + v1Ce;
							ceilNearBr = (br2Ce - br1Ce) * interp + br1Ce;
							floorNearU = (u2Fl - u1Fl) * interp + u1Fl;
							floorNearV = (v2Fl - v1Fl) * interp + v1Fl;
							floorNearBr = (br2Fl - br1Fl) * interp + br1Fl;
						}
					}
				}
			}

			// Проецируем (находим Y в системе координат экрана)
			// Для Z-пересечения текущего отрезка
			double yScrFloor1 = -(poly.yFloor - yCam) / zSliceCur * kProj + bufSizeY / 2;
			int    yiScrFloor1 = (int)floor(yScrFloor1 + 0.5);
			double yScrCeil1 = -(poly.yCeil - yCam) / zSliceCur * kProj + bufSizeY / 2;
			int    yiScrCeil1 = (int)floor(yScrCeil1 + 0.5);

			// Для Z-пересечения предыдущего отрезка
			double yScrFloor2 = -(poly.yFloor - yCam) / zSlicePrev * kProj + bufSizeY / 2;
			int    yiScrFloor2 = (int)floor(yScrFloor2 + 0.5);
			double yScrCeil2 = -(poly.yCeil - yCam) / zSlicePrev * kProj + bufSizeY / 2;
			int    yiScrCeil2 = (int)floor(yScrCeil2 + 0.5);
			double yScrRoof2 = -(poly.yRoof - yCam) / zSlicePrev * kProj + bufSizeY / 2;
			int    yiScrRoof2 = (int)floor(yScrRoof2 + 0.5);

			bool otherNotVisible = false;  // Остальные отрезки не видны

			if (edgeNComeFrom != -1 && zSlicePrev != zNear) {
				const Edge* edgeComeFrom = &poly.edges[edgeNComeFrom];
				double curU = (double(edgeComeFrom->u[0]) - edgeComeFrom->u[1]) * interpEdgePrev + edgeComeFrom->u[1];
				int wallBrightUp   = int(((edgeComeFrom->wallBrightsUp[0] - edgeComeFrom->wallBrightsUp[1]) * interpEdgePrev + edgeComeFrom->wallBrightsUp[1]) * 256);
				int wallBrightDown = int(((edgeComeFrom->wallBrightsDown[0] - edgeComeFrom->wallBrightsDown[1]) * interpEdgePrev + edgeComeFrom->wallBrightsDown[1]) * 256);

				double vDens = 0;
				if (yiScrRoof2 != yiScrCeil2) {
					vDens = -(double(poly.yRoof) - poly.yCeil) / (yScrRoof2 - yScrCeil2);
				}

				// Рисуем небо над внешней стенкой полигона ---------------------------------------------------------
				if (yiScrRoof2 > lastDrawedY2) {
					yiScrRoof2 = lastDrawedY2;
					otherNotVisible = true;
				}
				int from = lastDrawedY1 + 1;
				for (int y = from; y < yiScrRoof2; ++y) {
					unsigned char(&pixel)[3] = buf[y][columnN];
					pixel[0] = 255;
					pixel[1] = 192;
					pixel[2] = 0;
				}
				if (yiScrRoof2 > lastDrawedY1 + 1)
					lastDrawedY1 = yiScrRoof2 - 1;
				if (lastDrawedY1 >= lastDrawedY2 - 1)
					break;
				if (otherNotVisible)
					goto m1;

				// Рисуем внешнюю стенку полигона над потолком  ---------------------------------------------------------
				if (yiScrRoof2 <= lastDrawedY1)
					yiScrRoof2 = lastDrawedY1 + 1;
				if (yiScrCeil2 > lastDrawedY2) {
					yiScrCeil2 = lastDrawedY2;
					otherNotVisible = true;
				}
				if (yiScrCeil2 > yiScrRoof2) {
					double addV = edgeComeFrom->vWallUpAdd * vDens;
					double curV = edgeComeFrom->vWallUp - addV * (yScrCeil2 - (yiScrRoof2 + 0.5));  // В скобках сдвиг от необрезанного yScrCeil2 для которого задана V, до центра верхнего пиксела обрезанного отрезка, откуда начнём рисовать и где нам нужен V
					const Texture& curTex = textures[edgeComeFrom->texUp.texIndex];
					int ui = int(curU * curTex.sizeX) & (curTex.sizeX - 1);
					unsigned char* texture = curTex.buf + (ui << 2);
					for (int y = yiScrRoof2; y < yiScrCeil2; ++y) {
						unsigned char(&dst)[3] = buf[y][columnN];
						int vi = int(curV * curTex.sizeY) & (curTex.sizeY - 1);
						unsigned char* src = texture + ((vi << curTex.xPow2) << 2);
						dst[0] = (src[2] * wallBrightUp) >> 8;
						dst[1] = (src[1] * wallBrightUp) >> 8;
						dst[2] = (src[0] * wallBrightUp) >> 8;
						curV += addV;
					}
					if (yiScrCeil2 > lastDrawedY1 + 1) {
						lastDrawedY1 = yiScrCeil2 - 1;
						if (lastDrawedY1 >= lastDrawedY2 - 1)
							break;
					}
				}
				if (otherNotVisible)
					goto m1;

				// Рисуем внешнюю стенку полигона под полом  ---------------------------------------------------------
				double subPixelCorrection = (yiScrFloor2 + 0.5 - yScrFloor2);  // Субпиксельная коррекция, чтобы V-координата бралась для центра верхнего пиксела полигона
				int keep1 = yiScrFloor2;
				if (yiScrFloor2 <= lastDrawedY1) {
					yiScrFloor2 = lastDrawedY1 + 1;
					otherNotVisible = true;
				}
				if (lastDrawedY2 > yiScrFloor2) {
					double addV = edgeComeFrom->vWallDownAdd * vDens; // Изменение V-координаты с каждым пикселем
					double curV = edgeComeFrom->vWallDown + addV * (yiScrFloor2 - keep1 + subPixelCorrection);
					const Texture& curTex = textures[edgeComeFrom->texDown.texIndex];
					int ui = int(curU * curTex.sizeX) & (curTex.sizeX - 1);
					unsigned char* texture = curTex.buf + (ui << 2);
					for (int y = yiScrFloor2; y < lastDrawedY2; ++y) {
						unsigned char(&dst)[3] = buf[y][columnN];
						int vi = int(curV * curTex.sizeY) & (curTex.sizeY - 1);
						unsigned char* src = texture + ((vi << curTex.xPow2) << 2);
						dst[0] = (src[2] * wallBrightDown) >> 8;
						dst[1] = (src[1] * wallBrightDown) >> 8;
						dst[2] = (src[0] * wallBrightDown) >> 8;
						curV += addV;
					}
					if (yiScrFloor2 < lastDrawedY2) {
						lastDrawedY2 = yiScrFloor2;
						if (lastDrawedY1 >= lastDrawedY2 - 1)
							break;
					}
				}
				if (otherNotVisible)
					goto m1;
			}

			// Рисуем потолок полигона  ---------------------------------------------------------
			{
				if (yiScrCeil2 <= lastDrawedY1)
					yiScrCeil2 = lastDrawedY1 + 1;
				if (yiScrCeil1 > lastDrawedY2) {
					yiScrCeil1 = lastDrawedY2;
					otherNotVisible = true;
				}
				if (yiScrCeil1 > yiScrCeil2) {
					double u1 = ceilNearU;
					double v1 = ceilNearV;
					double br1 = ceilNearBr;
					double u1DivZ = u1 / zSlicePrev;
					double v1DivZ = v1 / zSlicePrev;
					double br1DivZ = br1 / zSlicePrev;
					double oneDivZ1 = 1 / zSlicePrev;

					const Edge& edgeCur = poly.edges[edgeWithMaxZ];
					const Edge& edgeCurNext = poly.edges[(edgeWithMaxZ + 1) % vertsInPoly];
					double u2 = (double(edgeCurNext.uCeil) - edgeCur.uCeil) * interpEdge + edgeCur.uCeil;
					double v2 = (double(edgeCurNext.vCeil) - edgeCur.vCeil) * interpEdge + edgeCur.vCeil;
					double br2 = (double(edgeCurNext.brightCeil) - edgeCur.brightCeil) * interpEdge + edgeCur.brightCeil;
					double u2DivZ = u2 / zSliceCur;
					double v2DivZ = v2 / zSliceCur;
					double br2DivZ = br2 / zSliceCur;
					double oneDivZ2 = 1 / zSliceCur;

					double srcInterp1 = ((yiScrCeil2 + 0.5) - yScrCeil2) / (yScrCeil1 - yScrCeil2);
					double uCur = (u2DivZ - u1DivZ) * srcInterp1 + u1DivZ;
					double vCur = (v2DivZ - v1DivZ) * srcInterp1 + v1DivZ;
					double brCur = (br2DivZ - br1DivZ) * srcInterp1 + br1DivZ;
					double zCur = (oneDivZ2 - oneDivZ1) * srcInterp1 + oneDivZ1;

					double srcInterp2 = (yiScrCeil1 + 0.5 - yScrCeil2) / (yScrCeil1 - yScrCeil2);
					double uCorr = (u2DivZ - u1DivZ) * srcInterp2 + u1DivZ;
					double vCorr = (v2DivZ - v1DivZ) * srcInterp2 + v1DivZ;
					double brCorr = (br2DivZ - br1DivZ) * srcInterp2 + br1DivZ;
					double zCorr = (oneDivZ2 - oneDivZ1) * srcInterp2 + oneDivZ1;

					double uAdd = (uCorr - uCur) / (yiScrCeil1 - yiScrCeil2);
					double vAdd = (vCorr - vCur) / (yiScrCeil1 - yiScrCeil2);
					double brAdd = (brCorr - brCur) / (yiScrCeil1 - yiScrCeil2);
					double zAdd = (zCorr - zCur) / (yiScrCeil1 - yiScrCeil2);

					const Texture& curTex = textures[poly.ceilTex.texIndex];
					unsigned char* texture = curTex.buf;
					for (int y = yiScrCeil2; y < yiScrCeil1; ++y) {
						unsigned char(&dst)[3] = buf[y][columnN];
						double uPixel = uCur / zCur;
						double vPixel = vCur / zCur;
						int brPixel = int(brCur / zCur);
						int ui = int(uPixel * curTex.sizeX) & (curTex.sizeX - 1);
						int vi = int(vPixel * curTex.sizeY) & (curTex.sizeY - 1);
						unsigned char* src = texture + (((vi << curTex.xPow2) + ui) << 2);
						dst[0] = int(src[2] * brPixel) >> 8;
						dst[1] = int(src[1] * brPixel) >> 8;
						dst[2] = int(src[0] * brPixel) >> 8;
						uCur += uAdd;
						vCur += vAdd;
						brCur += brAdd;
						zCur += zAdd;
					}
					if (yiScrCeil1 > lastDrawedY1 + 1) {
						lastDrawedY1 = yiScrCeil1 - 1;
						if (lastDrawedY1 >= lastDrawedY2 - 1)
							break;
					}
				}
				if (otherNotVisible)
					goto m1;
			}

			// Рисуем пол полигона  ---------------------------------------------------------
			{
				if (yiScrFloor1 <= lastDrawedY1)
					yiScrFloor1 = lastDrawedY1 + 1;
				if (yiScrFloor2 > lastDrawedY2)
					yiScrFloor2 = lastDrawedY2;
				if (yiScrFloor2 > yiScrFloor1) {
					const Edge& edgeCur = poly.edges[edgeWithMaxZ];
					const Edge& edgeCurNext = poly.edges[(edgeWithMaxZ + 1) % vertsInPoly];
					double u1 = (double(edgeCurNext.uFloor) - edgeCur.uFloor) * interpEdge + edgeCur.uFloor;
					double v1 = (double(edgeCurNext.vFloor) - edgeCur.vFloor) * interpEdge + edgeCur.vFloor;
					double br1 = (double(edgeCurNext.brightFloor) - edgeCur.brightFloor) * interpEdge + edgeCur.brightFloor;
					double u1DivZ = u1 / zSliceCur;
					double v1DivZ = v1 / zSliceCur;
					double br1DivZ = br1 / zSliceCur;
					double oneDivZ1 = 1 / zSliceCur;

					double u2 = floorNearU;
					double v2 = floorNearV;
					double br2 = floorNearBr;
					double u2DivZ = u2 / zSlicePrev;
					double v2DivZ = v2 / zSlicePrev;
					double br2DivZ = br2 / zSlicePrev;
					double oneDivZ2 = 1 / zSlicePrev;

					double srcInterp1 = ((yiScrFloor1 + 0.5) - yScrFloor1) / (yScrFloor2 - yScrFloor1);
					double uCur = (u2DivZ - u1DivZ) * srcInterp1 + u1DivZ;
					double vCur = (v2DivZ - v1DivZ) * srcInterp1 + v1DivZ;
					double brCur = (br2DivZ - br1DivZ) * srcInterp1 + br1DivZ;
					double zCur = (oneDivZ2 - oneDivZ1) * srcInterp1 + oneDivZ1;

					double srcInterp2 = (yiScrFloor2 + 0.5 - yScrFloor1) / (yScrFloor2 - yScrFloor1);
					double uCorr = (u2DivZ - u1DivZ) * srcInterp2 + u1DivZ;
					double vCorr = (v2DivZ - v1DivZ) * srcInterp2 + v1DivZ;
					double brCorr = (br2DivZ - br1DivZ) * srcInterp2 + br1DivZ;
					double zCorr = (oneDivZ2 - oneDivZ1) * srcInterp2 + oneDivZ1;

					double uAdd = (uCorr - uCur) / (yiScrFloor2 - yiScrFloor1);
					double vAdd = (vCorr - vCur) / (yiScrFloor2 - yiScrFloor1);
					double brAdd = (brCorr - brCur) / (yiScrFloor2 - yiScrFloor1);
					double zAdd = (zCorr - zCur) / (yiScrFloor2 - yiScrFloor1);

					const Texture& curTex = textures[poly.floorTex.texIndex];
					unsigned char* texture = curTex.buf;
					for (int y = yiScrFloor1; y < yiScrFloor2; ++y) {
						unsigned char(&dst)[3] = buf[y][columnN];
						double uPixel = uCur / zCur;
						double vPixel = vCur / zCur;
						int brPixel = int(brCur / zCur);
						int ui = int(uPixel * curTex.sizeX) & (curTex.sizeX - 1);
						int vi = int(vPixel * curTex.sizeY) & (curTex.sizeY - 1);
						unsigned char* src = texture + (((vi << curTex.xPow2) + ui) << 2);
						dst[0] = int(src[2] * brPixel) >> 8;
						dst[1] = int(src[1] * brPixel) >> 8;
						dst[2] = int(src[0] * brPixel) >> 8;
						uCur += uAdd;
						vCur += vAdd;
						brCur += brAdd;
						zCur += zAdd;
					}
					if (yiScrFloor1 < lastDrawedY2)
					{
						lastDrawedY2 = yiScrFloor1;
						if (lastDrawedY1 >= lastDrawedY2 - 1)
							break;
					}
				}
			}
		}

m1: 	zSlicePrev = zSliceCur;
		interpEdgePrev = interpEdge;
		edgeNComeFrom = poly.edges[edgeWithMaxZ].adjEdgeN;
		curPolyN = poly.edges[edgeWithMaxZ].adjPolyN;
		if (curPolyN == -1) {
			// Рисуем пустоту (где уже нет лабиринта)
			for (int y = lastDrawedY1+1; y < lastDrawedY2; ++y) {
				unsigned char(&pixel)[3] = buf[y][columnN];
				pixel[0] = 0;
				pixel[1] = 60;
				pixel[2] = 0;
			}
			break;                                   
		}
		Poly& poly2 = polies[curPolyN];
		const Edge& edgeComeFrom = poly2.edges[edgeNComeFrom];
		const Edge& edgeComeFromNextT = poly2.edges[(edgeNComeFrom + 1) % poly2.edges.size()];
		ceilNearU = (double(edgeComeFrom.uCeil) - edgeComeFromNextT.uCeil) * interpEdgePrev + edgeComeFromNextT.uCeil;
		ceilNearV = (double(edgeComeFrom.vCeil) - edgeComeFromNextT.vCeil) * interpEdgePrev + edgeComeFromNextT.vCeil;
		floorNearU = (double(edgeComeFrom.uFloor) - edgeComeFromNextT.uFloor) * interpEdgePrev + edgeComeFromNextT.uFloor;
		floorNearV = (double(edgeComeFrom.vFloor) - edgeComeFromNextT.vFloor) * interpEdgePrev + edgeComeFromNextT.vFloor;
		floorNearBr = (double(edgeComeFrom.brightFloor) - edgeComeFromNextT.brightFloor) * interpEdgePrev + edgeComeFromNextT.brightFloor;
		ceilNearBr = (double(edgeComeFrom.brightCeil) - edgeComeFromNextT.brightCeil) * interpEdgePrev + edgeComeFromNextT.brightCeil;
	}
}

// -------------------------------------------------------------------

int FindPolygonUnderCamera() // Возвращает индекс полигона под камерой (или -1)
{
	const double DELTA = 0.001;
	int curPolyN = 0;
	for (; curPolyN < polies.size(); ++curPolyN) {
		const Poly& poly = polies[curPolyN];
		int edgesNum = poly.edges.size();
		int nearEdge1N = -1;
		int nearEdge2N = -1;
		int i = 0;
		for (; i < edgesNum; ++i) {
			const Edge& edge1 = poly.edges[i];
			const Edge& edge2 = poly.edges[(i + 1) % edgesNum];
			const FPoint2D& vert1 = verts[edge1.firstInd];
			const FPoint2D& vert2 = verts[edge2.firstInd];

			double dx = double(vert2.x) - vert1.x;
			double dz = double(vert2.z) - vert1.z;
			double angle = -atan2(dx, dz);
			const double co = cos(angle);
			const double si = sin(angle);

			double xRef = vert1.x * co + vert1.z * si;
			double xCompare = xCam * co + zCam * si;

			double dist = xCompare - xRef;
			if (dist < 0) {
				break;              // Камера за пределами текущего полигона (снаружи от текущего ребра). Нет смысла проверять остальные рёбра полигона
			}
			if (dist < DELTA) {
				if (nearEdge1N == -1)
					nearEdge1N = i;
				else
					if (nearEdge2N == -1)
						nearEdge2N = i;
					else
						ExitMsg("More than 2 close edges");
			}
		}
		if (i == edgesNum) {          // Нашли полигон внутри которого мы
			if (nearEdge1N != -1) {   // Если одно или два ребра этого полигона слишком близко к камере, но отодвинем камеру от них
				FPoint2D& vr1 = verts[poly.edges[nearEdge1N].firstInd];
				FPoint2D& vr2 = verts[poly.edges[(nearEdge1N + 1) % edgesNum].firstInd];
				double nX = vr2.z - vr1.z;
				double nZ = vr1.x - vr2.x;
				double len = sqrt(nX * nX + nZ * nZ);
				double normalX = nX / len;
				double normalZ = nZ / len;
				if (nearEdge2N != -1) {
					FPoint2D& vr1b = verts[poly.edges[nearEdge2N].firstInd];
					FPoint2D& vr2b = verts[poly.edges[(nearEdge2N + 1) % edgesNum].firstInd];
					nX = vr2b.z - vr1b.z;
					nZ = vr1b.x - vr2b.x;
					len = sqrt(nX * nX + nZ * nZ);
					normalX += nX / len;
					normalZ += nZ / len;
				}

				xCam += normalX * DELTA;
				zCam += normalZ * DELTA;
			}
			break;
		}
	}
	if (curPolyN == polies.size())
		return -1;

	return curPolyN;
}

// -------------------------------------------------------------------

void InitGameLogic()
{
	FillLevelData(verts, polies, textures);

	xCam = 310; // ; 794
	yCam = 1060;
	zCam = -1398; // ;  -1390

	xCam = 690;
	zCam = -750;

}

// -------------------------------------------------------------------
const float TRUN_SPEED = 0.02f;
const float MOVE_SPEED = 0.5;

void Update()
{
	if (GetKeyState(VK_LEFT) & 0x8000)
	{
		alCam += TRUN_SPEED;
	}
	if (GetKeyState(VK_RIGHT) & 0x8000)
	{
		alCam -= TRUN_SPEED;
	}
	if (GetKeyState(VK_UP) & 0x8000)
	{
		xCam += sin(-alCam) * MOVE_SPEED;
		zCam += cos(-alCam) * MOVE_SPEED;
	}
	if (GetKeyState(VK_DOWN) & 0x8000)
	{
		xCam -= sin(-alCam) * MOVE_SPEED;
		zCam -= cos(-alCam) * MOVE_SPEED;
	}
	static double t = 0;
	t += 0.005f;
	polies[77].yFloor = ((sin(t) + 1.f) * 0.5f) * 120 + 1000;
}

// -------------------------------------------------------------------

void Draw(HWND hWnd)
{
	horCamlAngleRad = horizontalAngle / 180.0 * 3.14159265359;
	dz = 1.0 / tan(horCamlAngleRad / 2.0);
	kProj = bufSizeX / 2 / tan(horCamlAngleRad / 2);
	startingPoly = FindPolygonUnderCamera();
	if (startingPoly == -1) {
		return;
	}

	for (int x = 0; x < bufSizeX; ++x) {
		double curX = (x - bufSizeX / 2 + 0.5) * 2.0 / bufSizeX;  // Текущая горизонтальная позиция пиксела в системе камеры  -1..1
		double scanAngle = atan(curX / dz);  // Угол на который нужно повернуть вершины сцены, чтобы привести их в систему координат, где текущий секущий луч (для текущего столбца пикселов) был осью x=0
		DrawOneColumn(scanAngle, x);
	}

	int off = bufSizeY - 1;
	for (int y = 0; y < bufSizeY/2; ++y)
	{
		for (int x = 0; x < bufSizeX; ++x)
		{
			unsigned char t = buf[y][x][0];
			buf[y][x][0] = buf[off - y][x][0];
			buf[off-y][x][0] = t;

			t = buf[y][x][1];
			buf[y][x][1] = buf[off - y][x][1];
			buf[off - y][x][1] = t;

			t = buf[y][x][2];
			buf[y][x][2] = buf[off - y][x][2];
			buf[off - y][x][2] = t;
		}
	}


	//for (int i = 0; i < bufSizeX; ++i)
	//{
	//	for (int i2 = 0; i2 < bufSizeY; ++i2)
	//	{
	//		buf[i2][i][0] = 0;
	//		buf[i2][i][1] = 0;
	//		buf[i2][i][2] = rand();
	//	}
	//}

	DrawFrameBuf(hWnd);
}
