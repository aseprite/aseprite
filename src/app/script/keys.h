// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_KEYS_H_INCLUDED
#define APP_SCRIPT_KEYS_H_INCLUDED
#pragma once

#include "ui/keys.h"

namespace app {
namespace script {

  const char* vkcode_to_code(const ui::KeyScancode vkcode);

} // namespace script
} // namespace app

#endif
