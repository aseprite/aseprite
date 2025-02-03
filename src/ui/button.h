// Aseprite UI Library
// Copyright (C) 2021-2025  Igara Studio S.A.
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
  obs::signal<void()> Click;
  obs::signal<void()> RightClick;

protected:
  // Events
  bool onProcessMessage(Message* msg) override;

  // New events
  virtual void onClick();
  virtual void onRightClick();
  virtual void onStartDrag();
  virtual void onSelectWhenDragging();

private:
  void generateButtonSelectSignal();

  WidgetType m_behaviorType;
  bool m_pressedStatus;
  // Flag used to prevent nested calls of the onClik handler.
  // This situation can happen, for example, when a button in a window
  // opens a modal window. If the user presses the button as fast as possible
  // it may happen that the button's onClick handler is called in the window's
  // event loop, then the handler opens the modal and, finally a second onClick handler
  // is called due to queued system events that now are processed by the modal's event
  // loop that was started by the previously in course onClick handler.
  bool m_clicking = false;

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
  CheckBox(const std::string& text, const WidgetType drawType = kCheckWidget);
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
