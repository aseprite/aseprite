// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/alpha_slider.h"
#include "ui/message.h"

#include <algorithm>

using namespace ui;
using namespace app::gen;

namespace app {

AlphaSlider::AlphaSlider(int value, Type type)
  : ui::Slider(0, 255, value, this)
  , m_type(type)
{
}

void AlphaSlider::getSliderThemeInfo(int* min, int* max, int* value) const
{
  switch (getAlphaRange()) {
    case AlphaRange::PERCENTAGE:
      if (min) *min = 0;
      if (max) *max = 100;
      if (value) *value = std::round(((double)100)*((double)getValue())/((double)getMaxValue()));

      return;
    case AlphaRange::EIGHT_BIT:
      return Slider::getSliderThemeInfo(min, max, value);
  }
}

void AlphaSlider::updateValue(int value)
{
  if (getAlphaRange() == AlphaRange::PERCENTAGE)
    value = std::round(((double)getMaxValue())*((double)value)/((double)100));

  setValue(value);
}

std::string AlphaSlider::onGetTextFromValue(int value)
{
  char buf[128];
  const char *format = "%d";
  if (getAlphaRange() == AlphaRange::PERCENTAGE)
      format = "%d%%";

  std::snprintf(buf, sizeof(buf), format, value);
  return buf;
}

int AlphaSlider::onGetValueFromText(const std::string& text)
{
  std::string str = text;
  if (getAlphaRange() == AlphaRange::PERCENTAGE)
      str.erase(std::remove(str.begin(), str.end(), '%'), str.end());

  return std::strtol(str.c_str(), nullptr, 10);
}

}  // namespace app
