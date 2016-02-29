// Aseprite Pen Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pen/pen.h"

#ifdef _WIN32
  #include "pen/pen_win.h"
#else
  #include "pen/pen_none.h"
#endif

namespace pen {

PenAPI::PenAPI(void* nativeDisplayHandle)
  : m_impl(new Impl(nativeDisplayHandle))
{
}

PenAPI::~PenAPI()
{
  delete m_impl;
}

} // namespace pen
