#pragma once

	void give_bmp_size(const char* fname, int* px, int* py, int* bits);

	// Буфер по 3 байта на пиксел (BGR)(порядок строк исправляется автоматически, паддинг убирается тоже)
	void read_bmp24(const char* fileName, unsigned char* pixels);

	void save_bmp24(const char *name, int sizeX, int sizeY, const char *data);

	void save_bmp32(const char *name, int sizeX, int sizeY, const char *data);
