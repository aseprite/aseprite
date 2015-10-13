// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <Cocoa/Cocoa.h>

#include "she/osx/clipboard.h"

namespace she {

void ClipboardOSX::dispose()
{
  delete this;
}

std::string ClipboardOSX::getText(DisplayHandle display)
{
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  NSString* string = [pasteboard stringForType:NSStringPboardType];
  if (string)
    return std::string([string UTF8String]);
  else
    return std::string();
}

void ClipboardOSX::setText(DisplayHandle display, const std::string& text)
{
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  [pasteboard clearContents];
  [pasteboard setString:[NSString stringWithUTF8String:text.c_str()]
                forType:NSStringPboardType];
}

} // namespace she
