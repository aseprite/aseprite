// Aseprite Steam Wrapper
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "steam/steam.h"

#include "base/dll.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/path.h"
#include "base/string.h"

namespace steam {

typedef bool (*SteamAPI_Init_Func)();
typedef void (*SteamAPI_Shutdown_Func)();

#ifdef _WIN32
  #ifdef _WIN64
    #define STEAM_API_DLL_FILENAME "steam_api64.dll"
  #else
    #define STEAM_API_DLL_FILENAME "steam_api.dll"
  #endif
#elif defined(__APPLE__)
  #define STEAM_API_DLL_FILENAME "libsteam_api.dylib"
#else
  #define STEAM_API_DLL_FILENAME "libsteam_api.so"
#endif

class SteamAPI::Impl {
public:
  Impl() : m_initialized(false) {
    m_steamLib = base::load_dll(
      base::join_path(base::get_file_path(base::get_app_path()),
                      STEAM_API_DLL_FILENAME));
    if (!m_steamLib) {
      LOG("STEAM: Steam library not found...\n");
      return;
    }

    auto SteamAPI_Init = base::get_dll_proc<SteamAPI_Init_Func>(m_steamLib, "SteamAPI_Init");
    if (!SteamAPI_Init) {
      LOG("STEAM: SteamAPI_Init not found...\n");
      return;
    }

    if (!SteamAPI_Init()) {
      LOG("STEAM: Steam is not initialized...\n");
      return;
    }

    LOG("STEAM: Steam initialized...\n");
    m_initialized = true;
  }

  ~Impl() {
    if (!m_steamLib)
      return;

    auto SteamAPI_Shutdown = base::get_dll_proc<SteamAPI_Shutdown_Func>(m_steamLib, "SteamAPI_Shutdown");
    if (SteamAPI_Shutdown) {
      LOG("STEAM: Steam shutdown...\n");
      SteamAPI_Shutdown();
    }

    base::unload_dll(m_steamLib);
  }

  bool initialized() const {
    return m_initialized;
  }

private:
  base::dll m_steamLib;
  bool m_initialized;
};

SteamAPI::SteamAPI()
  : m_impl(new Impl)
{
}

SteamAPI::~SteamAPI()
{
  delete m_impl;
}

bool SteamAPI::initialized() const
{
  return m_impl->initialized();
}

} // namespace steam
