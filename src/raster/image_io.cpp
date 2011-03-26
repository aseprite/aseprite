/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "raster/image_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "raster/image.h"

#include <iostream>

namespace raster {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Image data:
//
//    DWORD		image ID
//    BYTE		image type
//    WORD[2]		w, h
//    DWORD		mask color
//    for each line	("h" times)
//      for each pixel  ("w" times)
//	  BYTE[4]	for RGB images, or
//	  BYTE[2]	for Grayscale images, or
//	  BYTE		for Indexed images

void write_image(std::ostream& os, Image* image)
{
  write8(os, image->imgtype);		  // Imgtype
  write16(os, image->w);		  // Width
  write16(os, image->h);		  // Height
  write32(os, image->mask_color);	  // Mask color

  int size = image_line_size(image, image->w);
  for (int c=0; c<image->h; c++)
    os.write((char*)image->line[c], size);
}

Image* read_image(std::istream& is)
{
  int imgtype = read8(is);		// Imgtype
  int width = read16(is);		// Width
  int height = read16(is);		// Height
  uint32_t maskColor = read32(is);	// Mask color

  UniquePtr<Image> image(image_new(imgtype, width, height));
  int size = image_line_size(image, image->w);

  for (int c=0; c<image->h; c++)
    is.read((char*)image->line[c], size);

  image->mask_color = maskColor;
  return image.release();
}

}
