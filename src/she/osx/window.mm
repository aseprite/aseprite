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
  she::EventQueue* m_queue;
  OSXWindow* m_window;
}
- (OSXWindowDelegate*)initWithWindow:(OSXWindow*)window;
- (BOOL)windowShouldClose:(id)sender;
- (void)windowWillClose:(NSNotification *)notification;
- (void)windowDidResize:(NSNotification*)notification;
- (void)windowDidMiniaturize:(NSNotification*)notification;
@end

@interface OSXView : NSView
- (void)mouseDragged:(NSEvent*)theEvent;
@end

@implementation OSXWindowDelegate

- (OSXWindowDelegate*)initWithWindow:(OSXWindow*)window
{
  m_window = window;
  m_queue = she::instance()->eventQueue();
  return self;
}

- (BOOL)windowShouldClose:(id)sender
{
  [m_window closeDelegate]->notifyClose();
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

  OSXView* view = [[[OSXView alloc] initWithFrame:rect] autorelease];
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

  [super initWithContentRect:rect
                   styleMask:(NSTitledWindowMask | NSClosableWindowMask |
                              NSMiniaturizableWindowMask | NSResizableWindowMask)
                     backing:NSBackingStoreBuffered
                       defer:NO];

  [self setDelegate:[[OSXWindowDelegate alloc] initWithWindow:self]];
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
