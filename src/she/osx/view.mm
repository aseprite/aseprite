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
#include "she/keys.h"
#include "she/osx/window.h"

using namespace she;

namespace {

KeyScancode cocoavk_to_scancode(UInt16 vk) {
  static KeyScancode keymap[256] = {
    // 0x00
    kKeyA, // 0x00 - kVK_ANSI_A
    kKeyS, // 0x01 - kVK_ANSI_S
    kKeyD, // 0x02 - kVK_ANSI_D
    kKeyF, // 0x03 - kVK_ANSI_F
    kKeyH, // 0x04 - kVK_ANSI_H
    kKeyG, // 0x05 - kVK_ANSI_G
    kKeyZ, // 0x06 - kVK_ANSI_Z
    kKeyX, // 0x07 - kVK_ANSI_X
    kKeyC, // 0x08 - kVK_ANSI_C
    kKeyV, // 0x09 - kVK_ANSI_V
    kKeyNil, // 0x0A - kVK_ISO_Section
    kKeyB, // 0x0B - kVK_ANSI_B
    kKeyQ, // 0x0C - kVK_ANSI_Q
    kKeyW, // 0x0D - kVK_ANSI_W
    kKeyE, // 0x0E - kVK_ANSI_E
    kKeyR, // 0x0F - kVK_ANSI_R
    // 0x10
    kKeyY, // 0x10 - kVK_ANSI_Y
    kKeyT, // 0x11 - kVK_ANSI_T
    kKey1, // 0x12 - kVK_ANSI_1
    kKey2, // 0x13 - kVK_ANSI_2
    kKey3, // 0x14 - kVK_ANSI_3
    kKey4, // 0x15 - kVK_ANSI_4
    kKey6, // 0x16 - kVK_ANSI_6
    kKey5, // 0x17 - kVK_ANSI_5
    kKeyEquals, // 0x18 - kVK_ANSI_Equal
    kKey9, // 0x19 - kVK_ANSI_9
    kKey7, // 0x1A - kVK_ANSI_7
    kKeyMinus, // 0x1B - kVK_ANSI_Minus
    kKey8, // 0x1C - kVK_ANSI_8
    kKey0, // 0x1D - kVK_ANSI_0
    kKeyClosebrace, // 0x1E - kVK_ANSI_RightBracket
    kKeyO, // 0x1F - kVK_ANSI_O
    // 0x20
    kKeyU, // 0x20 - kVK_ANSI_U
    kKeyOpenbrace, // 0x21 - kVK_ANSI_LeftBracket
    kKeyI, // 0x22 - kVK_ANSI_I
    kKeyP, // 0x23 - kVK_ANSI_P
    kKeyEnter, // 0x24 - kVK_Return
    kKeyL, // 0x25 - kVK_ANSI_L
    kKeyJ, // 0x26 - kVK_ANSI_J
    kKeyQuote, // 0x27 - kVK_ANSI_Quote
    kKeyK, // 0x28 - kVK_ANSI_K
    kKeySemicolon, // 0x29 - kVK_ANSI_Semicolon
    kKeyBackslash, // 0x2A - kVK_ANSI_Backslash
    kKeyComma, // 0x2B - kVK_ANSI_Comma
    kKeySlash, // 0x2C - kVK_ANSI_Slash
    kKeyN, // 0x2D - kVK_ANSI_N
    kKeyM, // 0x2E - kVK_ANSI_M
    kKeyStop, // 0x2F - kVK_ANSI_Period
    // 0x30
    kKeyTab, // 0x30 - kVK_Tab
    kKeySpace, // 0x31 - kVK_Space
    kKeyNil, // 0x32 - kVK_ANSI_Grave
    kKeyBackspace, // 0x33 - kVK_Delete
    kKeyNil, // 0x34 - ?
    kKeyEsc, // 0x35 - kVK_Escape
    kKeyNil, // 0x36 - ?
    kKeyCommand, // 0x37 - kVK_Command
    kKeyLShift, // 0x38 - kVK_Shift
    kKeyCapsLock, // 0x39 - kVK_CapsLock
    kKeyAlt, // 0x3A - kVK_Option
    kKeyLControl, // 0x3B - kVK_Control
    kKeyRShift, // 0x3C - kVK_RightShift
    kKeyAltGr, // 0x3D - kVK_RightOption
    kKeyRControl, // 0x3E - kVK_RightControl
    kKeyNil, // 0x3F - kVK_Function
    // 0x40
    kKeyNil, // 0x40 - kVK_F17
    kKeyNil, // 0x41 - kVK_ANSI_KeypadDecimal
    kKeyNil, // 0x42 - ?
    kKeyAsterisk, // 0x43 - kVK_ANSI_KeypadMultiply
    kKeyNil, // 0x44 - ?
    kKeyPlusPad, // 0x45 - kVK_ANSI_KeypadPlus
    kKeyNil, // 0x46 - ?
    kKeyDelPad, // 0x47 - kVK_ANSI_KeypadClear
    kKeyNil, // 0x48 - kVK_VolumeUp
    kKeyNil, // 0x49 - kVK_VolumeDown
    kKeyNil, // 0x4A - kVK_Mute
    kKeySlashPad, // 0x4B - kVK_ANSI_KeypadDivide
    kKeyEnterPad, // 0x4C - kVK_ANSI_KeypadEnter
    kKeyNil, // 0x4D - ?
    kKeyMinusPad, // 0x4E - kVK_ANSI_KeypadMinus
    kKeyNil, // 0x4F - kVK_F18
    // 0x50
    kKeyNil, // 0x50 - kVK_F19
    kKeyEqualsPad, // 0x51 - kVK_ANSI_KeypadEquals
    kKey0Pad, // 0x52 - kVK_ANSI_Keypad0
    kKey1Pad, // 0x53 - kVK_ANSI_Keypad1
    kKey2Pad, // 0x54 - kVK_ANSI_Keypad2
    kKey3Pad, // 0x55 - kVK_ANSI_Keypad3
    kKey4Pad, // 0x56 - kVK_ANSI_Keypad4
    kKey5Pad, // 0x57 - kVK_ANSI_Keypad5
    kKey6Pad, // 0x58 - kVK_ANSI_Keypad6
    kKey7Pad, // 0x59 - kVK_ANSI_Keypad7
    kKeyNil, // 0x5A - kVK_F20
    kKey8Pad, // 0x5B - kVK_ANSI_Keypad8
    kKey9Pad, // 0x5C - kVK_ANSI_Keypad9
    kKeyYen, // 0x5D - kVK_JIS_Yen
    kKeyNil, // 0x5E - kVK_JIS_Underscore
    kKeyNil, // 0x5F - kVK_JIS_KeypadComma
    // 0x60
    kKeyF5, // 0x60 - kVK_F5
    kKeyF6, // 0x61 - kVK_F6
    kKeyF7, // 0x62 - kVK_F7
    kKeyF3, // 0x63 - kVK_F3
    kKeyF8, // 0x64 - kVK_F8
    kKeyF9, // 0x65 - kVK_F9
    kKeyNil, // 0x66 - kVK_JIS_Eisu
    kKeyF11, // 0x67 - kVK_F11
    kKeyKana, // 0x68 - kVK_JIS_Kana
    kKeyNil, // 0x69 - kVK_F13
    kKeyNil, // 0x6A - kVK_F16
    kKeyNil, // 0x6B - kVK_F14
    kKeyNil, // 0x6C - ?
    kKeyF10, // 0x6D - kVK_F10
    kKeyNil, // 0x6E - ?
    kKeyF12, // 0x6F - kVK_F12
    // 0x70
    kKeyNil, // 0x70 - ?
    kKeyNil, // 0x71 - kVK_F15
    kKeyNil, // 0x72 - kVK_Help
    kKeyHome, // 0x73 - kVK_Home
    kKeyPageUp, // 0x74 - kVK_PageUp
    kKeyDel, // 0x75 - kVK_ForwardDelete
    kKeyF4, // 0x76 - kVK_F4
    kKeyEnd, // 0x77 - kVK_End
    kKeyF2, // 0x78 - kVK_F2
    kKeyPageDown, // 0x79 - kVK_PageDown
    kKeyF1, // 0x7A - kVK_F1
    kKeyLeft, // 0x7B - kVK_LeftArrow
    kKeyRight, // 0x7C - kVK_RightArrow
    kKeyDown, // 0x7D - kVK_DownArrow
    kKeyUp, // 0x7E - kVK_UpArrow
    kKeyNil // 0x7F - ?
  };
  if (vk < 0 || vk > 127)
    vk = 0;
  return keymap[vk];
}

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
#if 0 // Avoid beeps TODO comment this when the OS X menus are ready
  [super keyDown:event];
#endif

  Event ev;
  ev.setType(Event::KeyDown);
  ev.setScancode(cocoavk_to_scancode(event.keyCode));
  ev.setRepeat(event.ARepeat ? 1: 0);

  if (event.characters &&
      event.characters.length >= 1) {
    ev.setUnicodeChar([event.characters characterAtIndex:0]);
  }

  queue_event(ev);
}

- (void)keyUp:(NSEvent*)event
{
#if 0
  [super keyUp:event];
#endif

  Event ev;
  ev.setType(Event::KeyUp);
  ev.setScancode(cocoavk_to_scancode(event.keyCode));
  ev.setRepeat(event.ARepeat ? 1: 0);

  if (event.characters &&
      event.characters.length >= 1) {
    ev.setUnicodeChar([event.characters characterAtIndex:0]);
  }

  queue_event(ev);
}

- (void)flagsChanged:(NSEvent*)event
{
  [super flagsChanged:event];
}

- (void)mouseEntered:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseEnter);
  ev.setPosition(get_local_mouse_pos(self, event));
  queue_event(ev);
}

- (void)mouseMoved:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  queue_event(ev);
}

- (void)mouseExited:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseLeave);
  ev.setPosition(get_local_mouse_pos(self, event));
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
  ev.setType(Event::MouseDown);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  queue_event(ev);
}

- (void)handleMouseUp:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseUp);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  queue_event(ev);
}

- (void)handleMouseDragged:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
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
