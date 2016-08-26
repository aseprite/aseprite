// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_IMAGE_CLASS_H_INCLUDED
#define APP_SCRIPT_IMAGE_CLASS_H_INCLUDED
#pragma once

#include "script/engine.h"

namespace app {

  void register_image_class(script::index_t idx, script::Context& ctx);

} // namespace app

#endif
