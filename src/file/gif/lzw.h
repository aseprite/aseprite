#ifndef _LZW_H_
#define _LZW_H_
#include <stdio.h>
int LZW_decode (FILE *file, void (*write_pixel)(int pos, int code, unsigned char *data), unsigned char *data);
void LZW_encode (FILE *file, int (*read_pixel)(int pos, unsigned char *data), int size, unsigned char *data);
#endif
