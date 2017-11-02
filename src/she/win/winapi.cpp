// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/win/winapi.h"

namespace she {

#define GET_PROC(dll, name)                             \
  name = base::get_dll_proc<name##_Func>(dll, #name)

WinAPI::WinAPI()
  : EnableMouseInPointer(nullptr)
  , IsMouseInPointerEnabled(nullptr)
  , GetPointerInfo(nullptr)
  , GetPointerPenInfo(nullptr)
  , m_user32(nullptr)
{
  m_user32 = base::load_dll("user32.dll");
  if (m_user32) {
    GET_PROC(m_user32, EnableMouseInPointer);
    GET_PROC(m_user32, IsMouseInPointerEnabled);
    GET_PROC(m_user32, GetPointerInfo);
    GET_PROC(m_user32, GetPointerPenInfo);
  }
}

WinAPI::~WinAPI()
{
  if (m_user32) {
    base::unload_dll(m_user32);
    m_user32 = nullptr;
  }
}

} // namespace she
