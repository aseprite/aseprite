// Aseprite
// Copyright (c) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SLIDERS_H_INCLUDED
#define APP_UI_COLOR_SLIDERS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "obs/connection.h"
#include "obs/signal.h"
#include "ui/event.h"
#include "ui/grid.h"
#include "ui/widget.h"

#include <vector>

namespace ui {
  class Box;
  class Label;
  class Slider;
  class Entry;
}

namespace app {

  class ColorSlidersChangeEvent;

  class ColorSliders : public ui::Widget {
  public:
    enum Channel { Red, Green, Blue,
                   HsvHue, HsvSaturation, HsvValue,
                   HslHue, HslSaturation, HslLightness,
                   Gray, Alpha,
                   Channels };
    enum Mode { Absolute, Relative };

    ColorSliders();

    void setColor(const app::Color& color);
    void setColorType(const app::Color::Type type);
    void setColorTypes(const std::vector<app::Color::Type>& types);
    void setMode(Mode mode);
    void resetRelativeSliders();

    int getAbsSliderValue(const Channel i) const;
    int getRelSliderValue(const Channel i) const;
    void syncRelHsvHslSliders();

    // Signals
    obs::signal<void(ColorSlidersChangeEvent&)> ColorChange;

  private:
    void onSizeHint(ui::SizeHintEvent& ev) override;

    // For derived classes
    void addSlider(const Channel channel,
                   const char* labelText,
                   const int absMin, const int absMax,
                   const int relMin, const int relMax);
    void setAbsSliderValue(const Channel i, int value);
    void setRelSliderValue(const Channel i, int value);

    void updateSlidersVisibility();
    void onSetColor(const app::Color& color);
    app::Color getColorFromSliders(const Channel channel) const;
    void onSliderChange(const Channel i);
    void onEntryChange(const Channel i);
    void onControlChange(const Channel i);

    void updateEntryText(const Channel i);
    void updateSlidersBgColor();
    void updateSliderBgColor(ui::Slider* slider, const app::Color& color);

    struct Item {
      bool show = false;
      ui::Label* label = nullptr;
      ui::Box* box = nullptr;
      ui::Slider* absSlider = nullptr;
      ui::Slider* relSlider = nullptr;
      ui::Entry* entry = nullptr;
    };

    std::vector<Item> m_items;
    ui::Grid m_grid;
    Mode m_mode;
    int m_lockSlider;
    int m_lockEntry;
    app::Color m_color;
    obs::scoped_connection m_appConn;
  };

  //////////////////////////////////////////////////////////////////////
  // Events

  class ColorSlidersChangeEvent : public ui::Event {
  public:
    ColorSlidersChangeEvent(ColorSliders::Channel channel,
                            ColorSliders::Mode mode,
                            const app::Color& color,
                            int delta,
                            ui::Component* source)
      : Event(source)
      , m_channel(channel)
      , m_mode(mode)
      , m_color(color)
      , m_delta(delta) {
    }

    ColorSliders::Channel channel() const { return m_channel; }
    ColorSliders::Mode mode() const { return m_mode; }
    app::Color color() const { return m_color; }
    int delta() const { return m_delta; }

  private:
    ColorSliders::Channel m_channel;
    ColorSliders::Mode m_mode;
    app::Color m_color;
    int m_delta;
  };

} // namespace app

#endif
