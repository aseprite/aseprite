// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_PEN_H_INCLUDED
#define SHE_WIN_PEN_H_INCLUDED
#pragma once

#include "base/dll.h"

#include <windows.h>
#include "wacom/wintab.h"

#define PACKETDATA (PK_CURSOR | PK_BUTTONS | PK_X | PK_Y | PK_NORMAL_PRESSURE)
#define PACKETMODE (PK_BUTTONS)
#include "wacom/pktdef.h"

namespace she {

  // Wintab API wrapper
  // Read http://www.wacomeng.com/windows/docs/Wintab_v140.htm for more information.
  class PenAPI {
  public:
    PenAPI();
    ~PenAPI();

    HCTX open(HWND hwnd);
    void close(HCTX ctx);
    bool packet(HCTX ctx, UINT serial, LPVOID packet);

  private:
    bool loadWintab();
    bool isBuggyDll();

    base::dll m_wintabLib;
  };

} // namespace she

#endif
