// LAF OS Library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_WINAPI_H_INCLUDED
#define OS_WIN_WINAPI_H_INCLUDED
#pragma once

#include "base/dll.h"

#include <windows.h>
#include <interactioncontext.h>

namespace os {

  typedef BOOL (WINAPI* EnableMouseInPointer_Func)(BOOL fEnable);
  typedef BOOL (WINAPI* IsMouseInPointerEnabled_Func)(void);
  typedef BOOL (WINAPI* GetPointerInfo_Func)(UINT32 pointerId, POINTER_INFO* pointerInfo);
  typedef BOOL (WINAPI* GetPointerPenInfo_Func)(UINT32 pointerId, POINTER_PEN_INFO* penInfo);

  typedef HRESULT (WINAPI* CreateInteractionContext_Func)(HINTERACTIONCONTEXT* interactionContext);
  typedef HRESULT (WINAPI* DestroyInteractionContext_Func)(HINTERACTIONCONTEXT interactionContext);
  typedef HRESULT (WINAPI* StopInteractionContext_Func)(HINTERACTIONCONTEXT interactionContext);
  typedef HRESULT (WINAPI* RegisterOutputCallbackInteractionContext_Func)(
    HINTERACTIONCONTEXT interactionContext,
    INTERACTION_CONTEXT_OUTPUT_CALLBACK outputCallback,
    void* clientData);
  typedef HRESULT (WINAPI* AddPointerInteractionContext_Func)(
    HINTERACTIONCONTEXT interactionContext,
    UINT32 pointerId);
  typedef HRESULT (WINAPI* RemovePointerInteractionContext_Func)(
    HINTERACTIONCONTEXT interactionContext,
    UINT32 pointerId);
  typedef HRESULT (WINAPI* SetInteractionConfigurationInteractionContext_Func)(
     HINTERACTIONCONTEXT interactionContext,
     UINT32 configurationCount,
     const INTERACTION_CONTEXT_CONFIGURATION* configuration);
  typedef HRESULT (WINAPI* SetPropertyInteractionContext_Func)(
    HINTERACTIONCONTEXT interactionContext,
    INTERACTION_CONTEXT_PROPERTY contextProperty,
    UINT32 value);
  typedef HRESULT (WINAPI* ProcessPointerFramesInteractionContext_Func)(
    HINTERACTIONCONTEXT interactionContext,
    UINT32 entriesCount,
    UINT32 pointerCount,
    const POINTER_INFO* pointerInfo);

  class WinAPI {
  public:
    WinAPI();
    ~WinAPI();

    // These functions are availble only since Windows 8
    EnableMouseInPointer_Func EnableMouseInPointer;
    IsMouseInPointerEnabled_Func IsMouseInPointerEnabled;
    GetPointerInfo_Func GetPointerInfo;
    GetPointerPenInfo_Func GetPointerPenInfo;

    // InteractionContext introduced on Windows 8
    CreateInteractionContext_Func CreateInteractionContext;
    DestroyInteractionContext_Func DestroyInteractionContext;
    StopInteractionContext_Func StopInteractionContext;
    RegisterOutputCallbackInteractionContext_Func RegisterOutputCallbackInteractionContext;
    AddPointerInteractionContext_Func AddPointerInteractionContext;
    RemovePointerInteractionContext_Func RemovePointerInteractionContext;
    SetInteractionConfigurationInteractionContext_Func SetInteractionConfigurationInteractionContext;
    SetPropertyInteractionContext_Func SetPropertyInteractionContext;
    ProcessPointerFramesInteractionContext_Func ProcessPointerFramesInteractionContext;

  private:
    base::dll m_user32;
    base::dll m_ninput;
  };

} // namespace os

#endif
