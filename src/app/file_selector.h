// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_FILE_SELECTOR_H_INCLUDED
#define APP_FILE_SELECTOR_H_INCLUDED
#pragma once

#include <string>

namespace app {

  enum class FileSelectorType { Open, Save };

  class FileSelectorDelegate {
  public:
    virtual ~FileSelectorDelegate() { }
    virtual bool hasResizeCombobox() = 0;
    virtual double getResizeScale() = 0;
    virtual void setResizeScale(double scale) = 0;
  };

  std::string show_file_selector(
    const std::string& title,
    const std::string& initialPath,
    const std::string& showExtensions,
    FileSelectorType type,
    FileSelectorDelegate* delegate = nullptr);

} // namespace app

#endif
