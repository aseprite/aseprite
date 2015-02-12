// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_SHADING_OPTIONS_H_INCLUDED
#define APP_TOOLS_SHADING_OPTIONS_H_INCLUDED
#pragma once

namespace app {
  namespace tools {
    class ShadeTable8;

    class ShadingOptions {
    public:
      virtual ~ShadingOptions() { }
      virtual ShadeTable8* getShadeTable() = 0;
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_SHADING_OPTIONS_H_INCLUDED
