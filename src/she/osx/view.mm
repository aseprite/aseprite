// SHE library
// Copyright (C) 2015-2016  David Capello
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
#include "she/keys.h"
#include "she/osx/window.h"

using namespace she;

namespace {

// Internal array of pressed keys used in is_key_pressed()
bool pressed_keys[kKeyScancodes];

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

inline Event::MouseButton get_mouse_buttons(NSEvent* event)
{
  switch ([event buttonNumber]) {
    case 0: return Event::LeftButton; break;
    case 1: return Event::RightButton; break;
    case 2: return Event::MiddleButton; break;
    // TODO add support for other buttons
  }
  return Event::MouseButton::NoneButton;
}

inline KeyModifiers get_modifiers_from_nsevent(NSEvent* event)
{
  int modifiers = kKeyNoneModifier;
  NSEventModifierFlags nsFlags = event.modifierFlags;
  if (nsFlags & NSShiftKeyMask) modifiers |= kKeyShiftModifier;
  if (nsFlags & NSControlKeyMask) modifiers |= kKeyCtrlModifier;
  if (nsFlags & NSAlternateKeyMask) modifiers |= kKeyAltModifier;
  if (nsFlags & NSCommandKeyMask) modifiers |= kKeyCmdModifier;
  if (she::is_key_pressed(kKeySpace)) modifiers |= kKeySpaceModifier;
  return (KeyModifiers)modifiers;
}

inline int get_unicodechar_from_nsevent(NSEvent* event)
{
  int chr = 0;
  // TODO we should use "event.characters" for a new kind of event (Event::Char or something like that)
  NSString* chars = event.charactersIgnoringModifiers;
  if (chars && chars.length >= 1) {
    chr = [chars characterAtIndex:0];
    if (chr < 32)
      chr = 0;
  }
  return chr;
}

} // anonymous namespace

namespace she {

bool is_key_pressed(KeyScancode scancode)
{
  if (scancode >= 0 && scancode < kKeyScancodes)
    return pressed_keys[scancode];
  else
    return false;
}

} // namespace she

@implementation OSXView

- (id)initWithFrame:(NSRect)frameRect
{
  m_nsCursor = [NSCursor arrowCursor];
  m_visibleMouse = true;
  m_pointerType = she::PointerType::Unknown;

  self = [super initWithFrame:frameRect];
  if (self != nil) {
    [self createMouseTrackingArea];
  }
  return self;
}

- (BOOL)acceptsFirstResponder
{
  return YES;
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

- (void)keyDown:(NSEvent*)event
{
  [super keyDown:event];

  KeyScancode scancode = cocoavk_to_scancode(event.keyCode);
  if (scancode >= 0 && scancode < kKeyScancodes)
    pressed_keys[scancode] = true;

  Event ev;
  ev.setType(Event::KeyDown);
  ev.setScancode(scancode);
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setRepeat(event.ARepeat ? 1: 0);
  ev.setUnicodeChar(get_unicodechar_from_nsevent(event));

  queue_event(ev);
}

- (void)keyUp:(NSEvent*)event
{
  [super keyUp:event];

  KeyScancode scancode = cocoavk_to_scancode(event.keyCode);
  if (scancode >= 0 && scancode < kKeyScancodes)
    pressed_keys[scancode] = false;

  Event ev;
  ev.setType(Event::KeyUp);
  ev.setScancode(scancode);
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setRepeat(event.ARepeat ? 1: 0);
  ev.setUnicodeChar(get_unicodechar_from_nsevent(event));

  queue_event(ev);
}

- (void)flagsChanged:(NSEvent*)event
{
  [super flagsChanged:event];

  static int lastFlags = 0;
  static int flags[] = {
    NSShiftKeyMask,
    NSControlKeyMask,
    NSAlternateKeyMask,
    NSCommandKeyMask
  };
  static KeyScancode scancodes[] = {
    kKeyLShift,
    kKeyLControl,
    kKeyAlt,
    kKeyCommand
  };

  KeyModifiers modifiers = get_modifiers_from_nsevent(event);
  int newFlags = event.modifierFlags;

  for (int i=0; i<sizeof(flags)/sizeof(flags[0]); ++i) {
    if ((lastFlags & flags[i]) != (newFlags & flags[i])) {
      Event ev;
      ev.setType(
        ((newFlags & flags[i]) != 0 ? Event::KeyDown:
                                      Event::KeyUp));

      pressed_keys[scancodes[i]] = ((newFlags & flags[i]) != 0);

      ev.setScancode(scancodes[i]);
      ev.setModifiers(modifiers);
      ev.setRepeat(0);
      queue_event(ev);
    }
  }

  lastFlags = newFlags;
}

- (void)mouseEntered:(NSEvent*)event
{
  [self updateCurrentCursor];

  Event ev;
  ev.setType(Event::MouseEnter);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  queue_event(ev);
}

- (void)mouseMoved:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));

  if (m_pointerType != she::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  queue_event(ev);
}

- (void)mouseExited:(NSEvent*)event
{
  // Restore arrow cursor
  if (!m_visibleMouse) {
    m_visibleMouse = true;
    [NSCursor unhide];
  }
  [[NSCursor arrowCursor] set];

  Event ev;
  ev.setType(Event::MouseLeave);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  queue_event(ev);
}

- (void)mouseDown:(NSEvent*)event
{
  [self handleMouseDown:event];
}

- (void)mouseUp:(NSEvent*)event
{
  [self handleMouseUp:event];
}

- (void)mouseDragged:(NSEvent*)event
{
  [self handleMouseDragged:event];
}

- (void)rightMouseDown:(NSEvent*)event
{
  [self handleMouseDown:event];
}

- (void)rightMouseUp:(NSEvent*)event
{
  [self handleMouseUp:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
  [self handleMouseDragged:event];
}

- (void)otherMouseDown:(NSEvent*)event
{
  [self handleMouseDown:event];
}

- (void)otherMouseUp:(NSEvent*)event
{
  [self handleMouseUp:event];
}

- (void)otherMouseDragged:(NSEvent*)event
{
  [self handleMouseDragged:event];
}

- (void)handleMouseDown:(NSEvent*)event
{
  Event ev;
  ev.setType(event.clickCount == 2 ? Event::MouseDoubleClick:
                                     Event::MouseDown);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  ev.setModifiers(get_modifiers_from_nsevent(event));

  if (m_pointerType != she::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  queue_event(ev);
}

- (void)handleMouseUp:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseUp);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  ev.setModifiers(get_modifiers_from_nsevent(event));

  if (m_pointerType != she::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  queue_event(ev);
}

- (void)handleMouseDragged:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  ev.setModifiers(get_modifiers_from_nsevent(event));

  if (m_pointerType != she::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  queue_event(ev);
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

- (void)scrollWheel:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseWheel);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  ev.setModifiers(get_modifiers_from_nsevent(event));

  int scale = 1;
  if (self.window)
    scale = [(OSXWindow*)self.window scale];

  if (event.hasPreciseScrollingDeltas) {
    ev.setPointerType(she::PointerType::Multitouch);
    ev.setWheelDelta(gfx::Point(-event.scrollingDeltaX / scale,
                                -event.scrollingDeltaY / scale));
    ev.setPreciseWheel(true);
  }
  else {
    ev.setPointerType(she::PointerType::Mouse);
    ev.setWheelDelta(gfx::Point(-event.scrollingDeltaX,
                                -event.scrollingDeltaY));
  }

  queue_event(ev);
}

- (void)magnifyWithEvent:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::TouchMagnify);
  ev.setMagnification(event.magnification);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setPointerType(she::PointerType::Multitouch);
  queue_event(ev);
}

- (void)tabletProximity:(NSEvent*)event
{
  if (event.isEnteringProximity == YES) {
    switch (event.pointingDeviceType) {
      case NSPenPointingDevice: m_pointerType = she::PointerType::Pen; break;
      case NSCursorPointingDevice: m_pointerType = she::PointerType::Cursor; break;
      case NSEraserPointingDevice: m_pointerType = she::PointerType::Eraser; break;
      default:
        m_pointerType = she::PointerType::Unknown;
        break;
    }
  }
  else {
    m_pointerType = she::PointerType::Unknown;
  }
}

- (void)cursorUpdate:(NSEvent*)event
{
  [self updateCurrentCursor];
}

- (void)setCursor:(NSCursor*)cursor
{
  m_nsCursor = cursor;
  [self updateCurrentCursor];
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

- (void)updateCurrentCursor
{
  if (m_nsCursor) {
    if (!m_visibleMouse) {
      m_visibleMouse = true;
      [NSCursor unhide];
    }
    [m_nsCursor set];
  }
  else if (m_visibleMouse) {
    m_visibleMouse = false;
    [NSCursor hide];
  }
}

@end
