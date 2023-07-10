// Aseprite UI Library
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/button.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <queue>
#include <cstring>

namespace ui {

ButtonBase::ButtonBase(const std::string& text,
                       const WidgetType type,
                       const WidgetType behaviorType,
                       const WidgetType drawType)
  : Widget(type)
  , m_pressedStatus(false)
  , m_behaviorType(behaviorType)
  , m_handleSelect(true)
{
  setAlign(CENTER | MIDDLE);
  setText(text);
  setFocusStop(true);

  // Initialize theme
  setType(drawType);            // TODO Fix this nasty trick
  initTheme();
  setType(type);
}

ButtonBase::~ButtonBase()
{
}

WidgetType ButtonBase::behaviorType() const
{
  return m_behaviorType;
}

void ButtonBase::onClick()
{
  // Fire Click() signal
  Click();
}

bool ButtonBase::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
    case kFocusLeaveMessage:
      if (isEnabled()) {
        if (m_behaviorType == kButtonWidget) {
          // Deselect the widget (maybe the user press the key, but
          // before release it, changes the focus).
          if (isSelected())
            setSelected(false);
        }

        // TODO theme specific stuff
        invalidate();
      }
      break;

    case kKeyDownMessage: {
      KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
      KeyScancode scancode = keymsg->scancode();

      if (isEnabled() && isVisible()) {
        const bool mnemonicPressed =
          (mnemonic() &&
           (!mnemonicRequiresModifiers() ||
            msg->altPressed() ||
            msg->cmdPressed()) &&
           isMnemonicPressed(keymsg));

        // For kButtonWidget
        if (m_behaviorType == kButtonWidget) {
          // Has focus and press enter/space
          if (hasFocus()) {
            if ((scancode == kKeyEnter) ||
                (scancode == kKeyEnterPad) ||
                (scancode == kKeySpace)) {
              setSelected(true);
              return true;
            }
          }

          if (// Check if the user pressed mnemonic
              mnemonicPressed ||
              // Magnetic widget catches ENTERs
              (isFocusMagnet() &&
               ((scancode == kKeyEnter) ||
                (scancode == kKeyEnterPad)))) {
            manager()->setFocus(this);

            // Dispatch focus movement messages (because the buttons
            // process them)
            manager()->dispatchMessages();

            setSelected(true);
            return true;
          }
        }
        // For kCheckWidget or kRadioWidget
        else {
          // If the widget has the focus and the user press space or
          // if the user press Alt+the underscored letter of the button
          if ((hasFocus() && (scancode == kKeySpace)) || mnemonicPressed) {
            if (m_behaviorType == kCheckWidget) {
              // Swap the select status
              setSelected(!isSelected());
              invalidate();
            }
            else if (m_behaviorType == kRadioWidget) {
              if (!isSelected()) {
                setSelected(true);
              }
            }
            return true;
          }
        }
      }
      break;
    }

    case kKeyUpMessage:
      if (isEnabled() && hasFocus()) {
        switch (m_behaviorType) {

          case kButtonWidget:
            if (isSelected()) {
              generateButtonSelectSignal();
              return true;
            }
            break;

          case kCheckWidget: {
            // Fire onClick() event
            onClick();
            return true;
          }

        }
      }
      break;

    case kMouseDownMessage:
      switch (m_behaviorType) {

        case kButtonWidget:
          if (isEnabled()) {
            setSelected(true);

            m_pressedStatus = isSelected();
            captureMouse();

            onStartDrag();
          }
          return true;

        case kCheckWidget:
          if (isEnabled()) {
            setSelected(!isSelected());

            m_pressedStatus = isSelected();
            captureMouse();

            onStartDrag();
          }
          return true;

        case kRadioWidget:
          if (isEnabled()) {
            if (!isSelected()) {
              m_handleSelect = false;
              setSelected(true);
              m_handleSelect = true;

              m_pressedStatus = isSelected();
              captureMouse();

              onStartDrag();
            }
          }
          return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        if (hasMouse()) {
          switch (m_behaviorType) {

            case kButtonWidget:
              generateButtonSelectSignal();
              break;

            case kCheckWidget:
              {
                // Fire onClick() event
                onClick();

                invalidate();
              }
              break;

            case kRadioWidget:
              {
                setSelected(false);
                setSelected(true);

                // Fire onClick() event
                onClick();
              }
              break;
          }
        }
        return true;
      }
      break;

    case kMouseMoveMessage:
      if (isEnabled() && hasCapture()) {
        m_handleSelect = false;
        onSelectWhenDragging();
        m_handleSelect = true;
      }
      break;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      // TODO theme stuff
      if (isEnabled())
        invalidate();
      break;
  }

  return Widget::onProcessMessage(msg);
}

void ButtonBase::generateButtonSelectSignal()
{
  // Deselect
  setSelected(false);

  // Fire onClick() event
  onClick();
}

void ButtonBase::onStartDrag()
{
  // Do nothing
}

void ButtonBase::onSelectWhenDragging()
{
  const bool hasMouse = this->hasMouse();

  // Switch state when the mouse go out
  if ((hasMouse && isSelected() != m_pressedStatus) ||
      (!hasMouse && isSelected() == m_pressedStatus)) {
    if (hasMouse)
      setSelected(m_pressedStatus);
    else
      setSelected(!m_pressedStatus);
  }
}

// ======================================================================
// Button class
// ======================================================================

Button::Button(const std::string& text)
  : ButtonBase(text, kButtonWidget, kButtonWidget, kButtonWidget)
{
  setAlign(CENTER | MIDDLE);
}

// ======================================================================
// CheckBox class
// ======================================================================

CheckBox::CheckBox(const std::string& text,
                   const WidgetType drawType)
  : ButtonBase(text, kCheckWidget, kCheckWidget, drawType)
{
  setAlign(LEFT | MIDDLE);
}

// ======================================================================
// RadioButton class
// ======================================================================

RadioButton::RadioButton(const std::string& text,
                         const int radioGroup,
                         const WidgetType drawType)
  : ButtonBase(text, kRadioWidget, kRadioWidget, drawType)
{
  setAlign(LEFT | MIDDLE);
  setRadioGroup(radioGroup);
}

void RadioButton::setRadioGroup(int radioGroup)
{
  m_radioGroup = radioGroup;

  // TODO: Update old and new groups
}

int RadioButton::getRadioGroup() const
{
  return m_radioGroup;
}

void RadioButton::deselectRadioGroup()
{
  Widget* widget = window();
  if (!widget)
    return;

  std::queue<Widget*> allChildrens;
  allChildrens.push(widget);

  while (!allChildrens.empty()) {
    widget = allChildrens.front();
    allChildrens.pop();

    if (RadioButton* radioButton = dynamic_cast<RadioButton*>(widget)) {
      if (radioButton->getRadioGroup() == m_radioGroup)
        radioButton->setSelected(false);
    }

    for (auto child : widget->children()) {
      allChildrens.push(child);
    }
  }
}

void RadioButton::onSelect(bool selected)
{
  ButtonBase::onSelect(selected);
  if (!selected)
    return;

  if (!m_handleSelect)
    return;

  if (behaviorType() == kRadioWidget) {
    deselectRadioGroup();

    m_handleSelect = false;
    setSelected(true);
    m_handleSelect = true;
  }
}

} // namespace ui
