// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>

#include "she/osx/app.h"

#include "base/debug.h"
#include "base/thread.h"
#include "she/osx/app_delegate.h"

extern int app_main(int argc, char* argv[]);

namespace she {

class OSXApp::Impl {
public:
  bool init() {
    m_app = [NSApplication sharedApplication];
    m_appDelegate = [OSXAppDelegate new];

    [m_app setActivationPolicy:NSApplicationActivationPolicyRegular];
    [m_app setDelegate:m_appDelegate];

    // Don't activate the application ignoring other apps. This is
    // called by OS X when the application is launched by the user
    // from the application bundle. In this way, we can execute
    // aseprite from the command line/bash scripts and the app will
    // not be activated.
    //[m_app activateIgnoringOtherApps:YES];

    return true;
  }

  // We might need to call this function when the app is launched from
  // Steam. It appears that there is a bug on OS X Steam client where
  // the app is launched, activated, and then the Steam client is
  // activated again.
  void activateApp() {
    [m_app activateIgnoringOtherApps:YES];
  }

  void finishLaunching() {
    [m_app finishLaunching];
  }

private:
  NSApplication* m_app;
  OSXAppDelegate* m_appDelegate;
};

static OSXApp* g_instance = nullptr;

// static
OSXApp* OSXApp::instance()
{
  return g_instance;
}

OSXApp::OSXApp()
  : m_impl(new Impl)
{
  ASSERT(!g_instance);
  g_instance = this;
}

OSXApp::~OSXApp()
{
  ASSERT(g_instance == this);
  g_instance = nullptr;
}

bool OSXApp::init()
{
  return m_impl->init();
}

void OSXApp::activateApp()
{
  m_impl->activateApp();
}

void OSXApp::finishLaunching()
{
  m_impl->finishLaunching();
}

} // namespace she
