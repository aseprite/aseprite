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
#include "she/osx/view.h"
#include "she/osx/window_delegate.h"

@implementation OSXWindow

- (OSXWindow*)initWithImpl:(OSXWindowImpl*)impl
{
  m_impl = impl;

  NSRect rect = NSMakeRect(0, 0, 640, 480);
  m_clientSize.w = m_restoredSize.w = rect.size.width;
  m_clientSize.h = m_restoredSize.h = rect.size.height;

  OSXView* view = [[OSXView alloc] initWithFrame:rect];
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

- (OSXWindowImpl*)impl
{
  return m_impl;
}

- (gfx::Size)clientSize
{
  return m_clientSize;
}

- (gfx::Size)restoredSize
{
  return m_restoredSize;
}

@end
