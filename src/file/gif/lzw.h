#ifndef FILE_GIF_LZW_H_INCLUDED
#define FILE_GIF_LZW_H_INCLUDED

#include <stdio.h>

int LZW_decode (FILE *file, void (*write_pixel)(int pos, int code, unsigned char *data), unsigned char *data);
void LZW_encode (FILE *file, int (*read_pixel)(int pos, unsigned char *data), int size, unsigned char *data);

#endif
