// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
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
                       WidgetType type,
                       WidgetType behaviorType,
                       WidgetType drawType)
  : Widget(type)
  , m_pressedStatus(false)
  , m_behaviorType(behaviorType)
  , m_drawType(drawType)
  , m_iconInterface(NULL)
  , m_handleSelect(true)
{
  setAlign(CENTER | MIDDLE);
  setText(text);
  setFocusStop(true);

  // Initialize theme
  setType(m_drawType);      // TODO Fix this nasty trick
  initTheme();
  setType(type);
}

ButtonBase::~ButtonBase()
{
  if (m_iconInterface)
    m_iconInterface->destroy();
}

WidgetType ButtonBase::behaviorType() const
{
  return m_behaviorType;
}

WidgetType ButtonBase::drawType() const
{
  return m_drawType;
}

void ButtonBase::setIconInterface(IButtonIcon* iconInterface)
{
  if (m_iconInterface)
    m_iconInterface->destroy();

  m_iconInterface = iconInterface;

  invalidate();
}

void ButtonBase::onClick(Event& ev)
{
  // Fire Click() signal
  Click(ev);
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

      // If the button is enabled.
      if (isEnabled()) {
        bool mnemonicPressed =
          ((msg->altPressed() || msg->cmdPressed()) &&
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

          // Check if the user pressed mnemonic.
          if (mnemonicPressed) {
            setSelected(true);
            return true;
          }
          // Magnetic widget catches ENTERs
          else if (isFocusMagnet() &&
                   ((scancode == kKeyEnter) ||
                    (scancode == kKeyEnterPad))) {
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
          /* if the widget has the focus and the user press space or
             if the user press Alt+the underscored letter of the button */
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
      if (isEnabled()) {
        if (m_behaviorType == kButtonWidget) {
          if (isSelected()) {
            generateButtonSelectSignal();
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
          }
          return true;

        case kCheckWidget:
          if (isEnabled()) {
            setSelected(!isSelected());

            m_pressedStatus = isSelected();
            captureMouse();
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
            }
          }
          return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        if (hasMouseOver()) {
          switch (m_behaviorType) {

            case kButtonWidget:
              generateButtonSelectSignal();
              break;

            case kCheckWidget:
              {
                // Fire onClick() event
                Event ev(this);
                onClick(ev);

                invalidate();
              }
              break;

            case kRadioWidget:
              {
                setSelected(false);
                setSelected(true);

                // Fire onClick() event
                Event ev(this);
                onClick(ev);
              }
              break;
          }
        }
        return true;
      }
      break;

    case kMouseMoveMessage:
      if (isEnabled() && hasCapture()) {
        bool hasMouse = hasMouseOver();

        m_handleSelect = false;

        // Switch state when the mouse go out
        if ((hasMouse && isSelected() != m_pressedStatus) ||
            (!hasMouse && isSelected() == m_pressedStatus)) {
          if (hasMouse)
            setSelected(m_pressedStatus);
          else
            setSelected(!m_pressedStatus);
        }

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

void ButtonBase::onSizeHint(SizeHintEvent& ev)
{
  // If there is a style specified in this widget, use the new generic
  // widget to calculate the size hint.
  if (style())
    return Widget::onSizeHint(ev);

  gfx::Rect box;
  gfx::Size iconSize = (m_iconInterface ? m_iconInterface->size(): gfx::Size(0, 0));
  getTextIconInfo(&box, NULL, NULL,
                  m_iconInterface ? m_iconInterface->iconAlign(): 0,
                  iconSize.w,
                  iconSize.h);

  ev.setSizeHint(box.w + border().width(),
                 box.h + border().height());
}

void ButtonBase::onPaint(PaintEvent& ev)
{
  // If there is a style specified in this widget, use the new generic
  // widget painting code.
  if (style())
    return Widget::onPaint(ev);

  switch (m_drawType) {
    case kButtonWidget: ASSERT(false); break;
    case kCheckWidget:  theme()->paintCheckBox(ev); break;
    case kRadioWidget:  theme()->paintRadioButton(ev); break;
  }
}

void ButtonBase::generateButtonSelectSignal()
{
  // Deselect
  setSelected(false);

  // Fire onClick() event
  Event ev(this);
  onClick(ev);
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

CheckBox::CheckBox(const std::string& text, WidgetType drawType)
  : ButtonBase(text, kCheckWidget, kCheckWidget, drawType)
{
  setAlign(LEFT | MIDDLE);
}

// ======================================================================
// RadioButton class
// ======================================================================

RadioButton::RadioButton(const std::string& text, int radioGroup, WidgetType drawType)
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
