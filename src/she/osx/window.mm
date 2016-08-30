// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/osx/window.h"

#include "she/event.h"
#include "she/osx/event_queue.h"
#include "she/osx/view.h"
#include "she/osx/window_delegate.h"
#include "she/surface.h"

using namespace she;

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
                         styleMask:(NSTitledWindowMask | NSClosableWindowMask |
                                    NSMiniaturizableWindowMask | NSResizableWindowMask)
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

- (BOOL)setNativeMouseCursor:(const she::Surface*)surface
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

  std::vector<uint32_t> buf(4*w*h);
  uint32_t* bits = &buf[0];

  for (int y=0; y<h; ++y) {
    const uint32_t* ptr = (const uint32_t*)surface->getData(0, y/scale);
    for (int x=0, u=0; x<w; ++x, ++bits) {
      *bits = *ptr;
      if (++u == scale) {
        u = 0;
        ++ptr;
      }
    }
  }

  CGDataProviderRef dataRef =
    CGDataProviderCreateWithData(nullptr, &buf[0],
                                 w*h*4, nullptr);
  if (!dataRef)
    return NO;

  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGImageRef img =
    CGImageCreate(
      w, h, 8, 32, 4*w,
      colorSpace, kCGImageAlphaLast, dataRef,
      nullptr, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(colorSpace);

  CGDataProviderRelease(dataRef);

  NSCursor* nsCursor = nullptr;
  if (img) {
    NSImage* image =
      [[NSImage new] initWithCGImage:img
                                size:NSMakeSize(w, h)];
    CGImageRelease(img);

    nsCursor =
      [[NSCursor new] initWithImage:image
                            hotSpot:NSMakePoint(scale*focus.x,
                                                scale*focus.y)];
  }
  if (nsCursor)
    [self.contentView setCursor:nsCursor];
  return (nsCursor ? YES: NO);
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
