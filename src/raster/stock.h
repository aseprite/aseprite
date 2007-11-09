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

#ifndef RASTER_STOCK_H
#define RASTER_STOCK_H

#include "raster/gfxobj.h"

struct Image;

typedef struct Stock Stock;

struct Stock
{
  GfxObj gfxobj;
  int imgtype;		/* type of images (all images in the stock
			   must be of this type) */
  int nimage;		/* how many images have this stock */
  struct Image **image;	/* the images-array where the images are */
  bool ref;		/* this is a reference (non-original) stock? */
};

Stock *stock_new(int imgtype);
Stock *stock_new_ref(int imgtype);
Stock *stock_new_copy(const Stock *stock);
void stock_free(Stock *stock);

int stock_add_image(Stock *stock, struct Image *image);
void stock_remove_image(Stock *stock, struct Image *image);
void stock_replace_image(Stock *stock, int index, struct Image *image);

struct Image *stock_get_image(Stock *stock, int index);

#endif /* RASTER_STOCK_H */
