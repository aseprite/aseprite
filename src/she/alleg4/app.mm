// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#import <AppKit/AppKit.h>
#import <IOKit/hid/IOHIDLib.h>

#include <allegro.h>
#include <allegro/platform/alosx.h>
#include <allegro/internal/aintern.h>
#include <allegro/platform/aintosx.h>

#include "she/alleg4/internals.h"
#include "she/event.h"
#include "she/system.h"

@interface SheAppDelegate : AllegroAppDelegate
- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename;
@end

@implementation SheAppDelegate

- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename
{
  // This case is used when the application wasn't run by the user,
  // and a file is double-clicked from Finder. As the she::Display
  // (the class that contains the events queue) is not yet created, we
  // can use the Allegro implementation which converts this
  // application:openFile: message to an argument for the command
  // line. So the clicked file will be opened in the application.
  if (!she::instance())
    return [super application: theApplication openFile: filename];

  she::Event ev;
  ev.setType(she::Event::DropFiles);

  std::vector<std::string> files;
  files.push_back([filename lossyCString]);

  ev.setFiles(files);
  she::queue_event(ev); // If the display is not created yet, we lost this event.
  return YES;
}

@end

extern "C" int allegro_main(int argc, char *argv[], NSObject* app_delegate);

#undef main
int main(int argc, char *argv[])
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  SheAppDelegate *app_delegate = [[SheAppDelegate alloc] init];
  return allegro_main(argc, argv, app_delegate);
}
