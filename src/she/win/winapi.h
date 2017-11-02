// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_WINAPI_H_INCLUDED
#define SHE_WIN_WINAPI_H_INCLUDED
#pragma once

#include "base/dll.h"

#include <windows.h>

namespace she {

  typedef BOOL (WINAPI* EnableMouseInPointer_Func)(BOOL fEnable);
  typedef BOOL (WINAPI* IsMouseInPointerEnabled_Func)(void);
  typedef BOOL (WINAPI* GetPointerInfo_Func)(UINT32 pointerId, POINTER_INFO* pointerInfo);
  typedef BOOL (WINAPI* GetPointerPenInfo_Func)(UINT32 pointerId, POINTER_PEN_INFO* penInfo);

  class WinAPI {
  public:
    WinAPI();
    ~WinAPI();

    // These functions are availble only since Windows 8
    EnableMouseInPointer_Func EnableMouseInPointer;
    IsMouseInPointerEnabled_Func IsMouseInPointerEnabled;
    GetPointerInfo_Func GetPointerInfo;
    GetPointerPenInfo_Func GetPointerPenInfo;

  private:
    base::dll m_user32;
  };

} // namespace she

#endif
