// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_WINDOW_H_INCLUDED
#define SHE_OSX_WINDOW_H_INCLUDED
#pragma once

#include <Cocoa/Cocoa.h>

#include "gfx/rect.h"
#include "gfx/size.h"

class OSXWindowImpl {
public:
  virtual ~OSXWindowImpl() { }
  virtual void onClose() = 0;
  virtual void onResize(const gfx::Size& size) = 0;
  virtual void onDrawRect(const gfx::Rect& rect) = 0;
  virtual void onWindowChanged() = 0;
};

@class OSXWindowDelegate;

@interface OSXWindow : NSWindow {
@private
  OSXWindowImpl* m_impl;
  OSXWindowDelegate* m_delegate;
  int m_scale;
  gfx::Size m_clientSize;
  gfx::Size m_restoredSize;
}
- (OSXWindow*)initWithImpl:(OSXWindowImpl*)impl;
- (OSXWindowImpl*)impl;
- (int)scale;
- (void)setScale:(int)scale;
- (gfx::Size)clientSize;
- (gfx::Size)restoredSize;
@end

#endif
