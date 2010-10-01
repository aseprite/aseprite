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

Stock::Stock(int imgtype)
  : GfxObj(GFXOBJ_STOCK)
{
  m_imgtype = imgtype;

  // Image with index=0 is always NULL.
  m_image.push_back(NULL);
}

Stock::Stock(const Stock& stock)
  : GfxObj(stock)
{
  m_imgtype = stock.getImgType();

  try {
    for (int i=0; i<stock.size(); ++i) {
      if (!stock.getImage(i))
	addImage(NULL);
      else {
	Image* image_copy = image_new_copy(stock.getImage(i));
	addImage(image_copy);
      }
    }
  }
  catch (...) {
    for (int i=0; i<size(); ++i) {
      if (getImage(i))
	delete getImage(i);
    }
    throw;
  }

  ASSERT(size() == stock.size());
}

Stock::~Stock()
{
  for (int i=0; i<size(); ++i) {
    if (getImage(i))
      delete getImage(i);
  }
}

int Stock::getImgType() const
{
  return m_imgtype;
}

void Stock::setImgType(int imgtype)
{
  m_imgtype = imgtype;
}

Image* Stock::getImage(int index) const
{
  ASSERT((index >= 0) && (index < size()));

  return m_image[index];
}

int Stock::addImage(Image* image)
{
  int i = m_image.size();
  m_image.resize(m_image.size()+1);
  m_image[i] = image;
  return i;
}

void Stock::removeImage(Image* image)
{
  for (int i=0; i<size(); i++)
    if (m_image[i] == image) {
      m_image[i] = NULL;
      return;
    }

  ASSERT(false && "The specified image was not found");
}

void Stock::replaceImage(int index, Image* image)
{
  ASSERT((index > 0) && (index < size()));
  m_image[index] = image;
}
