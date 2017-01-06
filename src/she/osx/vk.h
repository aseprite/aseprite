// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_VK_H_INCLUDED
#define SHE_OSX_VK_H_INCLUDED
#pragma once

#include "she/keys.h"

#include <Cocoa/Cocoa.h>

namespace she {

  KeyScancode cocoavk_to_scancode(UInt16 keyCode,
                                  const NSEventModifierFlags modifierFlags);

  CFStringRef get_unicode_from_key_code(const UInt16 keyCode,
                                        const NSEventModifierFlags modifierFlags,
                                        UInt32* deadKeyState = nullptr);

}

#endif
