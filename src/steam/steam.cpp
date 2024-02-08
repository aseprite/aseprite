// Aseprite Steam Wrapper
// Copyright (c) 2020-2024 Igara Studio S.A.
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "steam/steam.h"

#include "base/convert_to.h"
#include "base/dll.h"
#include "base/fs.h"
#include "base/ints.h"
#include "base/launcher.h"
#include "base/log.h"
#include "base/string.h"

namespace steam {

typedef uint32_t HSteamPipe;
typedef uint32_t HSteamUser;
typedef uint32_t ScreenshotHandle;
struct ISteamScreenshots;
struct ISteamUtils;

enum {
  // Last callback received from Steam client when it's Steam is closed
  kSteamServersDisconnected = 103,
  kSteamUndocumentedLastCallback = 1009,
  // When a screenshot is ready in the library
  kScreenshotReady = 2301,
};

struct CallbackMsg_t {
  HSteamUser steamUser;
  int callback;
  uint8_t* pubParam;
  int cubParam;
};

#ifdef __GNUC__
  #define __cdecl
#endif

// Steam main API
typedef bool (__cdecl *SteamAPI_InitSafe_Func)();
typedef void (__cdecl *SteamAPI_Shutdown_Func)();
typedef HSteamPipe (__cdecl *SteamAPI_GetHSteamPipe_Func)();

// Steam callbacks
typedef void (__cdecl *SteamAPI_ManualDispatch_Init_Func)();
typedef void (__cdecl *SteamAPI_ManualDispatch_RunFrame_Func)(HSteamPipe);
typedef bool (__cdecl *SteamAPI_ManualDispatch_GetNextCallback_Func)(HSteamPipe, CallbackMsg_t*);
typedef void (__cdecl *SteamAPI_ManualDispatch_FreeLastCallback_Func)(HSteamPipe);

// ISteamScreenshots
typedef ISteamScreenshots* (__cdecl *SteamAPI_SteamScreenshots_v003_Func)();
typedef ScreenshotHandle (__cdecl *SteamAPI_ISteamScreenshots_WriteScreenshot_Func)(ISteamScreenshots*, void*, uint32_t, int, int);

// ISteamUtils
typedef ISteamUtils* (__cdecl *SteamAPI_SteamUtils_v009_Func)();
typedef uint32_t (__cdecl *SteamAPI_ISteamUtils_GetAppID_Func)(ISteamUtils*);

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

    auto SteamAPI_InitSafe = GETPROC(SteamAPI_InitSafe);
    if (!SteamAPI_InitSafe) {
      LOG("STEAM: SteamAPI_InitSafe not found...\n");
      return;
    }

    // Call SteamAPI_InitSafe() to connect to Steam
    if (!SteamAPI_InitSafe()) {
      LOG("STEAM: Steam is not initialized...\n");
      return;
    }

    // Get functions to dispatch callbacks manually
    auto SteamAPI_ManualDispatch_Init = GETPROC(SteamAPI_ManualDispatch_Init);
    SteamAPI_ManualDispatch_RunFrame = GETPROC(SteamAPI_ManualDispatch_RunFrame);
    SteamAPI_ManualDispatch_GetNextCallback = GETPROC(SteamAPI_ManualDispatch_GetNextCallback);
    SteamAPI_ManualDispatch_FreeLastCallback = GETPROC(SteamAPI_ManualDispatch_FreeLastCallback);
    auto SteamAPI_GetHSteamPipe = GETPROC(SteamAPI_GetHSteamPipe);
    if (SteamAPI_ManualDispatch_Init &&
        SteamAPI_ManualDispatch_RunFrame &&
        SteamAPI_ManualDispatch_GetNextCallback &&
        SteamAPI_ManualDispatch_FreeLastCallback &&
        SteamAPI_GetHSteamPipe) {
      SteamAPI_ManualDispatch_Init();
      m_pipe = SteamAPI_GetHSteamPipe();
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

    unloadLib();
  }

  bool initialized() const {
    return m_initialized;
  }

  void runCallbacks() {
    if (!m_pipe)
      return;

    ASSERT(SteamAPI_ManualDispatch_RunFrame);
    ASSERT(SteamAPI_ManualDispatch_GetNextCallback);
    ASSERT(SteamAPI_ManualDispatch_FreeLastCallback);

    SteamAPI_ManualDispatch_RunFrame(m_pipe);

    CallbackMsg_t msg;
    if (SteamAPI_ManualDispatch_GetNextCallback(m_pipe, &msg)) {
      //TRACEARGS("SteamAPI_ManualDispatch_GetNextCallback", msg.callback);

      bool disconnected = false;
      switch (msg.callback) {
        case kSteamServersDisconnected:
        case kSteamUndocumentedLastCallback:
          disconnected = true;
          break;

        // When a screenshot is ready, we open the Steam library of screenshots
        case kScreenshotReady: {
          std::string url = "steam://open/screenshots/";

          auto SteamAPI_SteamUtils_v009 = GETPROC(SteamAPI_SteamUtils_v009);
          auto SteamAPI_ISteamUtils_GetAppID = GETPROC(SteamAPI_ISteamUtils_GetAppID);
          if (SteamAPI_SteamUtils_v009 &&
              SteamAPI_ISteamUtils_GetAppID) {
            ISteamUtils* utils = SteamAPI_SteamUtils_v009();
            if (utils) {
              int appId = SteamAPI_ISteamUtils_GetAppID(utils);
              url += base::convert_to<std::string>(appId);
            }
          }
          base::launcher::open_url(url);
          break;
        }
      }
      SteamAPI_ManualDispatch_FreeLastCallback(m_pipe);

      // If the Steam client is closed, we have to unload the DLL and
      // don't use the pipe or any Steam API at all, in other case we
      // would crash.
      if (disconnected) {
        LOG("STEAM: Disconnected\n");
        unloadLib();
      }
    }
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
  void unloadLib() {
    base::unload_dll(m_steamLib);
    m_steamLib = nullptr;
    m_initialized = false;
    m_pipe = 0;
  }

  bool m_initialized = false;
  base::dll m_steamLib = nullptr;

  // To handle callbacks manually
  HSteamPipe m_pipe = 0;
  SteamAPI_ManualDispatch_RunFrame_Func SteamAPI_ManualDispatch_RunFrame = nullptr;
  SteamAPI_ManualDispatch_GetNextCallback_Func SteamAPI_ManualDispatch_GetNextCallback = nullptr;
  SteamAPI_ManualDispatch_FreeLastCallback_Func SteamAPI_ManualDispatch_FreeLastCallback = nullptr;
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

void SteamAPI::runCallbacks()
{
  m_impl->runCallbacks();
}

bool SteamAPI::writeScreenshot(void* rgbBuffer,
                               uint32_t sizeInBytes,
                               int width, int height)
{
  return m_impl->writeScreenshot(rgbBuffer, sizeInBytes, width, height);
}

} // namespace steam
