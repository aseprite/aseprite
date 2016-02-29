// Aseprite Pen Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef PEN_PEN_H_INCLUDED
#define PEN_PEN_H_INCLUDED
#pragma once

namespace pen {

class PenAPI {
public:
  PenAPI(void* nativeDisplayHandle);
  ~PenAPI();

private:
  class Impl;
  Impl* m_impl;
};

} // namespace pen

#endif
