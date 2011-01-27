// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_GRAPHICS_H_INCLUDED
#define GUI_GRAPHICS_H_INCLUDED

struct BITMAP;

class Graphics
{
public:
  Graphics(BITMAP* bmp, int dx, int dy);
  ~Graphics();
  
private:
  BITMAP* m_bmp;
  int m_dx;
  int m_dy;
};

#endif
