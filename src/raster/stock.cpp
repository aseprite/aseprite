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

#include "config.h"

#include <string.h>

#include "raster/image.h"
#include "raster/stock.h"

//////////////////////////////////////////////////////////////////////


Stock::Stock(int imgtype)
  : GfxObj(GFXOBJ_STOCK)
{
  this->imgtype = imgtype;
  this->nimage = 0;
  this->image = NULL;

  stock_add_image(this, NULL); /* image 0 is NULL */
}

Stock::Stock(const Stock& stock)
  : GfxObj(stock)
{
  this->imgtype = stock.imgtype;
  this->nimage = 0;
  this->image = NULL;

  for (int c=0; c<stock.nimage; ++c) {
    if (!stock.image[c])
      stock_add_image(this, NULL);
    else {
      Image* image_copy = image_new_copy(stock.image[c]);
      // TODO try/catch remove created images
      stock_add_image(this, image_copy);
    }
  }

  ASSERT(this->nimage == stock.nimage);
}

Stock::~Stock()
{
  for (int i=0; i<this->nimage; i++) {
    if (this->image[i])
      image_free(this->image[i]);
  }
  jfree(this->image);
}

//////////////////////////////////////////////////////////////////////


/**
 * Creates a new stock, the "imgtype" argument indicates that this
 * stock will contain images of that type.
 */
Stock* stock_new(int imgtype)
{
  return new Stock(imgtype);
}

/**
 * Creates a new copy of "stock"; the new copy will have copies of all
 * images.
 */
Stock* stock_new_copy(const Stock* stock)
{
  ASSERT(stock);
  return new Stock(*stock);
}

/**
 * Destroys the stock.
 */
void stock_free(Stock* stock)
{
  ASSERT(stock);
  delete stock;
}

/**
 * Adds a new image in the stock resizing the images-array.
 */
int stock_add_image(Stock* stock, Image* image)
{
  int i = stock->nimage++;

  stock->image = (Image**)jrealloc(stock->image, sizeof(Image*) * stock->nimage);
  if (!stock->image)
    return -1;

  stock->image[i] = image;
  return i;
}

/* removes a image from the stock, it doesn't resize the stock */
void stock_remove_image(Stock* stock, Image* image)
{
  int i;

  for (i=0; i<stock->nimage; i++)
    if (stock->image[i] == image) {
      stock->image[i] = NULL;
      break;
    }
}

/**
 * Replaces the image in the stock in the "index" position with the
 * new "image"; you must free the old image before, e.g:
 * @code
 *   Image* old_image = stock_get_image(stock, index);
 *   if (old_image)
 *     image_free(old_image);
 *   stock_replace_image(stock, index, new_image);
 * @endcode
 */
void stock_replace_image(Stock* stock, int index, Image* image)
{
  /* the zero index can't be changed */
  if ((index > 0) && (index < stock->nimage))
    stock->image[index] = image;
}

/**
 * Returns the image in the "index" position
 */
Image* stock_get_image(Stock* stock, int index)
{
  return ((index >= 0) && (index < stock->nimage)) ? stock->image[index]: NULL;
}

/**
 * Returns the image in the "index" position
 */
const Image* stock_get_image(const Stock* stock, int index)
{
  return ((index >= 0) && (index < stock->nimage)) ? stock->image[index]: NULL;
}

