// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_FILENAME_FORMATTER_H_INCLUDED
#define APP_FILENAME_FORMATTER_H_INCLUDED
#pragma once

#include <string>

namespace app {

  std::string filename_formatter(
    const std::string& format,
    const std::string& filename,
    const std::string& layerName = "",
    int frame = -1,
    bool replaceFrame = true);

  std::string set_frame_format(
    const std::string& format,
    const std::string& newFrameFormat);

  std::string add_frame_format(
    const std::string& format,
    const std::string& newFrameFormat);

} // namespace app

#endif
