// SHE library
// Copyright (C) 2012-2015  David Capello
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>

#include "she/osx/app.h"

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
    [m_app activateIgnoringOtherApps:YES];
    [m_app finishLaunching];

    return true;
  }

private:
  NSApplication* m_app;
  id m_appDelegate;
};

OSXApp::OSXApp()
  : m_impl(new Impl)
{
}

OSXApp::~OSXApp()
{
}

bool OSXApp::init()
{
  return m_impl->init();
}

} // namespace she
