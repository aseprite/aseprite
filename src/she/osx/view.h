// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

inline gfx::Point get_local_mouse_pos(NSView* view, NSEvent* event)
{
  NSPoint point = [view convertPoint:[event locationInWindow]
                            fromView:nil];
  // "she" layer coordinates expect (X,Y) origin at the top-left corner.
  return gfx::Point(point.x,
                    view.bounds.size.height - point.y);
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

@interface OSXView : NSView
{
  NSTrackingArea* m_trackingArea;
}
- (id)initWithFrame:(NSRect)frameRect;
- (void)dealloc;
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

@implementation OSXView

- (id)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self != nil) {
    [self createMouseTrackingArea];
  }
  return self;
}

- (void)dealloc
{
  [self destroyMouseTrackingArea];
  [super dealloc];
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
  [m_trackingArea release];
  m_trackingArea = nil;
}

@end
