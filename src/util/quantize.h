/* ASE - Allegro Sprite Editor
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

#ifndef UTIL_QUANTIZE_H
#define UTIL_QUANTIZE_H

#include <allegro/color.h>

struct Sprite;
struct Stock;

void sprite_quantize (struct Sprite *sprite);
void sprite_quantize_ex (struct Sprite *sprite, struct RGB *palette);

int quantize_bitmaps1 (struct Stock *stock, struct RGB *pal, int *bmp_i, int fill_other);
/* int quantize_bitmaps2 (struct Stock *stock, struct RGB *pal); */

#endif /* UTIL_QUANTIZE_H */

