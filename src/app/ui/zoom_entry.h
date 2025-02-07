// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_ZOOM_ENTRY_H_INCLUDED
#define APP_UI_ZOOM_ENTRY_H_INCLUDED
#pragma once

#include "render/zoom.h"
#include "ui/int_entry.h"
#include "ui/slider.h"

namespace app {

class ZoomEntry : public ui::IntEntry,
                  public ui::SliderDelegate {
public:
  ZoomEntry();

  void setZoom(const render::Zoom& zoom);

  obs::signal<void(const render::Zoom&)> ZoomChange;

private:
  // SliderDelegate impl
  std::string onGetTextFromValue(int value) override;
  int onGetValueFromText(const std::string& text) override;

  void onValueChange() override;

  bool m_locked;
};

} // namespace app

#endif
