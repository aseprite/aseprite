// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Foundation/Foundation.h>

#include <string>

namespace base {

std::string get_lib_app_support_path()
{
  NSArray* dirs = NSSearchPathForDirectoriesInDomains(
    NSApplicationSupportDirectory, NSUserDomainMask, YES);
  if (dirs) {
    NSString* dir = [dirs firstObject];
    if (dir)
      return std::string([dir UTF8String]);
  }
  return std::string();
}

} // namespace base
