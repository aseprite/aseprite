// Aseprite Document Library
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
Image* read_image(std::istream& is, bool setId = true);

} // namespace doc

#endif
