// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_INK_TYPE_H_INCLUDED
#define APP_TOOLS_INK_TYPE_H_INCLUDED
#pragma once

#include <string>

namespace app {
namespace tools {

  enum class InkType {
    DEFAULT = 0,
    SIMPLE = 0,
    ALPHA_COMPOSITING = 1,
    COPY_COLOR = 2,
    LOCK_ALPHA = 3,
    SHADING = 4,
  };

  inline bool inkHasOpacity(InkType inkType) {
    return (inkType == InkType::ALPHA_COMPOSITING ||
            inkType == InkType::LOCK_ALPHA);
  }

  std::string ink_type_to_string(InkType inkType);

} // namespace tools
} // namespace app

#endif
