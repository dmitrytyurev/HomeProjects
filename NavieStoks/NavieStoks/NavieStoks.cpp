// NavieStoks.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <stdarg.h>
#include <iostream>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <vector>

#include "bmp.h"


void _cdecl exit_msg(const char *text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	printf("%s", tmpStr);
	exit(1);
}

//--------------------------------------------------------------------------------------------

inline std::string digit5intFormat(int n)
{
	char number[6];
	number[0] = (n % 100000) / 10000 + '0';
	number[1] = (n % 10000) / 1000 + '0';
	number[2] = (n % 1000) / 100 + '0';
	number[3] = (n % 100) / 10 + '0';
	number[4] = (n % 10) + '0';
	number[5] = 0;
	return number;
}

using uint8_t = unsigned char;

struct Vec2
{
	float x = 0.0, y = 0.0;

	Vec2 operator-(Vec2 other)
	{
		Vec2 res;
		res.x = this->x - other.x;
		res.y = this->y - other.y;
		return res;
	}

	Vec2 operator+(Vec2 other)
	{
		Vec2 res;
		res.x = this->x + other.x;
		res.y = this->y + other.y;
		return res;
	}

	Vec2 operator*(float d)
	{
		Vec2 res;
		res.x = this->x * d;
		res.y = this->y * d;
		return res;
	}
};

struct Color3f
{
	float R = 0.0f;
	float G = 0.0f;
	float B = 0.0f;

	Color3f operator+ (Color3f other)
	{
		Color3f res;
		res.R = this->R + other.R;
		res.G = this->G + other.G;
		res.B = this->B + other.B;
		return res;
	}

	Color3f operator* (float d)
	{
		Color3f res;
		res.R = this->R * d;
		res.G = this->G * d;
		res.B = this->B * d;
		return res;
	}
};

struct Particle
{
	Vec2 u; // velocity
	Color3f color;
};

static struct Config
{
	float velocityDiffusion;
	float pressure;
	float vorticity;
	float colorDiffusion;
	float densityDiffusion;
	float forceScale;
	float bloomIntense;
	int radius;
	bool bloomEnabled;
} config;

static struct SystemConfig
{
	int velocityIterations = 20;
	int pressureIterations = 40;
	int xThreads = 80;
	int yThreads = 1;
} sConfig;

void setConfig(
	float vDiffusion = 0.8f,
	float pressure = 1.5f,
	float vorticity = 50.0f*0.55f,   // 0.7 много, 0.5 мало, 0.6 много
	float cDiffuion = 1.f,   //0.8f,
	float dDiffuion = 1.2f,
	float force = 5000.0f,
	float bloomIntense = 0.1f,
	int radius = 100,  // !!! 400
	bool bloom = true
)
{
	config.velocityDiffusion = vDiffusion;
	config.pressure = pressure;
	config.vorticity = vorticity;
	config.colorDiffusion = cDiffuion;
	config.densityDiffusion = dDiffuion;
	config.forceScale = force;
	config.bloomIntense = bloomIntense;
	config.radius = radius;
	config.bloomEnabled = bloom;
}

static const int colorArraySize = 7;
Color3f colorArray[colorArraySize];

static Particle* newField;
static Particle* oldField;
static uint8_t* colorField;
static size_t xSize, ySize;
static float* pressureOld;
static float* pressureNew;
static float* vorticityField;
static Color3f currentColor;
static float elapsedTime = 0.0f;
static float timeSincePress = 0.0f;

// inits all buffers, must be called before computeField function call
void cudaInit(size_t x, size_t y)
{
	setConfig();

	colorArray[0] = { 1.0f, 0.0f, 0.0f };
	colorArray[1] = { 0.0f, 1.0f, 0.0f };
	colorArray[2] = { 1.0f, 0.0f, 1.0f };
	colorArray[3] = { 1.0f, 1.0f, 0.0f };
	colorArray[4] = { 0.0f, 1.0f, 1.0f };
	colorArray[5] = { 1.0f, 0.0f, 1.0f };
	colorArray[6] = { 1.0f, 0.5f, 0.3f };

	int idx = rand() % colorArraySize;
	currentColor = colorArray[idx];

	xSize = x, ySize = y;

	colorField = (uint8_t*) malloc(xSize * ySize * 4 * sizeof(uint8_t));
	oldField = (Particle*) malloc(xSize * ySize * sizeof(Particle));
	newField = (Particle*) malloc(xSize * ySize * sizeof(Particle));
	pressureOld = (float*) malloc(xSize * ySize * sizeof(float));
	pressureNew = (float*) malloc(xSize * ySize * sizeof(float));
	vorticityField = (float*) malloc(xSize * ySize * sizeof(float));
}

// releases all buffers, must be called on program exit
void cudaExit()
{
	free(colorField);
	free(oldField);
	free(newField);
	free(pressureOld);
	free(pressureNew);
	free(vorticityField);
}

// interpolates quantity of grid cells
Particle interpolate(Vec2 v, Particle* field, size_t xSize, size_t ySize)
{
	float x1 = (int)v.x;
	float y1 = (int)v.y;
	float x2 = (int)v.x + 1;
	float y2 = (int)v.y + 1;
	Particle q1, q2, q3, q4;
#define CLAMP(val, minv, maxv) std::min(maxv, std::max(minv, val))
#define SET(Q, x, y) Q = field[int(CLAMP(y, 0.0f, ySize - 1.0f)) * xSize + int(CLAMP(x, 0.0f, xSize - 1.0f))]	
	SET(q1, x1, y1);
	SET(q2, x1, y2);
	SET(q3, x2, y1);
	SET(q4, x2, y2);
#undef SET
#undef CLAMP
	float t1 = (x2 - v.x) / (x2 - x1);
	float t2 = (v.x - x1) / (x2 - x1);
	Vec2 f1 = q1.u * t1 + q3.u * t2;
	Vec2 f2 = q2.u * t1 + q4.u * t2;
	Color3f C1 = q2.color * t1 + q4.color * t2;
	Color3f C2 = q2.color * t1 + q4.color * t2;
	float t3 = (y2 - v.y) / (y2 - y1);
	float t4 = (v.y - y1) / (y2 - y1);
	Particle res;
	res.u = f1 * t3 + f2 * t4;
	res.color = C1 * t3 + C2 * t4;
	return res;
}

// performs iteration of jacobi method on velocity grid field
Vec2 jacobiVelocity(Particle* field, size_t xSize, size_t ySize, Vec2 v, Vec2 B, float alpha, float beta)
{
	Vec2 vU = B * -1.0f, vD = B * -1.0f, vR = B * -1.0f, vL = B * -1.0f;
#define SET(U, x, y) if (x < xSize && x >= 0 && y < ySize && y >= 0) U = field[int(y) * xSize + int(x)].u
	SET(vU, v.x, v.y - 1);
	SET(vD, v.x, v.y + 1);
	SET(vL, v.x - 1, v.y);
	SET(vR, v.x + 1, v.y);
#undef SET
	v = (vU + vD + vL + vR + B * alpha) * (1.0f / beta);
	return v;
}

// performs iteration of jacobi method on pressure grid field
float jacobiPressure(float* pressureField, size_t xSize, size_t ySize, int x, int y, float B, float alpha, float beta)
{
	float C = pressureField[int(y) * xSize + int(x)];
	float xU = C, xD = C, xL = C, xR = C;
#define SET(P, x, y) if (x < xSize && x >= 0 && y < ySize && y >= 0) P = pressureField[int(y) * xSize + int(x)]
	SET(xU, x, y - 1);
	SET(xD, x, y + 1);
	SET(xL, x - 1, y);
	SET(xR, x + 1, y);
#undef SET
	float pressure = (xU + xD + xL + xR + alpha * B) * (1.0f / beta);
	return pressure;
}

// performs iteration of jacobi method on color grid field
Color3f jacobiColor(Particle* colorField, size_t xSize, size_t ySize, Vec2 pos, Color3f B, float alpha, float beta)
{
	Color3f xU, xD, xL, xR, res;
	int x = pos.x;
	int y = pos.y;
#define SET(P, x, y) if (x < xSize && x >= 0 && y < ySize && y >= 0) P = colorField[int(y) * xSize + int(x)]
	SET(xU, x, y - 1).color;
	SET(xD, x, y + 1).color;
	SET(xL, x - 1, y).color;
	SET(xR, x + 1, y).color;
#undef SET
	res = (xU + xD + xL + xR + B * alpha) * (1.0f / beta);
	return res;
}

// computes divergency of velocity field
float divergency(Particle* field, size_t xSize, size_t ySize, int x, int y)
{
	Particle& C = field[int(y) * xSize + int(x)];
	float x1 = -1 * C.u.x, x2 = -1 * C.u.x, y1 = -1 * C.u.y, y2 = -1 * C.u.y;
#define SET(P, x, y) if (x < xSize && x >= 0 && y < ySize && y >= 0) P = field[int(y) * xSize + int(x)]
	SET(x1, x + 1, y).u.x;
	SET(x2, x - 1, y).u.x;
	SET(y1, x, y + 1).u.y;
	SET(y2, x, y - 1).u.y;
#undef SET
	return (x1 - x2 + y1 - y2) * 0.5f;
}

// computes gradient of pressure field
Vec2 gradient(float* field, size_t xSize, size_t ySize, int x, int y)
{
	float C = field[y * xSize + x];
#define SET(P, x, y) if (x < xSize && x >= 0 && y < ySize && y >= 0) P = field[int(y) * xSize + int(x)]
	float x1 = C, x2 = C, y1 = C, y2 = C;
	SET(x1, x + 1, y);
	SET(x2, x - 1, y);
	SET(y1, x, y + 1);
	SET(y2, x, y - 1);
#undef SET
	Vec2 res = { (x1 - x2) * 0.5f, (y1 - y2) * 0.5f };
	return res;
}

// computes absolute value gradient of vorticity field
Vec2 absGradient(float* field, size_t xSize, size_t ySize, int x, int y)
{
	float C = field[int(y) * xSize + int(x)];
#define SET(P, x, y) if (x < xSize && x >= 0 && y < ySize && y >= 0) P = field[int(y) * xSize + int(x)]
	float x1 = C, x2 = C, y1 = C, y2 = C;
	SET(x1, x + 1, y);
	SET(x2, x - 1, y);
	SET(y1, x, y + 1);
	SET(y2, x, y - 1);
#undef SET
	Vec2 res = { (abs(x1) - abs(x2)) * 0.5f, (abs(y1) - abs(y2)) * 0.5f };
	return res;
}

// computes curl of velocity field
float curl(Particle* field, size_t xSize, size_t ySize, int x, int y)
{
	Vec2 C = field[int(y) * xSize + int(x)].u;
#define SET(P, x, y) if (x < xSize && x >= 0 && y < ySize && y >= 0) P = field[int(y) * xSize + int(x)]
	float x1 = -C.x, x2 = -C.x, y1 = -C.y, y2 = -C.y;
	SET(x1, x + 1, y).u.x;
	SET(x2, x - 1, y).u.x;
	SET(y1, x, y + 1).u.y;
	SET(y2, x, y - 1).u.y;
#undef SET
	float res = ((y1 - y2) - (x1 - x2)) * 0.5f;
	return res;
}

// adds quantity to particles using bilinear interpolation
 void advect(int x, int y, Particle* newField, Particle* oldField, size_t xSize, size_t ySize, float dDiffusion, float dt)
{
	float decay = 1.0f / (1.0f + dDiffusion * dt);
decay = 1;
	Vec2 pos = { x * 1.0f, y * 1.0f };
	Particle& Pold = oldField[y * xSize + x];
	// find new particle tracing where it came from
	Particle p = interpolate(pos - Pold.u * dt, oldField, xSize, ySize);
	p.u = p.u * decay;
	p.color.R = std::min(1.0f, pow(p.color.R, 1.0f) * decay);
	p.color.G = std::min(1.0f, pow(p.color.G, 1.0f) * decay);
	p.color.B = std::min(1.0f, pow(p.color.B, 1.0f) * decay);
	newField[y * xSize + x] = p;
}

// calculates color field diffusion
 void computeColor(int x, int y, Particle* newField, Particle* oldField, size_t xSize, size_t ySize, float cDiffusion, float dt)
{
	Vec2 pos = { x * 1.0f, y * 1.0f };
	Color3f c = oldField[y * xSize + x].color;
	float alpha = cDiffusion * cDiffusion / dt;
	float beta = 4.0f + alpha;
	// perfom one iteration of jacobi method (diffuse method should be called 20-50 times per cell)
	newField[y * xSize + x].color = jacobiColor(oldField, xSize, ySize, pos, c, alpha, beta);
}

// fills output image with corresponding color
 void paint(int x, int y, uint8_t* colorField, Particle* field, size_t xSize, size_t ySize)
{
	float R = field[y * xSize + x].color.R;
	float G = field[y * xSize + x].color.G;
	float B = field[y * xSize + x].color.B;

	colorField[4 * (y * xSize + x) + 0] = (uint8_t)std::min(255.0f, 255.0f * R);
	colorField[4 * (y * xSize + x) + 1] = (uint8_t)std::min(255.0f, 255.0f * G);
	colorField[4 * (y * xSize + x) + 2] = (uint8_t)std::min(255.0f, 255.0f * B);
	colorField[4 * (y * xSize + x) + 3] = 255;
}

// calculates nonzero divergency velocity field u
 void diffuse(int x, int y, Particle* newField, Particle* oldField, size_t xSize, size_t ySize, float vDiffusion, float dt)
{
	Vec2 pos = { x * 1.0f, y * 1.0f };
	Vec2 u = oldField[y * xSize + x].u;
	// perfoms one iteration of jacobi method (diffuse method should be called 20-50 times per cell)
	float alpha = vDiffusion * vDiffusion / dt;
	float beta = 4.0f + alpha;
	newField[y * xSize + x].u = jacobiVelocity(oldField, xSize, ySize, pos, u, alpha, beta);
}

// performs iteration of jacobi method on pressure field
 void computePressureImpl(int x, int y, Particle* field, size_t xSize, size_t ySize, float* pNew, float* pOld, float pressure, float dt)
{
	float div = divergency(field, xSize, ySize, x, y);

	float alpha = -1.0f * pressure * pressure;
	float beta = 4.0;
	pNew[y * xSize + x] = jacobiPressure(pOld, xSize, ySize, x, y, div, alpha, beta);
}

// projects pressure field on velocity field
 void project(int x, int y, Particle* newField, size_t xSize, size_t ySize, float* pField)
{
	Vec2& u = newField[y * xSize + x].u;
	u = u - gradient(pField, xSize, ySize, x, y);
}

// applies force and add color dye to the particle field
 void applyForce(int x, int y, Particle* field, size_t xSize, size_t ySize, Color3f color, Vec2 F, Vec2 pos, int r, float dt)
{
	float e = expf((-(powf(x - pos.x, 2) + powf(y - pos.y, 2))) / r);
	Vec2 uF = F * dt * e;
	Particle& p = field[y * xSize + x];
	p.u = p.u + uF;
	color = color * e + p.color;
	p.color.R = std::min(1.0f, color.R);
	p.color.G = std::min(1.0f, color.G);
	p.color.B = std::min(1.0f, color.B);
}

// computes vorticity field which should be passed to applyVorticity function
 void computeVorticity(int x, int y, float* vField, Particle* field, size_t xSize, size_t ySize)
{
	vField[y * xSize + x] = curl(field, xSize, ySize, x, y);
}

// applies vorticity to velocity field
 void applyVorticity(int x, int y, Particle* newField, Particle* oldField, float* vField, size_t xSize, size_t ySize, float vorticity, float dt)
{
	Particle& pOld = oldField[y * xSize + x];
	Particle& pNew = newField[y * xSize + x];

	Vec2 v = absGradient(vField, xSize, ySize, x, y);
	v.y *= -1.0f;

	float length = sqrtf(v.x * v.x + v.y * v.y) + 1e-5f;
	Vec2 vNorm = v * (1.0f / length);

	Vec2 vF = vNorm * vField[y * xSize + x] * vorticity;
	pNew = pOld;
	pNew.u = pNew.u + vF * dt;
}

// adds flashlight effect near the mouse position
 void applyBloom(int x, int y, uint8_t* colorField, size_t xSize, size_t ySize, int xpos, int ypos, float radius, float bloomIntense)
{
	int pos = 4 * (y * xSize + x);

	float e = bloomIntense * expf(-(powf(x - xpos, 2) + powf(y - ypos, 2)) / pow(radius, 2));

	uint8_t R = colorField[pos + 0];
	uint8_t G = colorField[pos + 1];
	uint8_t B = colorField[pos + 2];

	uint8_t maxval = std::max(R, std::max(G, B));

	colorField[pos + 0] = std::min(255.0f, R + maxval * e);
	colorField[pos + 1] = std::min(255.0f, G + maxval * e);
	colorField[pos + 2] = std::min(255.0f, B + maxval * e);
}

// performs several iterations over velocity and color fields
void computeDiffusion(float dt)
{
	// diffuse velocity and color
	for (int i = 0; i < sConfig.velocityIterations; i++)
	{
		for (int y=0; y<ySize; ++y) {
			for (int x = 0; x < xSize; ++x)	{
				diffuse(x, y, newField, oldField, xSize, ySize, config.velocityDiffusion, dt);
			}
		}
		for (int y = 0; y < ySize; ++y)	{
			for (int x = 0; x < xSize; ++x)	{
				computeColor(x, y, newField, oldField, xSize, ySize, config.colorDiffusion, dt);
			}
		}
		std::swap(newField, oldField);
	}
}

// performs several iterations over pressure field
void computePressure(float dt)
{
	for (int i = 0; i < sConfig.pressureIterations; i++)
	{
		for (int y = 0; y < ySize; ++y) {
			for (int x = 0; x < xSize; ++x) {
				computePressureImpl(x, y, oldField, xSize, ySize, pressureNew, pressureOld, config.pressure, dt);
			}
		}
		std::swap(pressureOld, pressureNew);
	}
}

// main function, calls vorticity -> diffusion -> force -> pressure -> project -> advect -> paint -> bloom
void computeField(uint8_t* result, float dt, int x1pos, int y1pos, int x2pos, int y2pos, bool isPressed)
{
	// curls and vortisity

	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			computeVorticity(x, y, vorticityField, oldField, xSize, ySize);
		}
	}
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			applyVorticity(x, y, newField, oldField, vorticityField, xSize, ySize, config.vorticity, dt);
		}
	}

	std::swap(oldField, newField);

	// diffuse velocity and color
	computeDiffusion(dt);

	// apply force
	if (isPressed)
	{
		timeSincePress = 0.0f;
		elapsedTime += dt;
		// apply gradient to color
		int roundT = int(elapsedTime) % colorArraySize;
		int ceilT = int((elapsedTime)+1) % colorArraySize;
		float w = elapsedTime - int(elapsedTime);
		currentColor = colorArray[roundT] * (1 - w) + colorArray[ceilT] * w;

		Vec2 F;
		float scale = config.forceScale;
		F.x = (x2pos - x1pos) * scale;
		F.y = (y2pos - y1pos) * scale;
//		F.x = (y1pos - y2pos) * scale;
//		F.y = -(x1pos - x2pos) * scale;
		Vec2 pos = { x2pos * 1.0f, y2pos * 1.0f };

		for (int y = 0; y < ySize; ++y) {
			for (int x = 0; x < xSize; ++x) {
				applyForce(x, y, oldField, xSize, ySize, currentColor, F, pos, config.radius, dt);
			}
		}
	}
	else
	{
		timeSincePress += dt;
	}

	// compute pressure
	computePressure(dt);

	// project
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			project(x, y, oldField, xSize, ySize, pressureOld);
		}
	}
	memset(pressureOld, 0, xSize * ySize * sizeof(float));

	// advect
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			advect(x, y, newField, oldField, xSize, ySize, config.densityDiffusion, dt);
		}
	}
	std::swap(newField, oldField);

	// paint image
	for (int y = 0; y < ySize; ++y) {
		for (int x = 0; x < xSize; ++x) {
			paint(x, y, colorField, oldField, xSize, ySize);
		}
	}

	// apply bloom in mouse pos
	if (config.bloomEnabled && timeSincePress < 5.0f)
	{
		for (int y = 0; y < ySize; ++y) {
			for (int x = 0; x < xSize; ++x) {
				applyBloom(x, y, colorField, xSize, ySize, x2pos, y2pos, config.radius, config.bloomIntense);
			}
		}
	}

	// copy image to cpu
	size_t size = xSize * ySize * 4 * sizeof(uint8_t);
	memcpy(result, colorField, size);
}


//SFML REQUIRED TO LAUNCH THIS CODE

#define SCALE 2
#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900
#define FIELD_WIDTH int(WINDOW_WIDTH / SCALE)
#define FIELD_HEIGHT int(WINDOW_HEIGHT / SCALE)


//void setConfig(float vDiffusion = 0.8f, float pressure = 1.5f, float vorticity = 50.0f, float cDiffuion = 0.8f,
//	float dDiffuion = 1.2f, float force = 1000.0f, float bloomIntense = 25000.0f, int radius = 100, bool bloom = true);
//void computeField(uint8_t* result, float dt, int x1pos, int y1pos, int x2pos, int y2pos, bool isPressed);
//void cudaInit(size_t xSize, size_t ySize);
//void cudaExit();

int main()
{
	cudaInit(FIELD_WIDTH, FIELD_HEIGHT);
	srand(time(NULL));

	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();

	std::vector<uint8_t> pixelBuffer;
	pixelBuffer.resize(FIELD_WIDTH * FIELD_HEIGHT * 4);
	int frame = 0;

	bool isPressed = true;
	int xMouse1 = 100;
	int yMouse1 = 100;
	int xMouse2 = 100;
	int yMouse2 = 100;
	while (true)
	{
		end = std::chrono::system_clock::now();
		std::chrono::duration<float> diff = end - start;
		start = end;

		isPressed = frame < 50;
		float dt = 0.02f;
		computeField(pixelBuffer.data(), dt, xMouse1, yMouse1, xMouse2, yMouse2, isPressed);
		std::string fname = "test" + digit5intFormat(frame) + ".bmp";
		save_bmp32(fname.c_str(), FIELD_WIDTH, FIELD_HEIGHT, (const char*)pixelBuffer.data());
		xMouse1 = xMouse2;
		yMouse1 = yMouse2;
		if (frame < 50)
			++yMouse2;
		else
			if (frame == 50) {
				xMouse2 = 150;
				yMouse2 = 210;
			}
			else {
				--xMouse2;
			}




		printf("frame %d done\n", frame);
		++frame;
	}
	cudaExit();
	return 0;
}

