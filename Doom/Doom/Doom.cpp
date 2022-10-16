// Doom.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Doom.h"

#include <iostream>
#include <vector>
#include <xutility>

#define MAX_LOADSTRING 100

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

// -------------------------------------------------------------------

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
	int firstInd;  // Индекс первой вершины ребра в verts. Индекс второй вершины ребра берём из следующего ребра в edges
	int adjPolyN;  // Индекс смежного полигона в polies, либо -1, если смежного полигона нет
	int adjEdgeN;  // Индекс смежного ребра в edges смежного полигона
	// Текстурные координаты стен
	float u[2];    // U-координаты в первой и второй точках ребра
	float vCeil;     // V-координата а уровне потолка
	float vCeilAdd;  // Приращение V-координаты (в единицах [0..1] на 1 метр сдвига вниз)
	float vFloor;    // V-координата а уровне пола
	float vFloorAdd; // Приращение V-координаты (в единицах [0..1] на 1 метр сдвига вниз)
	// Текстурные координаты пола/потолка
	float uFloorCeil;
	float vFloorCeil;
};

struct Poly
{
	float yRoof;
	float yCeil;
	float yFloor;
	std::vector<Edge> edges;
};
// -------------------------------------------------------------------

const int bufSizeX = 500;
const int bufSizeY = 340;

// -------------------------------------------------------------------

// Описание игрового уровня
std::vector<FPoint2D> verts = {{10,30}, {20,30}, {10,20} , {20,20} , {10,10} , {20,10} };
std::vector<Poly> polies = {
	{45,44,41,
		{{0,-1,-1,{0,10}, 1, 1, 0, 1,    0 ,0 },
		       {1,-1,-1,{0,10}, 1, 1, 0, 1,    3, 0},
		       {3, 1, 0,{0,10}, 1, 1, 0, 1,    3, 3},
		       {2,-1,-1,{0,10}, 1, 1, 0, 1,    0, 3}}},

	{46,45,40,
		{{2, 0, 2,{0,1}, 5, 5, 0, 5},
			   {3,-1,-1,{0,1}, 5, 5, 0, 5},
			   {5,-1,-1,{0,1}, 5, 5, 0, 5},
			   {4,-1,-1,{0,1}, 5, 5, 0, 5}}} };

// Параметры камеры
float xCam=15, yCam=42.5f, zCam=14.5;  // Позиция камеры в мире
float alCam;  // Угол вращения камеры. Если 0, то смотрит вдоль оси OZ
float horizontalAngle = 90.0;  // Горизонтальный угол обзора камеры в градусах

// -------------------------------------------------------------------
// Переменные отрисовки текущего кадра
unsigned char buf[bufSizeY][bufSizeX][3];
double horCamlAngleRad;
double dz;
double kProj;
int startingPoly;

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

void Update()
{
	
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

	int curPolyN = startingPoly;
	int edgeNComeFrom = -1;     // Номер ребра в текущем полигоне через которое мы пришли из предыдущего полигона
	double zSlicePrev = 0.001;  // Z-координата предыдущей точки сечения в системе координат камеры
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
			OutputDebugStringA("Error: edgeWithMaxZ == -1\n");
			exit(1);
		}

		// Поворачиваем найденную точку сечения ребра (0, maxZ) обратно, но не полностью - в систему камеры (где ось взгляда камеры - 0Z)
		// Точнее мы поворачиваем не всею точку, а только её Z-координату. Только она нам нужна дальше - для проекции Y на экран
		double zSliceCur = maxZ * coCam;     // Z-координата полученной точки сечения в системе координат камеры
		if (zSliceCur < 0.001) {
			zSliceCur = 0.001;               // Ограничиваем значением, на которое можно будет ниже делить. Спроекцированные значения при этом уйдут за вернюю или нижнюю границу экрана - не страшно, отклипаем на общих основаниях
		}

		// Проецируем (находим Y в системе координат экрана)
		// Для Z-пересечения текущего отрезка
		double yScrFloor1 = -(poly.yFloor - yCam) / zSliceCur * kProj + bufSizeY / 2;
		int    yiScrFloor1 = (int)floor(yScrFloor1 + 0.5);
		double yScrCeil1  = -(poly.yCeil - yCam) / zSliceCur * kProj + bufSizeY / 2;
		int    yiScrCeil1 = (int)floor(yScrCeil1 + 0.5);

		// Для Z-пересечения предыдущего отрезка
		double yScrFloor2 = -(poly.yFloor - yCam) / zSlicePrev * kProj + bufSizeY / 2;
		int    yiScrFloor2 = (int)floor(yScrFloor2 + 0.5);
		double yScrCeil2  = -(poly.yCeil  - yCam) / zSlicePrev * kProj + bufSizeY / 2;
		int    yiScrCeil2 = (int)floor(yScrCeil2 + 0.5);
		double yScrRoof2  = -(poly.yRoof  - yCam) / zSlicePrev * kProj + bufSizeY / 2;
		int    yiScrRoof2 = (int)floor(yScrRoof2 + 0.5);

		if (edgeNComeFrom == -1)  // Рисуем первый полигон, для него нет предыдущего ребра, но оно и не нужно, поскольку используется только для стен, а стены в первом полигоне не рисуются
			edgeNComeFrom = 0;
		const Edge& edgeComeFrom = poly.edges[edgeNComeFrom];

		double curU = (double(edgeComeFrom.u[0]) - edgeComeFrom.u[1]) * interpEdgePrev + edgeComeFrom.u[1];

		double vDens = 0;
		if (yiScrRoof2 != yiScrCeil2) {
			vDens = -(double(poly.yRoof) - poly.yCeil) / (yScrRoof2 - yScrCeil2);
		}


		bool otherNotVisible = false;  // Остальные отрезки не видны

		// Рисуем небо над внешней стенкой полигона ---------------------------------------------------------
		if (yiScrRoof2 > lastDrawedY2) {
			yiScrRoof2 = lastDrawedY2;
			otherNotVisible = true;
		}
		int from = lastDrawedY1 + 1;
		for (int y = from; y < yiScrRoof2; ++y) {
			unsigned char(&pixel)[3] = buf[y][columnN];
			pixel[0] = 0;
			pixel[1] = 255;
			pixel[2] = 255;
		}
		if (yiScrRoof2 > lastDrawedY1 + 1)
			lastDrawedY1 = yiScrRoof2 - 1;
		if (lastDrawedY1 >= lastDrawedY2 - 1)
			break;  
		if (otherNotVisible) 
			goto m1;

		// Рисуем внешнюю стенку полигона над потолком  ---------------------------------------------------------
		{
			if (yiScrRoof2 <= lastDrawedY1)
				yiScrRoof2 = lastDrawedY1 + 1;
			if (yiScrCeil2 > lastDrawedY2) {
				yiScrCeil2 = lastDrawedY2;
				otherNotVisible = true;
			}
			if (yiScrCeil2 > yiScrRoof2) {
				double addV = edgeComeFrom.vCeilAdd * vDens;
				double curV = edgeComeFrom.vCeil - addV * (yScrCeil2 - (yiScrRoof2 + 0.5));  // В скобках сдвиг от необрезанного yScrCeil2 для которого задана V, до центра верхнего пиксела обрезанного отрезка, откуда начнём рисовать и где нам нужен V
				for (int y = yiScrRoof2; y < yiScrCeil2; ++y) {
					unsigned char(&pixel)[3] = buf[y][columnN];
					int scale = 5;
					pixel[0] = (((int(curU * scale) + int(curV * scale)) % 2) * curV) * 255;
					pixel[1] = (((int(curU * scale) + int(curV * scale)) % 2) * curV) * 255;
					pixel[2] = (((int(curU * scale) + int(curV * scale)) % 2) * curV) * 255;
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
		}

		// Рисуем внешнюю стенку полигона под полом  ---------------------------------------------------------
		{
			int keep1 = yiScrFloor2;
			if (yiScrFloor2 <= lastDrawedY1) {
				yiScrFloor2 = lastDrawedY1 + 1;
				otherNotVisible = true;
			}
			if (lastDrawedY2 > yiScrFloor2) {
				double addV = edgeComeFrom.vFloorAdd * vDens; // Изменение V-координаты с каждым пикселем
				double subPixelCorrection = (yiScrFloor2 + 0.5 - yScrFloor2);  // Субпиксельная коррекция, чтобы V-координата бралась для центра верхнего пиксела полигона
				double curV = edgeComeFrom.vFloor + addV * (yiScrFloor2 - keep1 + subPixelCorrection);
				for (int y = yiScrFloor2; y < lastDrawedY2; ++y) {
					unsigned char(&pixel)[3] = buf[y][columnN];
					int scale = 5;
					pixel[0] = (((int(curU * scale) + int(curV * scale)) % 2) * curV) * 255;
					pixel[1] = (((int(curU * scale) + int(curV * scale)) % 2) * curV) * 255;
					pixel[2] = (((int(curU * scale) + int(curV * scale)) % 2) * curV) * 255;
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
				const Edge& edgeComeFromNext = poly.edges[(edgeNComeFrom + 1) % vertsInPoly];
				double u1 = (double(edgeComeFrom.uFloorCeil) - edgeComeFromNext.uFloorCeil) * interpEdgePrev + edgeComeFromNext.uFloorCeil;
				double v1 = (double(edgeComeFrom.vFloorCeil) - edgeComeFromNext.vFloorCeil) * interpEdgePrev + edgeComeFromNext.vFloorCeil;
				double u1DivZ = u1 / zSlicePrev;
				double v1DivZ = v1 / zSlicePrev;
				double oneDivZ1 = 1 / zSlicePrev;

				const Edge& edgeCur = poly.edges[edgeWithMaxZ];
				const Edge& edgeCurNext = poly.edges[(edgeWithMaxZ + 1) % vertsInPoly];
				double u2 = (double(edgeCurNext.uFloorCeil) - edgeCur.uFloorCeil) * interpEdge + edgeCur.uFloorCeil;
				double v2 = (double(edgeCurNext.vFloorCeil) - edgeCur.vFloorCeil) * interpEdge + edgeCur.vFloorCeil;
				double u2DivZ = u2 / zSliceCur;
				double v2DivZ = v2 / zSliceCur;
				double oneDivZ2 = 1 / zSliceCur;

				double srcInterp1 = ((yiScrCeil2 + 0.5) - yScrCeil2) / (yScrCeil1 - yScrCeil2);
				double uCur = (u2DivZ - u1DivZ) * srcInterp1 + u1DivZ;
				double vCur = (v2DivZ - v1DivZ) * srcInterp1 + v1DivZ;
				double zCur = (oneDivZ2 - oneDivZ1) * srcInterp1 + oneDivZ1;

				double srcInterp2 = (yiScrCeil1 + 0.5 - yScrCeil2) / (yScrCeil1 - yScrCeil2);
				double uCorr = (u2DivZ - u1DivZ) * srcInterp2 + u1DivZ;
				double vCorr = (v2DivZ - v1DivZ) * srcInterp2 + v1DivZ;
				double zCorr = (oneDivZ2 - oneDivZ1) * srcInterp2 + oneDivZ1;

				double uAdd = (uCorr - uCur) / (yiScrCeil1 - yiScrCeil2);
				double vAdd = (vCorr - vCur) / (yiScrCeil1 - yiScrCeil2);
				double zAdd = (zCorr - zCur) / (yiScrCeil1 - yiScrCeil2);

				for (int y = yiScrCeil2; y < yiScrCeil1; ++y) {
					unsigned char(&pixel)[3] = buf[y][columnN];
					double uPixel = uCur / zCur;
					double vPixel = vCur / zCur;
					int scale = 5;
					pixel[0] = (((int(uPixel * scale) + int(vPixel * scale)) % 2) * vPixel) * 255;
					pixel[1] = (((int(uPixel * scale) + int(vPixel * scale)) % 2) * vPixel) * 255;
					pixel[2] = (((int(uPixel * scale) + int(vPixel * scale)) % 2) * vPixel) * 255;
					uCur += uAdd;
					vCur += vAdd;
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
				double u1 = (double(edgeCurNext.uFloorCeil) - edgeCur.uFloorCeil) * interpEdge + edgeCur.uFloorCeil;
				double v1 = (double(edgeCurNext.vFloorCeil) - edgeCur.vFloorCeil) * interpEdge + edgeCur.vFloorCeil;
				double u1DivZ = u1 / zSliceCur;
				double v1DivZ = v1 / zSliceCur;
				double oneDivZ1 = 1 / zSliceCur;

				const Edge& edgeComeFromNext = poly.edges[(edgeNComeFrom + 1) % vertsInPoly];
				double u2 = (double(edgeComeFrom.uFloorCeil) - edgeComeFromNext.uFloorCeil) * interpEdgePrev + edgeComeFromNext.uFloorCeil;
				double v2 = (double(edgeComeFrom.vFloorCeil) - edgeComeFromNext.vFloorCeil) * interpEdgePrev + edgeComeFromNext.vFloorCeil;
				double u2DivZ = u2 / zSlicePrev;
				double v2DivZ = v2 / zSlicePrev;
				double oneDivZ2 = 1 / zSlicePrev;

				double srcInterp1 = ((yiScrFloor1 + 0.5) - yScrFloor1) / (yScrFloor2 - yScrFloor1);
				double uCur = (u2DivZ - u1DivZ) * srcInterp1 + u1DivZ;
				double vCur = (v2DivZ - v1DivZ) * srcInterp1 + v1DivZ;
				double zCur = (oneDivZ2 - oneDivZ1) * srcInterp1 + oneDivZ1;

				double srcInterp2 = (yiScrFloor2 + 0.5 - yScrFloor1) / (yScrFloor2 - yScrFloor1);
				double uCorr = (u2DivZ - u1DivZ) * srcInterp2 + u1DivZ;
				double vCorr = (v2DivZ - v1DivZ) * srcInterp2 + v1DivZ;
				double zCorr = (oneDivZ2 - oneDivZ1) * srcInterp2 + oneDivZ1;

				double uAdd = (uCorr - uCur) / (yiScrFloor2 - yiScrFloor1);
				double vAdd = (vCorr - vCur) / (yiScrFloor2 - yiScrFloor1);
				double zAdd = (zCorr - zCur) / (yiScrFloor2 - yiScrFloor1);

				for (int y = yiScrFloor1; y < yiScrFloor2; ++y) {
					unsigned char(&pixel)[3] = buf[y][columnN];
					double uPixel = uCur / zCur;
					double vPixel = vCur / zCur;
					int scale = 5;
					pixel[0] = (((int(uPixel * scale) + int(vPixel * scale)) % 2) * vPixel) * 255;
					pixel[1] = (((int(uPixel * scale) + int(vPixel * scale)) % 2) * vPixel) * 255;
					pixel[2] = (((int(uPixel * scale) + int(vPixel * scale)) % 2) * vPixel) * 255;
					uCur += uAdd;
					vCur += vAdd;
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


m1: 	zSlicePrev = zSliceCur;
		interpEdgePrev = interpEdge;
		edgeNComeFrom = poly.edges[edgeWithMaxZ].adjEdgeN;
		curPolyN = poly.edges[edgeWithMaxZ].adjPolyN;
		if (curPolyN == -1) {
			// Рисуем пустоту (где уже нет лабиринта)
			for (int y = lastDrawedY1+1; y < lastDrawedY2; ++y) {
				unsigned char(&pixel)[3] = buf[y][columnN];
				pixel[0] = 60;
				pixel[1] = 60;
				pixel[2] = 60;
			}
			break;                                   
		}
	}
}

// -------------------------------------------------------------------

void Draw(HWND hWnd)
{
	horCamlAngleRad = horizontalAngle / 180.0 * 3.14159265359;
	dz = 1.0 / tan(horCamlAngleRad / 2.0);
	kProj = bufSizeX / 2 / tan(horCamlAngleRad / 2);
	startingPoly = 1;
	zCam += 0.0003f;
	//alCam += 0.0001f;

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



//Проблема! При растеризации первого полигона у нас нет edgeNComeFrom и других параметров с предыдущего сечения ребра, а они нужны для мэппинга
