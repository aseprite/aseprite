// ASEPRITE gfx library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GFX_FWD_H_INCLUDED
#define GFX_FWD_H_INCLUDED

namespace gfx {

template<typename T> class BorderT;
template<typename T> class PointT;
template<typename T> class RectT;
template<typename T> class SizeT;

typedef BorderT<int> Border;
typedef PointT<int> Point;
typedef RectT<int> Rect;
typedef SizeT<int> Size;

} // namespace gfx

#endif
