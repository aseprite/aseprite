// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

class OSXWindowImpl;

@interface OSXWindowDelegate : NSObject {
@private
  OSXWindowImpl* m_impl;
}
@end

@implementation OSXWindowDelegate

- (OSXWindowDelegate*)initWithWindowImpl:(OSXWindowImpl*)impl
{
  m_impl = impl;
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
