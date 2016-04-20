// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_PEN_H_INCLUDED
#define SHE_WIN_PEN_H_INCLUDED
#pragma once

#include "base/dll.h"

#include <windows.h>
#include "wacom/wintab.h"

namespace she {

  class PenAPI {
  public:
    PenAPI();
    ~PenAPI();

    HCTX attachDisplay(HWND hwnd);
    void detachDisplay(HCTX ctx);

  private:
    bool loadWintab();

    base::dll m_wintabLib;
  };

} // namespace she

#endif
