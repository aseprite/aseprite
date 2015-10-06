// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/osx/window.h"

#include "she/event.h"
#include "she/event_queue.h"
#include "she/system.h"

@interface OSXWindowDelegate : NSObject
{
  she::EventQueue* queue;
  OSXWindow* window;
}
- (BOOL)windowShouldClose:(id)sender;
- (void)windowWillClose:(NSNotification *)notification;
- (void)windowDidResize:(NSNotification*)notification;
- (void)windowDidMiniaturize:(NSNotification*)notification;
- (void)setEventQueue:(she::EventQueue*)aQueue;
- (void)setOSXWindow:(OSXWindow*)aWindow;
@end

@interface OSXView : NSView
- (void)mouseDragged:(NSEvent*)theEvent;
@end

@implementation OSXWindowDelegate

- (BOOL)windowShouldClose:(id)sender
{
  [window closeDelegate]->notifyClose();
  return YES;
}

- (void)windowWillClose:(NSNotification*)notification
{
}

- (void)windowDidResize:(NSNotification*)notification
{
}

- (void)windowDidMiniaturize:(NSNotification*)notification
{
}

- (void)setEventQueue:(she::EventQueue*)aQueue
{
  queue = aQueue;
}

- (void)setOSXWindow:(OSXWindow*)aWindow
{
  window = aWindow;
}

@end

@implementation OSXView

- (void)mouseDragged:(NSEvent*)theEvent
{
}

@end

@implementation OSXWindow

- (OSXWindow*)init
{
  closeDelegate = nullptr;

  NSRect rect = NSMakeRect(0, 0, 640, 480);
  clientSize.w = restoredSize.w = rect.size.width;
  clientSize.h = restoredSize.h = rect.size.height;

  OSXWindowDelegate* windowDelegate = [[OSXWindowDelegate new] autorelease];
  [windowDelegate setEventQueue:she::instance()->eventQueue()];
  [windowDelegate setOSXWindow:self];

  OSXView* view = [[[OSXView alloc] initWithFrame:rect] autorelease];
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

  [super initWithContentRect:rect
                   styleMask:(NSTitledWindowMask | NSClosableWindowMask |
                              NSMiniaturizableWindowMask | NSResizableWindowMask)
                     backing:NSBackingStoreBuffered
                       defer:NO];

  [self setDelegate:windowDelegate];
  [self setContentView:view];
  [self center];
  return self;
}

- (void)dealloc
{
  [super dealloc];
}

- (CloseDelegate*)closeDelegate
{
  return closeDelegate;
}

- (void)setCloseDelegate:(CloseDelegate*)aDelegate
{
  closeDelegate = aDelegate;
}

- (gfx::Size)clientSize
{
  return clientSize;
}

- (gfx::Size)restoredSize
{
  return restoredSize;
}

@end
