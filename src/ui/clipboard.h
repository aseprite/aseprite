// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_CLIPBOARD_H_INCLUDED
#define UI_CLIPBOARD_H_INCLUDED

#include "ui/base.h"

namespace ui {
  namespace clipboard {

    const char* get_text();
    void set_text(const char* text);

  } // namespace clipboard
} // namespace ui

#endif // ASE_JINETE_JCLIPBOARD_H
