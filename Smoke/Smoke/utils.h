

const float PI = 3.14159f;

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

