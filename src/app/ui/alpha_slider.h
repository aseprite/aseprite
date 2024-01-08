// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_ALPHA_SLIDER_H_INCLUDED
#define APP_UI_ALPHA_SLIDER_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "ui/slider.h"

namespace app {

class AlphaSlider : public ui::Slider,
                    ui::SliderDelegate
{
public:
  enum Type {ALPHA, OPACITY};

  AlphaSlider(int value, Type type);

  void getSliderThemeInfo(int* min, int* max, int* value) const override;
  void updateValue(int value) override;

  app::gen::AlphaRange getAlphaRange() const {
    return (m_type == ALPHA ? Preferences::instance().range.alpha()
                            : Preferences::instance().range.opacity());
  }

private:
  // ui::SliderDelegate impl
  std::string onGetTextFromValue(int value) override;
  int onGetValueFromText(const std::string& text) override;

  Type m_type;
};

}

#endif
