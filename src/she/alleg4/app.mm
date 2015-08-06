// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <AppKit/AppKit.h>
#include <IOKit/hid/IOHIDLib.h>

#include <allegro.h>
#include <allegro/platform/alosx.h>
#include <allegro/internal/aintern.h>
#include <allegro/platform/aintosx.h>

#include "she/alleg4/alleg_display.h"
#include "she/event.h"
#include "she/event_queue.h"

void* get_osx_window()
{
  return osx_window;
}

@interface SheAppDelegate : AllegroAppDelegate
- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename;
@end

@implementation SheAppDelegate

- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename
{
  she::Event ev;
  ev.setType(she::Event::DropFiles);

  std::vector<std::string> files;
  files.push_back([filename UTF8String]);

  ev.setFiles(files);
  she::queue_event(ev);
  return YES;
}

@end

extern "C" int allegro_main(int argc, char *argv[], NSObject* app_delegate);

#undef main
int main(int argc, char *argv[])
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  (void)pool;

  SheAppDelegate* app_delegate = [[SheAppDelegate alloc] init];
  return allegro_main(argc, argv, app_delegate);
}
