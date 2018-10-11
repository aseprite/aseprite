// Aseprite UI Library
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_BUTTON_H_INCLUDED
#define UI_BUTTON_H_INCLUDED
#pragma once

#include "obs/signal.h"
#include "ui/widget.h"

#include <vector>

namespace os {
  class Surface;
}

namespace ui {

  class Event;

  // Generic button
  class ButtonBase : public Widget {
  public:
    ButtonBase(const std::string& text,
               const WidgetType type,
               const WidgetType behaviorType,
               const WidgetType drawType);
    virtual ~ButtonBase();

    WidgetType behaviorType() const;
    // Signals
    obs::signal<void(Event&)> Click;

  protected:
    // Events
    bool onProcessMessage(Message* msg) override;

    // New events
    virtual void onClick(Event& ev);

  private:
    void generateButtonSelectSignal();

    bool m_pressedStatus;
    WidgetType m_behaviorType;

  protected:
    bool m_handleSelect;
  };

  // Pushable button to execute commands
  class Button : public ButtonBase {
  public:
    Button(const std::string& text);
  };

  // Check boxes
  class CheckBox : public ButtonBase {
  public:
    CheckBox(const std::string& text,
             const WidgetType drawType = kCheckWidget);
  };

  // Radio buttons
  class RadioButton : public ButtonBase {
  public:
    RadioButton(const std::string& text,
                const int radioGroup = 0,
                const WidgetType drawType = kRadioWidget);

    int getRadioGroup() const;
    void setRadioGroup(int radioGroup);

    void deselectRadioGroup();

  protected:
    void onSelect(bool selected) override;

  private:
    int m_radioGroup;
  };

} // namespace ui

#endif
