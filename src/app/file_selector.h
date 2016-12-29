// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_SELECTOR_H_INCLUDED
#define APP_FILE_SELECTOR_H_INCLUDED
#pragma once

#include <string>

#include "doc/pixel_ratio.h"

namespace ui {
  class ComboBox;
}

namespace app {

  enum class FileSelectorType { Open, Save };

  class FileSelectorDelegate {
  public:
    virtual ~FileSelectorDelegate() { }
    virtual bool hasResizeCombobox() = 0;
    virtual double getResizeScale() = 0;
    virtual void setResizeScale(double scale) = 0;

    // TODO refactor this to avoid using a ui::ComboBox,
    //      mainly to re-use this in the native file selector
    virtual void fillLayersComboBox(ui::ComboBox* layers) = 0;
    virtual void fillFramesComboBox(ui::ComboBox* frames) = 0;
    virtual std::string getLayers() = 0;
    virtual std::string getFrames() = 0;
    virtual void setLayers(const std::string& layers) = 0;
    virtual void setFrames(const std::string& frames) = 0;

    virtual void setApplyPixelRatio(bool applyPixelRatio) = 0;
    virtual bool applyPixelRatio() const = 0;
    virtual doc::PixelRatio pixelRatio() = 0;
  };

  std::string show_file_selector(
    const std::string& title,
    const std::string& initialPath,
    const std::string& showExtensions,
    FileSelectorType type,
    FileSelectorDelegate* delegate = nullptr);

} // namespace app

#endif
