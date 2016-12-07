// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_ALLEG4_SCANCODE_H_INCLUDED
#define SHE_ALLEG4_SCANCODE_H_INCLUDED
#pragma once

#include "she/keys.h"

namespace she {

KeyScancode alleg_to_she_scancode(int scancode);
int she_to_alleg_scancode(KeyScancode scancode);

} // namespace she

#endif
