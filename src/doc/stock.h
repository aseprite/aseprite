// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_STOCK_H_INCLUDED
#define DOC_STOCK_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/object.h"
#include "doc/pixel_format.h"

#include <vector>

namespace doc {

  class Image;
  class Sprite;

  typedef std::vector<Image*> ImagesList;

  class Stock : public Object {
  public:
    Stock(Sprite* sprite, PixelFormat format);
    virtual ~Stock();

    Sprite* sprite() const;

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

  private:
    void fixupImage(Image* image);

    PixelFormat m_format; // Type of images (all images in the stock must be of this type).
    ImagesList m_image;   // The images-array where the images are.
    Sprite* m_sprite;

    Stock();
    DISABLE_COPYING(Stock);
  };

} // namespace doc

#endif
