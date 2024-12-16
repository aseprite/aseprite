// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_ROTSPRITE_H_INCLUDED
#define DOC_ALGORITHM_ROTSPRITE_H_INCLUDED
#pragma once

namespace doc {
class Image;

namespace algorithm {

void rotsprite_image(Image* dst,
                     const Image* src,
                     const Image* mask,
                     int x1,
                     int y1,
                     int x2,
                     int y2,
                     int x3,
                     int y3,
                     int x4,
                     int y4);

} // namespace algorithm
} // namespace doc

#endif
