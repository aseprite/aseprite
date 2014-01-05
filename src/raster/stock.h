/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef RASTER_STOCK_H_INCLUDED
#define RASTER_STOCK_H_INCLUDED

#include "raster/object.h"
#include "raster/pixel_format.h"

#include <vector>

namespace raster {

  class Image;

  typedef std::vector<Image*> ImagesList;

  class Stock : public Object {
  public:
    Stock(PixelFormat format);
    Stock(const Stock& stock);
    virtual ~Stock();

    PixelFormat getPixelFormat() const;
    void setPixelFormat(PixelFormat format);

    // Returns the number of image in the stock.
    int size() const {
      return m_image.size();
    }

    // Returns the image in the "index" position
    Image* getImage(int index) const;

    // Adds a new image in the stock resizing the images-array. Returns
    // the index/position in the stock (this index can be used with the
    // Stock::getImage() function).
    int addImage(Image* image);

    // Removes a image from the stock, it doesn't resize the stock.
    void removeImage(Image* image);

    // Replaces the image in the stock in the "index" position with the
    // new "image"; you must delete the old image before, e.g:
    //
    //   Image* old_image = stock->getImage(index);
    //   if (old_image)
    //     delete old_image;
    //   stock->replaceImage(index, new_image);
    //
    void replaceImage(int index, Image* image);

    //private: TODO uncomment this line
    PixelFormat m_format; // Type of images (all images in the stock must be of this type).
    ImagesList m_image;   // The images-array where the images are.
  };

} // namespace raster

#endif
