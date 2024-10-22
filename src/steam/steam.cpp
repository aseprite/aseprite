// Aseprite Steam Wrapper
// Copyright (c) 2020-2024 Igara Studio S.A.
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "steam/steam.h"
#include "base/convert_to.h"
#include "base/dll.h"
#include "base/fs.h"
#include "base/ints.h"
#include "base/launcher.h"
#include "base/log.h"
#include "base/string.h"

namespace steam {

class SteamAPI::Impl {
public:
  Impl() {
    m_steamLib = base::load_dll(base::join_path(base::get_file_path(base::get_app_path()), STEAM_API_DLL_FILENAME));
    if (!m_steamLib) {
      LOG("STEAM: Steam library not found...\n");
      return;
    }
    initializeSteamAPI();
  }

  ~Impl() {
    if (!m_steamLib) return;

    auto SteamAPI_Shutdown = getProc<SteamAPI_Shutdown_Func>("SteamAPI_Shutdown");
    if (SteamAPI_Shutdown) {
      LOG("STEAM: Steam shutdown...\n");
      SteamAPI_Shutdown();
    }

    unloadLib();
  }

  bool isInitialized() const {
    return m_initialized;
  }

  void runCallbacks() {
    if (!m_pipe) return;

    ASSERT(SteamAPI_ManualDispatch_RunFrame);
    ASSERT(SteamAPI_ManualDispatch_GetNextCallback);
    ASSERT(SteamAPI_ManualDispatch_FreeLastCallback);

    SteamAPI_ManualDispatch_RunFrame(m_pipe);

    std::unique_ptr<CallbackMsg_t> msg = std::make_unique<CallbackMsg_t>();
    if (SteamAPI_ManualDispatch_GetNextCallback(m_pipe, msg.get())) {
      bool disconnected = false;
      switch (msg->callback) {
        case kSteamServersDisconnected:
        case kSteamUndocumentedLastCallback:
          disconnected = true;
          break;

        case kScreenshotReady: {
          std::string url = "steam://open/screenshots/";
          auto SteamAPI_SteamUtils_v009 = getProc<SteamAPI_SteamUtils_v009_Func>("SteamAPI_SteamUtils_v009");
          auto SteamAPI_ISteamUtils_GetAppID = getProc<SteamAPI_ISteamUtils_GetAppID_Func>("SteamAPI_ISteamUtils_GetAppID");

          if (SteamAPI_SteamUtils_v009 && SteamAPI_ISteamUtils_GetAppID) {
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

      if (disconnected) {
        LOG("STEAM: Disconnected\n");
        unloadLib();
      }
    }
  }

  bool writeScreenshot(void* rgbBuffer, uint32_t sizeInBytes, int width, int height) {
    if (!m_initialized) return false;

    auto SteamScreenshots = getProc<SteamAPI_SteamScreenshots_v003_Func>("SteamAPI_SteamScreenshots_v003");
    auto WriteScreenshot = getProc<SteamAPI_ISteamScreenshots_WriteScreenshot_Func>("SteamAPI_ISteamScreenshots_WriteScreenshot");

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

  template <typename Func>
  Func getProc(const char* procName) {
    Func proc = base::get_dll_proc<Func>(m_steamLib, procName);
    if (!proc) {
      LOG("STEAM: %s not found...\n", procName);
      return nullptr;
    }
    return proc;
  }

  void initializeSteamAPI() {
    auto SteamAPI_InitSafe = getProc<SteamAPI_InitSafe_Func>("SteamAPI_InitSafe");
    if (!SteamAPI_InitSafe || !SteamAPI_InitSafe()) {
      LOG("STEAM: Steam is not initialized...\n");
      return;
    }

    auto SteamAPI_ManualDispatch_Init = getProc<SteamAPI_ManualDispatch_Init_Func>("SteamAPI_ManualDispatch_Init");
    SteamAPI_ManualDispatch_RunFrame = getProc<SteamAPI_ManualDispatch_RunFrame_Func>("SteamAPI_ManualDispatch_RunFrame");
    SteamAPI_ManualDispatch_GetNextCallback = getProc<SteamAPI_ManualDispatch_GetNextCallback_Func>("SteamAPI_ManualDispatch_GetNextCallback");
    SteamAPI_ManualDispatch_FreeLastCallback = getProc<SteamAPI_ManualDispatch_FreeLastCallback_Func>("SteamAPI_ManualDispatch_FreeLastCallback");
    auto SteamAPI_GetHSteamPipe = getProc<SteamAPI_GetHSteamPipe_Func>("SteamAPI_GetHSteamPipe");

    if (SteamAPI_ManualDispatch_Init && SteamAPI_GetHSteamPipe) {
      SteamAPI_ManualDispatch_Init();
      m_pipe = SteamAPI_GetHSteamPipe();
    }

    LOG("STEAM: Steam initialized\n");
    m_initialized = true;
  }

  // Private members for managing Steam API
  bool m_initialized = false;
  base::dll m_steamLib = nullptr;
  HSteamPipe m_pipe = 0;

  // Function pointers for Steam API
  SteamAPI_ManualDispatch_RunFrame_Func SteamAPI_ManualDispatch_RunFrame = nullptr;
  SteamAPI_ManualDispatch_GetNextCallback_Func SteamAPI_ManualDispatch_GetNextCallback = nullptr;
  SteamAPI_ManualDispatch_FreeLastCallback_Func SteamAPI_ManualDispatch_FreeLastCallback = nullptr;
};

SteamAPI* g_instance = nullptr;

// static
SteamAPI* SteamAPI::instance() {
  return g_instance;
}

SteamAPI::SteamAPI() : m_impl(std::make_unique<Impl>()) {
  ASSERT(g_instance == nullptr);
  g_instance = this;
}

SteamAPI::~SteamAPI() {
  ASSERT(g_instance == this);
  g_instance = nullptr;
}

bool SteamAPI::isInitialized() const {
  return m_impl->isInitialized();
}

void SteamAPI::runCallbacks() {
  m_impl->runCallbacks();
}

bool SteamAPI::writeScreenshot(void* rgbBuffer, uint32_t sizeInBytes, int width, int height) {
  return m_impl->writeScreenshot(rgbBuffer, sizeInBytes, width, height);
}

}  // namespace steam
 
