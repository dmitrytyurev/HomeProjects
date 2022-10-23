#include "Windows.h"
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include "bmp.h"
#include <fstream>

void _cdecl ExitMsg(const char *text, ...);


  //--------------------------------------------------------------------------------------------
  // Глобальные параметры
  //--------------------------------------------------------------------------------------------
#pragma warning(suppress : 4996)


    //--------------------------------------------------------------------------------------------

  int my_create(const char *fullFileName)
  {
	  int handle;

	  _sopen_s(&handle, fullFileName, _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, 0, 0);
	  if (handle != -1)
	  {
		  return handle;
	  }
	  return -1;
  }

  //--------------------------------------------------------------------------------------------

  int my_close(int handle)
  {
      int res = _close(handle);
	  if (res == 0)
	  {
		  return 0;
	  }
	  return -1;
  }

  //--------------------------------------------------------------------------------------------

  int my_write(int handle, const void *adr, int size)
  {
      int res = _write(handle, adr, size);
	  if (res != -1)
	  {
		  if (res != size)
		  {
			  return -1;
		  }
		  return res;
	  }
	  return -1;
  }

  //--------------------------------------------------------------------------------------------
  // По имени файла картинки вернуть её размеры и битность
  //--------------------------------------------------------------------------------------------

  void give_bmp_size(const char *fname, int* px, int *py, int *bits)
  {
    int hdl,res;
    WORD sign;

	_sopen_s(&hdl, fname, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
	if (-1 == hdl) ExitMsg("Can't open file %s", fname);
    res = _read(hdl, &sign, 2);
    if (-1 == res)
m1:    ExitMsg("Error reading file ", fname);
    if (sign != 0x4d42)  ExitMsg("not a valid BMP-file %s", fname);
    _lseek(hdl, 18, SEEK_SET);
    res = _read(hdl, px, 4);
    if (-1 == res) goto m1;
    _lseek(hdl, 22, SEEK_SET);
    res = _read(hdl, py, 4);
    if (-1 == res) goto m1;
    _lseek(hdl, 28, SEEK_SET);
    res = _read(hdl, bits, 4);
    if (-1 == res) goto m1;
    _close(hdl);
  }

  //--------------------------------------------------------------------------------------------

void read_bmp24(const char* fileName, unsigned char* pixels)
{
	FILE* f = nullptr;
	fopen_s(&f, fileName, "rb");
	if (!f)
		return;

	int xSize = 0;
	int ySize = 0;
	int res = fseek(f, 18, SEEK_SET);
	if (res) ExitMsg("error reading 1");
	res = (int)fread(&xSize, 1, 4, f);
	if (res != 4) ExitMsg("error reading 1");
	res = (int)fread(&ySize, 1, 4, f);
	if (res != 4) ExitMsg("error reading 1");

	int linePadd = (-3 * xSize) & 3;

	res = fseek(f, 54, SEEK_SET);
	if (res) ExitMsg("error reading 1");
	for (int i = 0; i < ySize; ++i) {
		res = (int)fread(pixels + (ySize-1-i)* 3 * xSize, 1, 3*xSize, f);
		if (res != 3 * xSize) ExitMsg("error reading 1");
		if (linePadd) {
			res = (int)fseek(f, linePadd, SEEK_CUR);
			if (res) ExitMsg("error reading 1");
		}

	}
	fclose(f);
}

  //--------------------------------------------------------------------------------------------

  int odd4(int n)
  {
    return (n + 3) & ~3;
  }

  //--------------------------------------------------------------------------------------------
  // Записывает переданный массив DWORD'ов, как BMP32
  //--------------------------------------------------------------------------------------------

  void save_bmp32(const char *name, int sizeX, int sizeY, const char *data)
  {
    char signature[]="BM";
    int i, hdl = my_create(name);
    int pitch = odd4(sizeX * 4);
    int size = odd4(54 + sizeY * pitch);
    int endLineSize = pitch - sizeX * 4;              // длина дописки в конец каждой строки (выравнивание на границу 4)
    int endFileSize = size - (54 + sizeY * pitch);      // длина дописки в конец файла (выравнивание на границу 4)

    my_write(hdl, signature, 2);
    my_write(hdl, &size, 4);
    size = 0;
    my_write(hdl, &size, 4);
    size = 54;
    my_write(hdl, &size, 4);
    size = 40;
    my_write(hdl, &size, 4);
    size = sizeX;
    my_write(hdl, &size, 4);
    size = sizeY;
    my_write(hdl, &size, 4);
    size = 1;
    my_write(hdl, &size, 2);
    size = 32;
    my_write(hdl, &size, 2);
    size = 0;
    my_write(hdl, &size, 4);
    my_write(hdl, &size, 4);
    size = 2834;
    my_write(hdl, &size, 4);
    my_write(hdl, &size, 4);
    size = 0;
    my_write(hdl, &size, 4);
    my_write(hdl, &size, 4);

    for (i=0; i<sizeY; i++)
    {
      my_write(hdl, data + sizeX*4 * (sizeY-1-i), sizeX*4);
      my_write(hdl, &size, endLineSize);
    }

    my_write(hdl, &size, endFileSize);
    my_close(hdl);
  }


 //--------------------------------------------------------------------------------------------
// Записывает переданный массив DWORD'ов, как BMP24
//--------------------------------------------------------------------------------------------

  void save_bmp24(const char *name, int sizeX, int sizeY, const char *data)
  {
	  FILE* f = nullptr;
	  fopen_s(&f, name, "wb");
	  if (!f)
		  return;

	  char signature[] = "BM";
	  int pitch = odd4(sizeX * 3);
	  int size = odd4(54 + sizeY * pitch);
	  int endLineSize = pitch - sizeX * 3;              // длина дописки в конец каждой строки (выравнивание на границу 4)
	  int endFileSize = size - (54 + sizeY * pitch);      // длина дописки в конец файла (выравнивание на границу 4)

	  fwrite(signature, 1, 2, f);
	  fwrite(&size, 1, 4, f);
	  size = 0;
	  fwrite(&size, 1, 4, f);
	  size = 54;
	  fwrite(&size, 1, 4, f);
	  size = 40;
	  fwrite(&size, 1, 4, f);
	  size = sizeX;
	  fwrite(&size, 1, 4, f);
	  size = sizeY;
	  fwrite(&size, 1, 4, f);
	  size = 1;
	  fwrite(&size, 1, 2, f);
	  size = 24;
	  fwrite(&size, 1, 2, f);
	  size = 0;
	  fwrite(&size, 1, 4, f);
	  fwrite(&size, 1, 4, f);
	  size = 2834;
	  fwrite(&size, 1, 4, f);
	  fwrite(&size, 1, 4, f);
	  size = 0;
	  fwrite(&size, 1, 4, f);
	  fwrite(&size, 1, 4, f);

	  for (int i = 0; i < sizeY; i++)
	  {
		  fwrite(data + sizeX * 3 * (sizeY - 1 - i), 1, sizeX * 3, f);
		  fwrite(&size, 1, endLineSize, f);
	  }

	  fwrite(&size, 1, endFileSize, f);
	  fclose(f);
  }

