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
  : m_userThread(nullptr)
{
  g_instance = this;
}

OSXApp::~OSXApp()
{
  g_instance = nullptr;
}

int OSXApp::run(int argc, char* argv[])
{
  NSAutoreleasePool* pool = [NSAutoreleasePool new];
  (void)pool;

  NSApplication* app = [NSApplication sharedApplication];
  OSXAppDelegate* appDelegate = [OSXAppDelegate new];

  // Create default main menu
  NSMenu* mainMenu = [[NSMenu new] autorelease];
  {
    NSMenu* appMenu = [[NSMenu new] autorelease];
    NSMenuItem* quitItem = [appMenu addItemWithTitle:@"Quit " PACKAGE
                                              action:@selector(quit:)
                                       keyEquivalent:@"q"];
    [quitItem setKeyEquivalentModifierMask:NSCommandKeyMask];
    [quitItem setTarget:appDelegate];

    NSMenuItem* appMenuItem = [[NSMenuItem new] autorelease];
    [appMenuItem setSubmenu:appMenu];

    [mainMenu setTitle:@PACKAGE];
    [mainMenu addItem:appMenuItem];
  }

  [app setActivationPolicy:NSApplicationActivationPolicyRegular];
  [app setDelegate:appDelegate];
  [app setMainMenu:mainMenu];

  // The whole application runs in a background thread (non-main UI thread).
  m_userThread = new base::thread([&]() {
    // Ignore return value, as [NSApp run] doesn't return we cannot use it.
    app_main(argc, argv);
  });

  [app run];

  // In this case, the main NSRunLoop was stopped, so we have to terminate here.
  //if (m_userThread)
  // joinUserThread();

  return 0;
}

void OSXApp::joinUserThread()
{
  // Join the user background thread to call all destructors and close everything properly.
  m_userThread->join();
  delete m_userThread;
  m_userThread = nullptr;
}

void OSXApp::stopUIEventLoop()
{
  // Stop the main NSRunLoop and post a dummy event to wake it up.
  [NSApp stop:nil];
  [NSApp postEvent:[NSEvent new] atStart:true];
}

} // namespace she
