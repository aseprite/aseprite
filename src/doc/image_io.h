// Aseprite Document Library
// Copyright (c) 2025-present Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_IO_H_INCLUDED
#define DOC_IMAGE_IO_H_INCLUDED
#pragma once

#include "doc/io.h"

namespace doc {

class Image;

bool write_image(std::ostream& os, const Image* image, CancelIO* cancel = nullptr);
bool write_image_pixels(std::ostream& os, const Image* image, CancelIO* cancel = nullptr);

Image* read_image(std::istream& is, const IdMapperIO& mapper);
void read_image_pixels(std::istream& is, Image* image);

void copy_image_pixels(std::istream& is, std::ostream& os);

} // namespace doc

#endif
