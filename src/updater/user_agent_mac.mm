// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>

void getMacOSXVersion(int& major, int& minor, int& patch)
{
  major = 10;
  minor = 0;
  patch = 0;

  NSProcessInfo* info = [NSProcessInfo processInfo];
  if ([info respondsToSelector:@selector(operatingSystemVersion)]) {
    NSOperatingSystemVersion osVer = [info operatingSystemVersion];
    major = osVer.majorVersion;
    minor = osVer.minorVersion;
    patch = osVer.patchVersion;
  }
  else {
    SInt32 systemVersion, majorVersion, minorVersion, patchVersion;
    if (Gestalt(gestaltSystemVersion, &systemVersion) != noErr)
      return;
    if (systemVersion < 0x1040) {
      major = ((systemVersion & 0xF000) >> 12) * 10 + ((systemVersion & 0x0F00) >> 8);
      minor = (systemVersion & 0x00F0) >> 4;
      patch = (systemVersion & 0x000F);
    }
    else if (Gestalt(gestaltSystemVersionMajor, &majorVersion) == noErr &&
             Gestalt(gestaltSystemVersionMinor, &minorVersion) == noErr &&
             Gestalt(gestaltSystemVersionBugFix, &patchVersion) == noErr) {
      major = majorVersion;
      minor = minorVersion;
      patch = patchVersion;
    }
  }
}
