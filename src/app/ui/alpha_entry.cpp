// Aseprite UI Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/pref/preferences.h"
#include "app/ui/alpha_entry.h"
#include "base/scoped_value.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "os/font.h"
#include "ui/fit_bounds.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/popup_window.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/slider.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>

namespace app {

using namespace gfx;

AlphaEntry::AlphaEntry(AlphaSlider::Type type) : IntEntry(0, 255)
{
  m_slider = std::make_unique<AlphaSlider>(0, type);
  m_slider->setFocusStop(false); // In this way the IntEntry doesn't lost the focus
  m_slider->setTransparent(true);
  m_slider->Change.connect([this] { this->onChangeSlider(); });
}

int AlphaEntry::getValue() const
{
  int value = m_slider->convertTextToValue(text());
  if (static_cast<AlphaSlider*>(m_slider.get())->getAlphaRange() ==
      app::gen::AlphaRange::PERCENTAGE)
    value = std::round(((double)m_slider->getMaxValue()) * ((double)value) / ((double)100));

  return std::clamp(value, m_min, m_max);
}

void AlphaEntry::setValue(int value)
{
  value = std::clamp(value, m_min, m_max);

  if (m_popupWindow && !m_changeFromSlider)
    m_slider->setValue(value);

  if (static_cast<AlphaSlider*>(m_slider.get())->getAlphaRange() ==
      app::gen::AlphaRange::PERCENTAGE)
    value = std::round(((double)100) * ((double)value) / ((double)m_slider->getMaxValue()));

  setText(m_slider->convertValueToText(value));

  onValueChange();
}

} // namespace app
