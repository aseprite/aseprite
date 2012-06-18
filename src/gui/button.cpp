// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro/gfx.h>
#include <allegro/keyboard.h>
#include <allegro/timer.h>
#include <string.h>
#include <queue>

#include "gui/button.h"
#include "gui/frame.h"
#include "gui/list.h"
#include "gui/manager.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/preferred_size_event.h"
#include "gui/theme.h"
#include "gui/widget.h"

namespace ui {

ButtonBase::ButtonBase(const char* text, int type, int behaviorType, int drawType)
  : Widget(type)
  , m_pressedStatus(false)
  , m_handleSelect(true)
  , m_behaviorType(behaviorType)
  , m_drawType(drawType)
  , m_iconInterface(NULL)
{
  setAlign(JI_CENTER | JI_MIDDLE);
  setText(text);
  setFocusStop(true);

  // Initialize theme
  this->type = m_drawType;      // TODO Fix this nasty trick
  initTheme();
  this->type = type;
}

ButtonBase::~ButtonBase()
{
  if (m_iconInterface)
    m_iconInterface->destroy();
}

int ButtonBase::getBehaviorType() const
{
  return m_behaviorType;
}

int ButtonBase::getDrawType() const
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
  switch (msg->type) {

    case JM_FOCUSENTER:
    case JM_FOCUSLEAVE:
      if (isEnabled()) {
        if (m_behaviorType == JI_BUTTON) {
          // Deselect the widget (maybe the user press the key, but
          // before release it, changes the focus).
          if (isSelected())
            setSelected(false);
        }

        // TODO theme specific stuff
        invalidate();
      }
      break;

    case JM_KEYPRESSED:
      // If the button is enabled.
      if (isEnabled()) {
        // For JI_BUTTON
        if (m_behaviorType == JI_BUTTON) {
          // Has focus and press enter/space
          if (hasFocus()) {
            if ((msg->key.scancode == KEY_ENTER) ||
                (msg->key.scancode == KEY_ENTER_PAD) ||
                (msg->key.scancode == KEY_SPACE)) {
              setSelected(true);
              return true;
            }
          }
          // Check if the user pressed mnemonic.
          if ((msg->any.shifts & KB_ALT_FLAG) &&
              (isScancodeMnemonic(msg->key.scancode))) {
            setSelected(true);
            return true;
          }
          // Magnetic widget catches ENTERs
          else if (isFocusMagnet() &&
                   ((msg->key.scancode == KEY_ENTER) ||
                    (msg->key.scancode == KEY_ENTER_PAD))) {
            getManager()->setFocus(this);

            // Dispatch focus movement messages (because the buttons
            // process them)
            getManager()->dispatchMessages();

            setSelected(true);
            return true;
          }
        }
        // For JI_CHECK or JI_RADIO
        else {
          /* if the widget has the focus and the user press space or
             if the user press Alt+the underscored letter of the button */
          if ((hasFocus() &&
               (msg->key.scancode == KEY_SPACE)) ||
              ((msg->any.shifts & KB_ALT_FLAG) &&
               (isScancodeMnemonic(msg->key.scancode)))) {
            if (m_behaviorType == JI_CHECK) {
              // Swap the select status
              setSelected(!isSelected());

              invalidate();
            }
            else if (m_behaviorType == JI_RADIO) {
              if (!isSelected()) {
                setSelected(true);
              }
            }
            return true;
          }
        }
      }
      break;

    case JM_KEYRELEASED:
      if (isEnabled()) {
        if (m_behaviorType == JI_BUTTON) {
          if (isSelected()) {
            generateButtonSelectSignal();
            return true;
          }
        }
      }
      break;

    case JM_BUTTONPRESSED:
      switch (m_behaviorType) {

        case JI_BUTTON:
          if (isEnabled()) {
            setSelected(true);

            m_pressedStatus = isSelected();
            captureMouse();
          }
          return true;

        case JI_CHECK:
          if (isEnabled()) {
            setSelected(!isSelected());

            m_pressedStatus = isSelected();
            captureMouse();
          }
          return true;

        case JI_RADIO:
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

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
        releaseMouse();

        if (hasMouseOver()) {
          switch (m_behaviorType) {

            case JI_BUTTON:
              generateButtonSelectSignal();
              break;

            case JI_CHECK:
              {
                // Fire onClick() event
                Event ev(this);
                onClick(ev);

                invalidate();
              }
              break;

            case JI_RADIO:
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

    case JM_MOTION:
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

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      // TODO theme stuff
      if (isEnabled())
        invalidate();
      break;
  }

  return Widget::onProcessMessage(msg);
}

void ButtonBase::onPreferredSize(PreferredSizeEvent& ev)
{
  struct jrect box, text, icon;

  jwidget_get_texticon_info(this, &box, &text, &icon,
                            m_iconInterface ? m_iconInterface->getIconAlign(): 0,
                            m_iconInterface ? m_iconInterface->getWidth(): 0,
                            m_iconInterface ? m_iconInterface->getHeight(): 0);

  ev.setPreferredSize(this->border_width.l + jrect_w(&box) + this->border_width.r,
                      this->border_width.t + jrect_h(&box) + this->border_width.b);
}

void ButtonBase::onPaint(PaintEvent& ev)
{
  switch (m_drawType) {
    case JI_BUTTON: getTheme()->paintButton(ev); break;
    case JI_CHECK:  getTheme()->paintCheckBox(ev); break;
    case JI_RADIO:  getTheme()->paintRadioButton(ev); break;
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

Button::Button(const char *text)
  : ButtonBase(text, JI_BUTTON, JI_BUTTON, JI_BUTTON)
{
  setAlign(JI_CENTER | JI_MIDDLE);
}

// ======================================================================
// CheckBox class
// ======================================================================

CheckBox::CheckBox(const char *text, int drawType)
  : ButtonBase(text, JI_CHECK, JI_CHECK, drawType)
{
  setAlign(JI_LEFT | JI_MIDDLE);
}

// ======================================================================
// RadioButton class
// ======================================================================

RadioButton::RadioButton(const char *text, int radioGroup, int drawType)
  : ButtonBase(text, JI_RADIO, JI_RADIO, drawType)
{
  setAlign(JI_LEFT | JI_MIDDLE);
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
  Widget* widget = getRoot();
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

    JLink link;
    JI_LIST_FOR_EACH(widget->children, link) {
      allChildrens.push((Widget*)link->data);
    }
  }
}

void RadioButton::onSelect()
{
  ButtonBase::onSelect();

  if (!m_handleSelect)
    return;

  if (getBehaviorType() == JI_RADIO) {
    deselectRadioGroup();

    m_handleSelect = false;
    setSelected(true);
    m_handleSelect = true;
  }
}

} // namespace ui
