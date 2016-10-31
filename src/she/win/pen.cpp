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

#include <iostream>

typedef UINT (API* WTInfoW_Func)(UINT, UINT, LPVOID);
typedef HCTX (API* WTOpenW_Func)(HWND, LPLOGCONTEXTW, BOOL);
typedef BOOL (API* WTClose_Func)(HCTX);
typedef BOOL (API* WTPacket_Func)(HCTX, UINT, LPVOID);

namespace she {

static WTInfoW_Func WTInfo;
static WTOpenW_Func WTOpen;
static WTClose_Func WTClose;
static WTPacket_Func WTPacket;

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

HCTX PenAPI::open(HWND hwnd)
{
  if (!m_wintabLib && !loadWintab())
    return nullptr;

  LOGCONTEXTW logctx;
  memset(&logctx, 0, sizeof(LOGCONTEXTW));
  UINT infoRes = WTInfo(WTI_DEFSYSCTX, 0, &logctx);

  // TODO Sometimes we receive infoRes=88 from WTInfo and logctx.lcOptions=0
  //      while sizeof(LOGCONTEXTW) is 212
  ASSERT(infoRes == sizeof(LOGCONTEXTW));
  ASSERT(logctx.lcOptions & CXO_SYSTEM);

  if (infoRes != sizeof(LOGCONTEXTW)) {
    LOG(ERROR)
      << "PEN: Not supported WTInfo:\n"
      << "     Expected context size: " << sizeof(LOGCONTEXTW) << "\n"
      << "     Actual context size: " << infoRes << " (options " << logctx.lcOptions << ")\n";
    return nullptr;
  }

  logctx.lcOptions =
    CXO_SYSTEM |
    CXO_MESSAGES |
    CXO_CSRMESSAGES;
  logctx.lcPktData = PACKETDATA;
  logctx.lcPktMode = PACKETMODE;
  logctx.lcMoveMask = PACKETDATA;

  HCTX ctx = WTOpen(hwnd, &logctx, TRUE);
  if (!ctx) {
    LOG("PEN: Error attaching pen to display\n");
    return nullptr;
  }

  LOG("PEN: Pen attached to display\n");
  return ctx;
}

void PenAPI::close(HCTX ctx)
{
  if (ctx) {
    ASSERT(m_wintabLib);
    LOG("PEN: Pen detached from window\n");
    WTClose(ctx);
  }
}

bool PenAPI::packet(HCTX ctx, UINT serial, LPVOID packet)
{
  return (WTPacket(ctx, serial, packet) ? true: false);
}

bool PenAPI::loadWintab()
{
  ASSERT(!m_wintabLib);

  m_wintabLib = base::load_dll("wintab32.dll");
  if (!m_wintabLib) {
    LOG(ERROR) << "PEN: wintab32.dll is not present\n";
    return false;
  }

  WTInfo = base::get_dll_proc<WTInfoW_Func>(m_wintabLib, "WTInfoW");
  WTOpen = base::get_dll_proc<WTOpenW_Func>(m_wintabLib, "WTOpenW");
  WTClose = base::get_dll_proc<WTClose_Func>(m_wintabLib, "WTClose");
  WTPacket = base::get_dll_proc<WTPacket_Func>(m_wintabLib, "WTPacket");
  if (!WTInfo || !WTOpen || !WTClose || !WTPacket) {
    LOG(ERROR) << "PEN: wintab32.dll does not contain all required functions\n";
    return false;
  }

  LOG("PEN: Pen initialized\n");
  return true;
}

} // namespace she
