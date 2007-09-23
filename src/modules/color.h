/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MODULES_COLOR_H
#define MODULES_COLOR_H

#define DEFAULT_FG   "rgb{0,0,0,255}"
#define DEFAULT_BG   "rgb{255,255,255,255}"

struct BITMAP;
struct Image;

enum {
  COLOR_TYPE_MASK,
  COLOR_TYPE_RGB,
  COLOR_TYPE_GRAY,
  COLOR_TYPE_INDEX,
};

int init_module_color(void);
void exit_module_color(void);

const char *get_fg_color(void);
const char *get_bg_color(void);
void set_fg_color(const char *string);
void set_bg_color(const char *string);

int color_type(const char *color);
char *color_mask(void);
char *color_rgb(int r, int g, int b, int a);
char *color_gray(int g, int a);
char *color_index(int index);
int color_get_black(int imgtype, const char *color);
int color_get_red(int imgtype, const char *color);
int color_get_green(int imgtype, const char *color);
int color_get_blue(int imgtype, const char *color);
int color_get_index(int imgtype, const char *color);
int color_get_alpha(int imgtype, const char *color);
void color_set_alpha(char **color, int alpha);
char *color_from_image(int imgtype, int c);

int blackandwhite(int r, int g, int b);
int blackandwhite_neg(int r, int g, int b);

int get_color_for_allegro(int depth, const char *color);
int get_color_for_image(int imgtype, const char *color);
char *image_getpixel_color(struct Image *image, int x, int y);
void color_to_formalstring(int imgtype, const char *color, char *buf,
			   int size, int long_format);
void draw_color(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
		int imgtype, const char *color);

#endif /* MODULES_COLOR_H */

