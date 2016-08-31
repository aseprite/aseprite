// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;

BOOL IsWow64()
{
  BOOL isWow64 = FALSE;

  fnIsWow64Process = (LPFN_ISWOW64PROCESS)
    GetProcAddress(GetModuleHandle(L"kernel32"),
                   "IsWow64Process");
  if (fnIsWow64Process != NULL)
    fnIsWow64Process(GetCurrentProcess(), &isWow64);

  return isWow64;
}
