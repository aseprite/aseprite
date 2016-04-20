// Aseprite Pen Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/debug.h"
#include "base/dll.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/path.h"
#include "base/string.h"

#include <windows.h>
#include "wacom/wintab.h"

typedef UINT (API* WTInfoW_Func)(UINT, UINT, LPVOID);
typedef HCTX (API* WTOpenW_Func)(HWND, LPLOGCONTEXTW, BOOL);
typedef BOOL (API* WTClose_Func)(HCTX);

class pen::PenAPI::Impl {
public:
  Impl(void* nativeDisplayHandle) : m_ctx(nullptr) {
    m_wintabLib = base::load_dll("wintab32.dll");
    if (!m_wintabLib)
      return;

    auto WTInfoW = base::get_dll_proc<WTInfoW_Func>(m_wintabLib, "WTInfoW");
    auto WTOpenW = base::get_dll_proc<WTOpenW_Func>(m_wintabLib, "WTOpenW");
    if (!WTInfoW || !WTOpenW)
      return;

    LOGCONTEXTW logctx;
    memset(&logctx, 0, sizeof(LOGCONTEXTW));
    logctx.lcOptions |= CXO_SYSTEM;
    UINT infoRes = WTInfoW(WTI_DEFSYSCTX, 0, &logctx);
    ASSERT(infoRes == sizeof(LOGCONTEXTW));
    ASSERT(logctx.lcOptions & CXO_SYSTEM);

    logctx.lcOptions |= CXO_SYSTEM;
    m_ctx = WTOpenW((HWND)nativeDisplayHandle, &logctx, TRUE);
    if (!m_ctx) {
      LOG("Pen library is not initialized...\n");
      return;
    }

    LOG("Pen initialized...\n");
  }

  ~Impl() {
    if (!m_wintabLib)
      return;

    if (m_ctx) {
      auto WTClose = base::get_dll_proc<WTClose_Func>(m_wintabLib, "WTClose");
      if (WTClose) {
        LOG("Pen shutdown...\n");
        WTClose(m_ctx);
      }
    }

    base::unload_dll(m_wintabLib);
  }

private:
  base::dll m_wintabLib;
  HCTX m_ctx;
};
