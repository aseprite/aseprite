// LAF OS Library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/win/winapi.h"

namespace os {

#define GET_PROC(dll, name)                             \
  name = base::get_dll_proc<name##_Func>(dll, #name)

WinAPI::WinAPI()
  : EnableMouseInPointer(nullptr)
  , IsMouseInPointerEnabled(nullptr)
  , GetPointerInfo(nullptr)
  , GetPointerPenInfo(nullptr)
  , CreateInteractionContext(nullptr)
  , DestroyInteractionContext(nullptr)
  , StopInteractionContext(nullptr)
  , RegisterOutputCallbackInteractionContext(nullptr)
  , AddPointerInteractionContext(nullptr)
  , RemovePointerInteractionContext(nullptr)
  , SetInteractionConfigurationInteractionContext(nullptr)
  , ProcessPointerFramesInteractionContext(nullptr)
  , m_user32(nullptr)
  , m_ninput(nullptr)
{
  m_user32 = base::load_dll("user32.dll");
  m_ninput = base::load_dll("ninput.dll");
  if (m_user32) {
    GET_PROC(m_user32, EnableMouseInPointer);
    GET_PROC(m_user32, IsMouseInPointerEnabled);
    GET_PROC(m_user32, GetPointerInfo);
    GET_PROC(m_user32, GetPointerPenInfo);
  }
  if (m_ninput) {
    GET_PROC(m_ninput, CreateInteractionContext);
    GET_PROC(m_ninput, DestroyInteractionContext);
    GET_PROC(m_ninput, StopInteractionContext);
    GET_PROC(m_ninput, RegisterOutputCallbackInteractionContext);
    GET_PROC(m_ninput, AddPointerInteractionContext);
    GET_PROC(m_ninput, RemovePointerInteractionContext);
    GET_PROC(m_ninput, SetInteractionConfigurationInteractionContext);
    GET_PROC(m_ninput, SetPropertyInteractionContext);
    GET_PROC(m_ninput, ProcessPointerFramesInteractionContext);
  }
}

WinAPI::~WinAPI()
{
  if (m_user32) {
    base::unload_dll(m_user32);
    m_user32 = nullptr;
  }
  if (m_ninput) {
    base::unload_dll(m_ninput);
    m_ninput = nullptr;
  }
}

} // namespace os
