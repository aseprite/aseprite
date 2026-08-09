// Aseprite Document Library
// Copyright (c) 2025 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_IO_H_INCLUDED
#define DOC_IMAGE_IO_H_INCLUDED
#pragma once

#include <iosfwd>

namespace doc {

class CancelIO;
class Image;

bool write_image(std::ostream& os, const Image* image, CancelIO* cancel = nullptr);
bool write_image_pixels(std::ostream& os, const Image* image, CancelIO* cancel = nullptr);

Image* read_image(std::istream& is, bool setId = true);
void read_image_pixels(std::istream& is, Image* image);

} // namespace doc

#endif
