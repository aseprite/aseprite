// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/graphics.h"

Graphics::Graphics(BITMAP* bmp, int dx, int dy)
  : m_bmp(bmp)
  , m_dx(dx)
  , m_dy(dy)
{
}

Graphics::~Graphics()
{
}
