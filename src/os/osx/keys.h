// LAF OS Library
// Copyright (C) 2017-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_KEYS_H_INCLUDED
#define OS_OSX_KEYS_H_INCLUDED
#pragma once

#include "os/keys.h"

#include <Cocoa/Cocoa.h>

namespace os {

  KeyScancode scancode_from_nsevent(NSEvent* event);

  CFStringRef get_unicode_from_key_code(const UInt16 keyCode,
                                        const NSEventModifierFlags modifierFlags,
                                        UInt32* deadKeyState = nullptr);

}

#endif
