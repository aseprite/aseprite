// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>

#include "she/osx/app_delegate.h"

#include "base/fs.h"
#include "she/event.h"
#include "she/event_queue.h"
#include "she/osx/app.h"
#include "she/osx/generate_drop_files.h"
#include "she/osx/view.h"
#include "she/system.h"

@protocol OSXValidateMenuItemProtocol
- (void)validateMenuItem;
@end

@implementation OSXAppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
  she::Event ev;
  ev.setType(she::Event::CloseDisplay);
  she::queue_event(ev);
  return NSTerminateCancel;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app
{
  return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
}

- (void)applicationWillResignActive:(NSNotification*)notification
{
  NSEvent* event = [NSApp currentEvent];
  if (event != nil)
    [OSXView updateKeyFlags:event];
}

- (void)applicationDidBecomeActive:(NSNotification*)notification
{
  NSEvent* event = [NSApp currentEvent];
  if (event != nil)
    [OSXView updateKeyFlags:event];
}

- (BOOL)application:(NSApplication*)app openFiles:(NSArray*)filenames
{
  generate_drop_files_from_nsarray(filenames);

  [app replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
  return YES;
}

- (void)executeMenuItem:(id)sender
{
  [sender executeMenuItem:sender];
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
  if ([menuItem respondsToSelector:@selector(validateMenuItem)]) {
    [((id<OSXValidateMenuItemProtocol>)menuItem) validateMenuItem];
    return menuItem.enabled;
  }
  else
    return [super validateMenuItem:menuItem];
}

@end
