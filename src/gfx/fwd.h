// Aseprite Gfx Library
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_FWD_H_INCLUDED
#define GFX_FWD_H_INCLUDED
#pragma once

namespace gfx {

template<typename T> class BorderT;
template<typename T> class ClipT;
template<typename T> class PointT;
template<typename T> class RectT;
template<typename T> class SizeT;

typedef BorderT<int> Border;
typedef ClipT<int> Clip;
typedef PointT<int> Point;
typedef RectT<int> Rect;
typedef SizeT<int> Size;

class Region;

} // namespace gfx

#endif
