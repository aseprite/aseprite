// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_APP_INIT_H_INCLUDED
#define APP_APP_INIT_H_INCLUDED
#pragma once

#include "base/memory.h"
#include "base/paths.h"
#include "doc/pixel_format.h"
#include "obs/signal.h"
#include "os/system.h"

#if ENABLE_SENTRY
  #include "app/sentry_wrapper.h"
  #if LAF_WINDOWS
    #define USE_SENTRY_BREADCRUMB_FOR_WINTAB 1
    #include "os/win/wintab.h"
  #endif
#else
  #include "base/memory_dump.h"
#endif

#include <memory>
#include <string>
#include <vector>

namespace {

  // Memory leak detector wrapper
  class MemLeak {
  public:
#ifdef LAF_MEMLEAK
    MemLeak() { base_memleak_init(); }
    ~MemLeak() { base_memleak_exit(); }
#else
    MemLeak() { }
#endif
  };


} // anonymous namespace

namespace app {
#if LAF_WINDOWS

  struct CoInit {
    long hr;
    CoInit();
    ~CoInit();
  };
#endif  // LAF_WINDOWS

#if ENABLE_SENTRY && USE_SENTRY_BREADCRUMB_FOR_WINTAB
  // Delegate to write Wintab information as a Sentry breadcrumb (to
  // know if there is a specific Wintab driver giving problems)
  class WintabApiDelegate : public os::WintabAPI::Delegate {
    bool m_done = false;
  public:
    WintabApiDelegate() {
      os::instance()->setWintabDelegate(this);
    }
    ~WintabApiDelegate() {
      os::instance()->setWintabDelegate(nullptr);
    }
    void onWintabID(const std::string& id) override {
      if (!m_done) {
        m_done = true;
        app::Sentry::addBreadcrumb("Wintab ID=" + id);
      }
    }
    void onWintabFields(const std::map<std::string, std::string>& fields) override {
      app::Sentry::addBreadcrumb("Wintab DLL", fields);
    }
  };
#endif // ENABLE_SENTRY && USE_SENTRY_BREADCRUMB_FOR_WINTAB

} // namespace app

#endif
