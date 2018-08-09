// LAF OS Library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/osx/window.h"

#include "base/debug.h"
#include "os/event.h"
#include "os/osx/event_queue.h"
#include "os/osx/view.h"
#include "os/osx/window_delegate.h"
#include "os/surface.h"

using namespace os;

@implementation OSXWindow

- (OSXWindow*)initWithImpl:(OSXWindowImpl*)impl
                     width:(int)width
                    height:(int)height
                     scale:(int)scale
{
  m_impl = impl;
  m_scale = scale;

  NSRect rect = NSMakeRect(0, 0, width, height);
  self = [self initWithContentRect:rect
                         styleMask:(NSWindowStyleMaskTitled |
                                    NSWindowStyleMaskClosable |
                                    NSWindowStyleMaskMiniaturizable |
                                    NSWindowStyleMaskResizable)
                           backing:NSBackingStoreBuffered
                             defer:NO];
  if (!self)
    return nil;

  m_delegate = [[OSXWindowDelegate alloc] initWithWindowImpl:impl];

  // The NSView width and height will be a multiple of 4. In this way
  // all scaled pixels should be exactly the same
  // for Screen Scaling > 1 and <= 4)
  self.contentResizeIncrements = NSMakeSize(4, 4);

  OSXView* view = [[OSXView alloc] initWithFrame:rect];
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

  [self setDelegate:m_delegate];
  [self setContentView:view];
  [self center];
  [self makeKeyAndOrderFront:self];

  // Hide the "View > Show Tab Bar" menu item
  if ([self respondsToSelector:@selector(setTabbingMode:)])
    [self setTabbingMode:NSWindowTabbingModeDisallowed];

  return self;
}

- (OSXWindowImpl*)impl
{
  return m_impl;
}

- (int)scale
{
  return m_scale;
}

- (void)setScale:(int)scale
{
  m_scale = scale;

  if (m_impl) {
    NSRect bounds = [[self contentView] bounds];
    m_impl->onResize(gfx::Size(bounds.size.width,
                               bounds.size.height));
  }
}

- (gfx::Size)clientSize
{
  return gfx::Size([[self contentView] frame].size.width,
                   [[self contentView] frame].size.height);
}

- (gfx::Size)restoredSize
{
  return [self clientSize];
}

- (void)setMousePosition:(const gfx::Point&)position
{
   NSView* view = self.contentView;
   NSPoint pt = NSMakePoint(
     position.x*m_scale,
     view.frame.size.height - position.y*m_scale);

   pt = [view convertPoint:pt toView:view];
   pt = [view convertPoint:pt toView:nil];
   pt = [self convertBaseToScreen:pt];
   pt.y = [[self screen] frame].size.height - pt.y;

   CGPoint pos = CGPointMake(pt.x, pt.y);
   CGEventRef event = CGEventCreateMouseEvent(
     NULL, kCGEventMouseMoved, pos, kCGMouseButtonLeft);
   CGEventPost(kCGHIDEventTap, event);
   CFRelease(event);
}

- (BOOL)setNativeMouseCursor:(NativeCursor)cursor
{
  NSCursor* nsCursor = nullptr;

  switch (cursor) {
    case kArrowCursor:
    case kWaitCursor:
    case kHelpCursor:
    case kSizeNECursor:
    case kSizeNWCursor:
    case kSizeSECursor:
    case kSizeSWCursor:
      nsCursor = [NSCursor arrowCursor];
      break;
    case kCrosshairCursor:
      nsCursor = [NSCursor crosshairCursor];
      break;
    case kIBeamCursor:
      nsCursor = [NSCursor IBeamCursor];
      break;
    case kLinkCursor:
      nsCursor = [NSCursor pointingHandCursor];
      break;
    case kForbiddenCursor:
      nsCursor = [NSCursor operationNotAllowedCursor];
      break;
    case kMoveCursor:
      nsCursor = [NSCursor openHandCursor];
      break;
    case kSizeNSCursor:
      nsCursor = [NSCursor resizeUpDownCursor];
      break;
    case kSizeWECursor:
      nsCursor = [NSCursor resizeLeftRightCursor];
      break;
    case kSizeNCursor:
      nsCursor = [NSCursor resizeUpCursor];
      break;
    case kSizeECursor:
      nsCursor = [NSCursor resizeRightCursor];
      break;
    case kSizeSCursor:
      nsCursor = [NSCursor resizeDownCursor];
      break;
    case kSizeWCursor:
      nsCursor = [NSCursor resizeLeftCursor];
      break;
  }

  [self.contentView setCursor:nsCursor];
  return (nsCursor ? YES: NO);
}

- (BOOL)setNativeMouseCursor:(const os::Surface*)surface
                       focus:(const gfx::Point&)focus
                       scale:(const int)scale
{
  ASSERT(surface);
  SurfaceFormatData format;
  surface->getFormat(&format);
  if (format.bitsPerPixel != 32)
    return NO;

  const int w = scale*surface->width();
  const int h = scale*surface->height();

  if (4*w*h == 0)
    return NO;

  @autoreleasepool {
    NSBitmapImageRep* bmp =
      [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:nil
                      pixelsWide:w
                      pixelsHigh:h
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSDeviceRGBColorSpace
                    bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                     bytesPerRow:w*4
                    bitsPerPixel:32];
    if (!bmp)
      return NO;

    uint32_t* dst = (uint32_t*)[bmp bitmapData];
    for (int y=0; y<h; ++y) {
      const uint32_t* src = (const uint32_t*)surface->getData(0, y/scale);
      for (int x=0, u=0; x<w; ++x, ++dst) {
        *dst = *src;
        if (++u == scale) {
          u = 0;
          ++src;
        }
      }
    }

    NSImage* img = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];
    if (!img)
      return NO;

    [img addRepresentation:bmp];

    NSCursor* nsCursor =
      [[NSCursor alloc] initWithImage:img
                              hotSpot:NSMakePoint(scale*focus.x + scale/2,
                                                  scale*focus.y + scale/2)];
    if (!nsCursor)
      return NO;

    [self.contentView setCursor:nsCursor];
    return YES;
  }
}

- (void)noResponderFor:(SEL)eventSelector
{
  if (eventSelector == @selector(keyDown:)) {
    // Do nothing (avoid beep)
  }
  else {
    [super noResponderFor:eventSelector];
  }
}

@end
