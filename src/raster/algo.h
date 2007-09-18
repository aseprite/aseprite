/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#ifndef RASTER_ALGO_H
#define RASTER_ALGO_H

struct Dirty;
struct Image;

typedef void (*AlgoPixel) (int x, int y, void *data);
typedef void (*AlgoHLine) (int x1, int y, int x2, void *data);
typedef void (*AlgoLine) (int x1, int y1, int x2, int y2, void *data);

void algo_dirty (struct Dirty *dirty, void *data, AlgoHLine proc);

void algo_line (int x1, int y1, int x2, int y2, void *data, AlgoPixel proc);
void algo_ellipse (int x1, int y1, int x2, int y2, void *data, AlgoPixel proc);
void algo_ellipsefill (int x1, int y1, int x2, int y2, void *data, AlgoHLine proc);

void algo_spline (double x0, double y0, double x1, double y1,
		  double x2, double y2, double x3, double y3,
		  void *data, AlgoLine proc);

double algo_spline_get_y (double x0, double y0, double x1, double y1,
			  double x2, double y2, double x3, double y3,
			  double x);

double algo_spline_get_tan (double x0, double y0, double x1, double y1,
			    double x2, double y2, double x3, double y3,
			    double in_x);

void algo_floodfill (struct Image *image, int x, int y, void *data, AlgoHLine proc);

#endif /* RASTER_ALGO_H */
