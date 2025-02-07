// Aseprite
// Copyright (c) 2020-2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SERIAL_FORMAT_H_INCLUDED
#define DOC_SERIAL_FORMAT_H_INCLUDED
#pragma once

#include "base/ints.h"

namespace doc {

// Format version of raw document/sprite data when it's serialized in
// IO operations for undo/redo or in recovery data to allow backward
// compatibility with old session backups.
enum class SerialFormat : uint16_t {
  Ver0 = 0, // Old version
  Ver1 = 1, // New version with tilesets
  Ver2 = 2, // Version 2 adds custom properties to user data
  LastVer = Ver2
};

} // namespace doc

#endif
