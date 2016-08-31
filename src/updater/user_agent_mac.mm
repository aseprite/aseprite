// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>

void getMacOSXVersion(int* major, int* minor, int* bugFix)
{
  OSErr err;
  SInt32 systemVersion, versionMajor, versionMinor, versionBugFix;
  if (Gestalt(gestaltSystemVersion, &systemVersion) != noErr) goto fail;
  if (systemVersion < 0x1040) {
    if (major) *major = ((systemVersion & 0xF000) >> 12) * 10 + ((systemVersion & 0x0F00) >> 8);
    if (minor) *minor = (systemVersion & 0x00F0) >> 4;
    if (bugFix) *bugFix = (systemVersion & 0x000F);
  }
  else {
    if ((err = Gestalt(gestaltSystemVersionMajor, &versionMajor)) != noErr) goto fail;
    if ((err = Gestalt(gestaltSystemVersionMinor, &versionMinor)) != noErr) goto fail;
    if ((err = Gestalt(gestaltSystemVersionBugFix, &versionBugFix)) != noErr) goto fail;
    if (major) *major = versionMajor;
    if (minor) *minor = versionMinor;
    if (bugFix) *bugFix = versionBugFix;
  }
  return;

fail:
  if (major) *major = 10;
  if (minor) *minor = 0;
  if (bugFix) *bugFix = 0;
}
