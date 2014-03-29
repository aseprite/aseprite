// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CLIPBOARD_H_INCLUDED
#define UI_CLIPBOARD_H_INCLUDED
#pragma once

#include "ui/base.h"

namespace ui {
  namespace clipboard {

    const char* get_text();
    void set_text(const char* text);

  } // namespace clipboard
} // namespace ui

#endif
