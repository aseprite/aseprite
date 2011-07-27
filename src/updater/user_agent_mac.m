/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

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
