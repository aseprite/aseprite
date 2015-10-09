// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

#include "she/osx/app.h"

#include "base/thread.h"
#include "she/osx/app_delegate.h"

extern int app_main(int argc, char* argv[]);

namespace she {

OSXApp* OSXApp::g_instance = nullptr;

OSXApp::OSXApp()
{
  g_instance = this;
}

OSXApp::~OSXApp()
{
  g_instance = nullptr;
}

int OSXApp::run(int argc, char* argv[])
{
  NSApplication* app = [NSApplication sharedApplication];
  id appDelegate = [OSXAppDelegate new];

  [app setActivationPolicy:NSApplicationActivationPolicyRegular];
  [app setDelegate:appDelegate];

  app_main(argc, argv);
  return 0;
}

} // namespace she
