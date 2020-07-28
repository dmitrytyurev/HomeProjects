#pragma once

  enum BMP_Y_FLIP
  {
    BMP_Y_NORMAL,
    BMP_Y_FLIPPED
  };

  void read_bmp24(const char* fileName, unsigned char* pixels);

  void give_bmp_size(const char *fname, int *px, int *py);
  void open_color_bmp(const char *fname);
  void close_color_bmp();
  void save_bmp24(const char *name, int sizeX, int sizeY, const char *data);
  void save_bmp32(const char *name, int sizeX, int sizeY, const char *data);
  unsigned char *give_pixel_adr(int x, int y, BMP_Y_FLIP flip);

