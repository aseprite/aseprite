// Aseprite Steam Wrapper
// Copyright (c) 2020 Igara Studio S.A.
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
#include "base/ints.h"
#include "base/log.h"
#include "base/string.h"

namespace steam {

typedef uint32_t ScreenshotHandle;
typedef void* ISteamScreenshots;

// Steam main API
typedef bool __cdecl (*SteamAPI_Init_Func)();
typedef void __cdecl (*SteamAPI_Shutdown_Func)();

// ISteamScreenshots
typedef ISteamScreenshots* __cdecl (*SteamAPI_SteamScreenshots_v003_Func)();
typedef ScreenshotHandle __cdecl (*SteamAPI_ISteamScreenshots_WriteScreenshot_Func)(ISteamScreenshots*, void*, uint32_t, int, int);

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

#define GETPROC(name) base::get_dll_proc<name##_Func>(m_steamLib, #name)

class SteamAPI::Impl {
public:
  Impl() {
    m_steamLib = base::load_dll(
      base::join_path(base::get_file_path(base::get_app_path()),
                      STEAM_API_DLL_FILENAME));
    if (!m_steamLib) {
      LOG("STEAM: Steam library not found...\n");
      return;
    }

    auto SteamAPI_Init = GETPROC(SteamAPI_Init);
    if (!SteamAPI_Init) {
      LOG("STEAM: SteamAPI_Init not found...\n");
      return;
    }

    // Call SteamAPI_Init() to connect to Steam
    if (!SteamAPI_Init()) {
      LOG("STEAM: Steam is not initialized...\n");
      return;
    }

    LOG("STEAM: Steam initialized\n");
    m_initialized = true;
  }

  ~Impl() {
    if (!m_steamLib)
      return;

    auto SteamAPI_Shutdown = GETPROC(SteamAPI_Shutdown);
    if (SteamAPI_Shutdown) {
      LOG("STEAM: Steam shutdown...\n");
      SteamAPI_Shutdown();
    }

    base::unload_dll(m_steamLib);
  }

  bool initialized() const {
    return m_initialized;
  }

  bool writeScreenshot(void* rgbBuffer,
                       uint32_t sizeInBytes,
                       int width, int height) {
    if (!m_initialized)
      return false;

    auto SteamScreenshots = GETPROC(SteamAPI_SteamScreenshots_v003);
    auto WriteScreenshot = GETPROC(SteamAPI_ISteamScreenshots_WriteScreenshot);
    if (!SteamScreenshots || !WriteScreenshot) {
      LOG("STEAM: Error getting Steam Screenshot API functions\n");
      return false;
    }

    auto screenshots = SteamScreenshots();
    if (!screenshots) {
      LOG("STEAM: Error getting Steam Screenshot API instance\n");
      return false;
    }

    WriteScreenshot(screenshots, rgbBuffer, sizeInBytes, width, height);
    return true;
  }

private:
  bool m_initialized = false;
  base::dll m_steamLib = nullptr;
};

SteamAPI* g_instance = nullptr;

// static
SteamAPI* SteamAPI::instance()
{
  return g_instance;
}

SteamAPI::SteamAPI()
  : m_impl(new Impl)
{
  ASSERT(g_instance == nullptr);
  g_instance = this;
}

SteamAPI::~SteamAPI()
{
  delete m_impl;

  ASSERT(g_instance == this);
  g_instance = nullptr;
}

bool SteamAPI::initialized() const
{
  return m_impl->initialized();
}

bool SteamAPI::writeScreenshot(void* rgbBuffer,
                               uint32_t sizeInBytes,
                               int width, int height)
{
  return m_impl->writeScreenshot(rgbBuffer, sizeInBytes, width, height);
}

} // namespace steam
