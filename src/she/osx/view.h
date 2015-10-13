// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_VIEW_H_INCLUDED
#define SHE_OSX_VIEW_H_INCLUDED
#pragma once

#include <Cocoa/Cocoa.h>

@interface OSXView : NSView {
@private
  NSTrackingArea* m_trackingArea;
}
- (id)initWithFrame:(NSRect)frameRect;
- (BOOL)acceptsFirstResponder;
- (void)viewDidHide;
- (void)viewDidUnhide;
- (void)viewDidMoveToWindow;
- (void)drawRect:(NSRect)dirtyRect;
- (void)keyDown:(NSEvent*)event;
- (void)keyUp:(NSEvent*)event;
- (void)flagsChanged:(NSEvent*)event;
- (void)mouseEntered:(NSEvent*)event;
- (void)mouseMoved:(NSEvent*)event;
- (void)mouseExited:(NSEvent*)event;
- (void)mouseDown:(NSEvent*)event;
- (void)mouseUp:(NSEvent*)event;
- (void)mouseDragged:(NSEvent*)event;
- (void)rightMouseDown:(NSEvent*)event;
- (void)rightMouseUp:(NSEvent*)event;
- (void)rightMouseDragged:(NSEvent*)event;
- (void)otherMouseDown:(NSEvent*)event;
- (void)otherMouseUp:(NSEvent*)event;
- (void)otherMouseDragged:(NSEvent*)event;
- (void)handleMouseDown:(NSEvent*)event;
- (void)handleMouseUp:(NSEvent*)event;
- (void)handleMouseDragged:(NSEvent*)event;
- (void)setFrameSize:(NSSize)newSize;
- (void)createMouseTrackingArea;
- (void)destroyMouseTrackingArea;
@end

#endif
