/*
 * Written 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef _FLI_H
#define _FLI_H

/** structures */

typedef struct _fli_header {
	unsigned long filesize;
	unsigned short magic;
	unsigned short frames;
	unsigned short width;
	unsigned short height;
	unsigned short depth;
	unsigned short flags;
	unsigned long speed;
	unsigned long created;
	unsigned long creator;
	unsigned long updated;
	unsigned short aspect_x, aspect_y;
	unsigned long oframe1, oframe2;
} s_fli_header;

typedef struct _fli_frame {
	unsigned long size;
	unsigned short magic;
	unsigned short chunks;
} s_fli_frame;

typedef struct _fli_chunk {
	unsigned long size;
	unsigned short magic;
	unsigned char *data;
} s_fli_chunk;

/** chunk magics */
#define HEADER_FLI	0xAF11
#define HEADER_FLC	0xAF12
#define FRAME		0xF1FA

/** codec magics */
#define FLI_COLOR	11
#define FLI_BLACK	13
#define FLI_BRUN	15
#define FLI_COPY	16
#define FLI_LC		12
#define FLI_LC_2	7
#define FLI_COLOR_2	4
#define FLI_MINI	18

/** codec masks */
#define W_COLOR		0x0001
#define W_BLACK		0x0002
#define W_BRUN		0x0004
#define W_COPY		0x0008
#define W_LC		0x0010
#define W_LC_2		0x0020
#define W_COLOR_2	0x0040
#define W_MINI		0x0080
#define W_ALL		0xFFFF

/** functions */
void fli_read_header(FILE *f, s_fli_header *fli_header);
void fli_read_frame(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *old_cmap, unsigned char *framebuf, unsigned char *cmap);

void fli_read_color(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap); 
void fli_read_color_2(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap); 
void fli_read_black(FILE *f, s_fli_header *fli_header, unsigned char *framebuf); 
void fli_read_brun(FILE *f, s_fli_header *fli_header, unsigned char *framebuf); 
void fli_read_copy(FILE *f, s_fli_header *fli_header, unsigned char *framebuf); 
void fli_read_lc(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf); 
void fli_read_lc_2(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf);

void fli_write_header(FILE *f, s_fli_header *fli_header);
void fli_write_frame(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *old_cmap, unsigned char *framebuf, unsigned char *cmap, unsigned short codec_mask);

int fli_write_color(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap); 
int fli_write_color_2(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap); 
void fli_write_black(FILE *f, s_fli_header *fli_header, unsigned char *framebuf); 
void fli_write_brun(FILE *f, s_fli_header *fli_header, unsigned char *framebuf); 
void fli_write_copy(FILE *f, s_fli_header *fli_header, unsigned char *framebuf); 
void fli_write_lc(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf); 
void fli_write_lc_2(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf);

#endif
