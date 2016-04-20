// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/win/pen.h"

#include "base/debug.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/path.h"
#include "base/string.h"

typedef UINT (API* WTInfoW_Func)(UINT, UINT, LPVOID);
typedef HCTX (API* WTOpenW_Func)(HWND, LPLOGCONTEXTW, BOOL);
typedef BOOL (API* WTClose_Func)(HCTX);

namespace she {

static WTInfoW_Func WTInfo;
static WTOpenW_Func WTOpen;
static WTClose_Func WTClose;

PenAPI::PenAPI()
  : m_wintabLib(nullptr)
{
}

PenAPI::~PenAPI()
{
  if (!m_wintabLib)
    return;

  base::unload_dll(m_wintabLib);
  m_wintabLib = nullptr;
}

HCTX PenAPI::attachDisplay(HWND hwnd)
{
  if (!m_wintabLib && !loadWintab())
    return nullptr;

  LOGCONTEXTW logctx;
  memset(&logctx, 0, sizeof(LOGCONTEXTW));
  logctx.lcOptions |= CXO_SYSTEM;
  UINT infoRes = WTInfo(WTI_DEFSYSCTX, 0, &logctx);
  ASSERT(infoRes == sizeof(LOGCONTEXTW));
  ASSERT(logctx.lcOptions & CXO_SYSTEM);
  logctx.lcOptions |= CXO_SYSTEM;
  HCTX ctx = WTOpen(hwnd, &logctx, TRUE);
  if (!ctx) {
    LOG("Error attaching pen to display\n");
    return nullptr;
  }

  LOG("Pen attached to display\n");
  return ctx;
}

void PenAPI::detachDisplay(HCTX ctx)
{
  if (ctx) {
    ASSERT(m_wintabLib);
    LOG("Pen detached from window\n");
    WTClose(ctx);
  }
}

bool PenAPI::loadWintab()
{
  ASSERT(!m_wintabLib);

  m_wintabLib = base::load_dll("wintab32.dll");
  if (!m_wintabLib) {
    LOG("wintab32.dll is not present\n");
    return false;
  }

  WTInfo = base::get_dll_proc<WTInfoW_Func>(m_wintabLib, "WTInfoW");
  WTOpen = base::get_dll_proc<WTOpenW_Func>(m_wintabLib, "WTOpenW");
  WTClose = base::get_dll_proc<WTClose_Func>(m_wintabLib, "WTClose");
  if (!WTInfo || !WTOpen || !WTClose) {
    LOG("wintab32.dll does not contain all required functions\n");
    return false;
  }

  LOG("Pen initialized\n");
  return true;
}

} // namespace she
