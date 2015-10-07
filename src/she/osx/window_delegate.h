// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

@interface OSXWindowDelegate : NSObject
{
  OSXWindowImpl* m_impl;
}
- (OSXWindowDelegate*)initWithWindow:(OSXWindow*)window;
- (BOOL)windowShouldClose:(id)sender;
- (void)windowWillClose:(NSNotification *)notification;
- (void)windowDidResize:(NSNotification*)notification;
- (void)windowDidMiniaturize:(NSNotification*)notification;
@end

@implementation OSXWindowDelegate

- (OSXWindowDelegate*)initWithWindow:(OSXWindow*)window
{
  m_impl = [window impl];
  return self;
}

- (BOOL)windowShouldClose:(id)sender
{
  she::Event ev;
  ev.setType(she::Event::CloseDisplay);
  //ev.setDisplay(nullptr);             // TODO
  she::queue_event(ev);
  return NO;
}

- (void)windowWillClose:(NSNotification*)notification
{
  m_impl->onClose();
}

- (void)windowDidResize:(NSNotification*)notification
{
}

- (void)windowDidMiniaturize:(NSNotification*)notification
{
}

@end
