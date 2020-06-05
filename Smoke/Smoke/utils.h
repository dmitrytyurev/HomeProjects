
//--------------------------------------------------------------------------------------------

const float PI = 3.14159f;

//--------------------------------------------------------------------------------------------

void _cdecl exit_msg(const char *text, ...);

//--------------------------------------------------------------------------------------------

inline float distSq(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	float dz = z1 - z2;
	return dx * dx + dy * dy + dz * dz;
}

//--------------------------------------------------------------------------------------------

inline float calcDist(float x1, float y1, float x2, float y2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	return sqrt(dx * dx + dy * dy);
}

//--------------------------------------------------------------------------------------------

inline float calcDist(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	float dz = z1 - z2;
	return sqrt(dx * dx + dy * dy + dz * dz);
}

//--------------------------------------------------------------------------------------------

inline int rand(int min, int max)
{
	return rand() * (max - min + 1) / (RAND_MAX + 1) + min;
}

//--------------------------------------------------------------------------------------------

inline float  randf(float min, float max)
{
	return rand() * (max - min) / RAND_MAX + min;
}

//--------------------------------------------------------------------------------------------

inline float normalize(float& x, float& y, float& z, bool& succes)
{
	float dist = sqrt(x*x + y*y + z*z);
	if (dist < 0.0001f) {
		succes = false;
		return dist;
	}
	succes = true;
	x /= dist;
	y /= dist;
	z /= dist;
	return dist;
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

//--------------------------------------------------------------------------------------------

void log(const std::string& str);

//--------------------------------------------------------------------------------------------

inline void turnPoint(float x, float y, double angle, float& newX, float& newY)
{
	double s = sin(angle);
	double c = cos(angle);
	newX = (float)(x * c - y * s);
	newY = (float)(x * s + y * c);
}

//--------------------------------------------------------------------------------------------

inline void turnPointCenter(float x, float y, double angle, float xCenter, float yCenter, float& newX, float& newY)
{
	float xM = x - xCenter;
	float yM = y - yCenter;
	float xRes = 0;
	float yRes = 0;
	turnPoint(xM, yM, angle, xRes, yRes);
	newX = xRes + xCenter;
	newY = yRes + yCenter;
}
