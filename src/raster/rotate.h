/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef RASTER_ROTATE_H_INCLUDED
#define RASTER_ROTATE_H_INCLUDED

class Image;

void image_scale(Image* dst, Image* src,
		 int x, int y, int w, int h);

void image_rotate(Image* dst, Image* src,
		  int x, int y, int w, int h,
		  int cx, int cy, double angle);

void image_parallelogram(Image* bmp, Image* sprite,
			 int x1, int y1, int x2, int y2,
			 int x3, int y3, int x4, int y4);

#endif
