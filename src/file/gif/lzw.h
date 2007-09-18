#ifndef _LZW_H_
#define _LZW_H_
#include <allegro/file.h>
int LZW_decode (PACKFILE *file, void (*write_pixel)(int pos, int code, unsigned char *data), unsigned char *data);
void LZW_encode (PACKFILE *file, int (*read_pixel)(int pos, unsigned char *data), int size, unsigned char *data);
#endif
