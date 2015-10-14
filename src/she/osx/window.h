// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_WINDOW_H_INCLUDED
#define SHE_OSX_WINDOW_H_INCLUDED
#pragma once

#include <Cocoa/Cocoa.h>

#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "she/keys.h"
#include "she/native_cursor.h"

namespace she {
  KeyScancode cocoavk_to_scancode(UInt16 vk);
}

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
}
- (OSXWindow*)initWithImpl:(OSXWindowImpl*)impl
                     width:(int)width
                    height:(int)height
                     scale:(int)scale;
- (OSXWindowImpl*)impl;
- (int)scale;
- (void)setScale:(int)scale;
- (gfx::Size)clientSize;
- (gfx::Size)restoredSize;
- (void)setMousePosition:(const gfx::Point&)position;
- (BOOL)setNativeMouseCursor:(she::NativeCursor)cursor;
@end

#endif
