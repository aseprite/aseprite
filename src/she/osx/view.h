// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_VIEW_H_INCLUDED
#define SHE_OSX_VIEW_H_INCLUDED
#pragma once

#include <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

@interface OSXView : NSView {
@private
  NSTrackingArea* m_trackingArea;
}
- (id)initWithFrame:(NSRect)frameRect;
- (void)viewDidHide;
- (void)viewDidUnhide;
- (void)viewDidMoveToWindow;
- (void)drawRect:(NSRect)dirtyRect;
- (void)mouseDown:(NSEvent*)event;
- (void)mouseUp:(NSEvent*)event;
- (void)mouseEntered:(NSEvent*)event;
- (void)mouseMoved:(NSEvent*)event;
- (void)mouseExited:(NSEvent*)event;
- (void)mouseDragged:(NSEvent*)event;
- (void)setFrameSize:(NSSize)newSize;
- (void)createMouseTrackingArea;
- (void)destroyMouseTrackingArea;
@end

#endif
