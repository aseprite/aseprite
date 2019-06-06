// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>

typedef BOOL (WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
typedef const char* (CDECL* LPFN_WINE_GET_VERSION)(void);

static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
static LPFN_WINE_GET_VERSION fnWineGetVersion = NULL;

BOOL IsWow64()
{
  if (!fnIsWow64Process) {
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)
      GetProcAddress(GetModuleHandle(L"kernel32"),
                     "IsWow64Process");
  }

  BOOL isWow64 = FALSE;
  if (fnIsWow64Process)
    fnIsWow64Process(GetCurrentProcess(), &isWow64);
  return isWow64;
}

const char* WineVersion()
{
  if (!fnWineGetVersion) {
    fnWineGetVersion = (LPFN_WINE_GET_VERSION)
      GetProcAddress(GetModuleHandle(L"ntdll.dll"),
                     "wine_get_version");
  }
  if (fnWineGetVersion)
    return fnWineGetVersion();
  else
    return NULL;
}
