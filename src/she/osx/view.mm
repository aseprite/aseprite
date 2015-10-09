// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/osx/view.h"

#include "gfx/point.h"
#include "she/event.h"
#include "she/event_queue.h"
#include "she/osx/window.h"

namespace {

inline gfx::Point get_local_mouse_pos(NSView* view, NSEvent* event)
{
  NSPoint point = [view convertPoint:[event locationInWindow]
                            fromView:nil];
  int scale = 1;

  if ([view window])
    scale = [(OSXWindow*)[view window] scale];

  // "she" layer coordinates expect (X,Y) origin at the top-left corner.
  return gfx::Point(point.x / scale,
                    (view.bounds.size.height - point.y) / scale);
}

inline she::Event::MouseButton get_mouse_buttons(NSEvent* event)
{
  switch ([event buttonNumber]) {
    case 0: return she::Event::LeftButton; break;
    case 1: return she::Event::RightButton; break;
    case 2: return she::Event::MiddleButton; break;
    // TODO add support for other buttons
  }
  return she::Event::MouseButton::NoneButton;
}

} // anonymous namespace

@implementation OSXView

- (id)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self != nil) {
    [self createMouseTrackingArea];
  }
  return self;
}

- (void)viewDidHide
{
  [super viewDidHide];
  [self destroyMouseTrackingArea];
}

- (void)viewDidUnhide
{
  [super viewDidUnhide];
  [self createMouseTrackingArea];
}

- (void)viewDidMoveToWindow
{
  [super viewDidMoveToWindow];

  if ([self window]) {
    OSXWindowImpl* impl = [((OSXWindow*)[self window]) impl];
    if (impl)
      impl->onWindowChanged();
  }
}

- (void)drawRect:(NSRect)dirtyRect
{
  [super drawRect:dirtyRect];

  if ([self window]) {
    OSXWindowImpl* impl = [((OSXWindow*)[self window]) impl];
    if (impl)
      impl->onDrawRect(gfx::Rect(dirtyRect.origin.x,
                                 dirtyRect.origin.y,
                                 dirtyRect.size.width,
                                 dirtyRect.size.height));
  }
}

- (void)mouseDown:(NSEvent*)event
{
  she::Event ev;
  ev.setType(she::Event::MouseDown);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(she::Event::LeftButton);
  she::queue_event(ev);
}

- (void)mouseUp:(NSEvent*)event
{
  she::Event ev;
  ev.setType(she::Event::MouseUp);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(she::Event::LeftButton);
  she::queue_event(ev);
}

- (void)mouseEntered:(NSEvent*)event
{
  she::Event ev;
  ev.setType(she::Event::MouseEnter);
  ev.setPosition(get_local_mouse_pos(self, event));
  she::queue_event(ev);
}

- (void)mouseMoved:(NSEvent*)event
{
  she::Event ev;
  ev.setType(she::Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  she::queue_event(ev);
}

- (void)mouseExited:(NSEvent*)event
{
  she::Event ev;
  ev.setType(she::Event::MouseLeave);
  ev.setPosition(get_local_mouse_pos(self, event));
  she::queue_event(ev);
}

- (void)mouseDragged:(NSEvent*)event
{
  she::Event ev;
  ev.setType(she::Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  she::queue_event(ev);
}

- (void)setFrameSize:(NSSize)newSize
{
  [super setFrameSize:newSize];

  // Re-create the mouse tracking area
  [self destroyMouseTrackingArea];
  [self createMouseTrackingArea];

  // Call OSXWindowImpl::onResize handler
  if ([self window]) {
    OSXWindowImpl* impl = [((OSXWindow*)[self window]) impl];
    if (impl)
      impl->onResize(gfx::Size(newSize.width,
                               newSize.height));
  }
}

- (void)createMouseTrackingArea
{
  // Create a tracking area to receive mouseMoved events
  m_trackingArea =
    [[NSTrackingArea alloc]
        initWithRect:self.bounds
             options:(NSTrackingMouseEnteredAndExited |
                      NSTrackingMouseMoved |
                      NSTrackingActiveAlways |
                      NSTrackingEnabledDuringMouseDrag)
               owner:self
            userInfo:nil];
  [self addTrackingArea:m_trackingArea];
}

- (void)destroyMouseTrackingArea
{
  [self removeTrackingArea:m_trackingArea];
  m_trackingArea = nil;
}

@end
