// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>

#include "she/osx/app_delegate.h"

#include "she/event.h"
#include "she/event_queue.h"
#include "she/osx/app.h"
#include "she/system.h"

@implementation OSXAppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
  return NSTerminateNow;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app
{
  return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
  she::Event ev;
  ev.setType(she::Event::CloseDisplay);
  she::queue_event(ev);
}

- (BOOL)application:(NSApplication*)app openFiles:(NSArray*)filenames
{
  std::vector<std::string> files;
  for (int i=0; i<[filenames count]; ++i) {
    NSString* fn = [filenames objectAtIndex: i];
    files.push_back([fn UTF8String]);
  }

  she::Event ev;
  ev.setType(she::Event::DropFiles);
  ev.setFiles(files);
  she::queue_event(ev);

  [app replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
  return YES;
}

@end
