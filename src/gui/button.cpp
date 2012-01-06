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

ButtonBase::ButtonBase(const char* text, int type, int behaviorType, int drawType)
  : Widget(type)
  , m_behaviorType(behaviorType)
  , m_drawType(drawType)
  , m_iconInterface(NULL)
{
  this->setAlign(JI_CENTER | JI_MIDDLE);
  this->setText(text);
  jwidget_focusrest(this, true);

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
      if (this->isEnabled()) {
        if (m_behaviorType == JI_BUTTON) {
          /* deselect the widget (maybe the user press the key, but
             before release it, changes the focus) */
          if (this->isSelected())
            this->setSelected(false);
        }

        /* TODO theme specific stuff */
        invalidate();
      }
      break;

    case JM_KEYPRESSED:
      /* if the button is enabled */
      if (this->isEnabled()) {
        /* for JI_BUTTON */
        if (m_behaviorType == JI_BUTTON) {
          /* has focus and press enter/space */
          if (this->hasFocus()) {
            if ((msg->key.scancode == KEY_ENTER) ||
                (msg->key.scancode == KEY_ENTER_PAD) ||
                (msg->key.scancode == KEY_SPACE)) {
              this->setSelected(true);
              return true;
            }
          }
          /* the underscored letter with Alt */
          if ((msg->any.shifts & KB_ALT_FLAG) &&
              (jwidget_check_underscored(this, msg->key.scancode))) {
            this->setSelected(true);
            return true;
          }
          /* magnetic */
          else if (jwidget_is_magnetic(this) &&
                   ((msg->key.scancode == KEY_ENTER) ||
                    (msg->key.scancode == KEY_ENTER_PAD))) {
            jmanager_set_focus(this);

            /* dispatch focus movement messages (because the buttons
               process them) */
            jmanager_dispatch_messages(ji_get_default_manager());

            this->setSelected(true);
            return true;
          }
        }
        /* for JI_CHECK or JI_RADIO */
        else {
          /* if the widget has the focus and the user press space or
             if the user press Alt+the underscored letter of the button */
          if ((this->hasFocus() &&
               (msg->key.scancode == KEY_SPACE)) ||
              ((msg->any.shifts & KB_ALT_FLAG) &&
               (jwidget_check_underscored(this, msg->key.scancode)))) {
            if (m_behaviorType == JI_CHECK) {
              // Swap the select status
              this->setSelected(!this->isSelected());

              // Signal
              jwidget_emit_signal(this, JI_SIGNAL_CHECK_CHANGE);
              invalidate();
            }
            else if (m_behaviorType == JI_RADIO) {
              if (!this->isSelected()) {
                this->setSelected(true);
                jwidget_emit_signal(this, JI_SIGNAL_RADIO_CHANGE);
              }
            }
            return true;
          }
        }
      }
      break;

    case JM_KEYRELEASED:
      if (this->isEnabled()) {
        if (m_behaviorType == JI_BUTTON) {
          if (this->isSelected()) {
            generateButtonSelectSignal();
            return true;
          }
        }
      }
      break;

    case JM_BUTTONPRESSED:
      switch (m_behaviorType) {

        case JI_BUTTON:
          if (this->isEnabled()) {
            this->setSelected(true);

            m_pressedStatus = this->isSelected();
            this->captureMouse();
          }
          return true;

        case JI_CHECK:
          if (this->isEnabled()) {
            this->setSelected(!this->isSelected());

            m_pressedStatus = this->isSelected();
            this->captureMouse();
          }
          return true;

        case JI_RADIO:
          if (this->isEnabled()) {
            if (!this->isSelected()) {
              jwidget_signal_off(this);
              this->setSelected(true);
              jwidget_signal_on(this);

              m_pressedStatus = this->isSelected();
              this->captureMouse();
            }
          }
          return true;
      }
      break;

    case JM_BUTTONRELEASED:
      if (this->hasCapture()) {
        this->releaseMouse();

        if (this->hasMouseOver()) {
          switch (m_behaviorType) {

            case JI_BUTTON:
              generateButtonSelectSignal();
              break;

            case JI_CHECK:
              {
                jwidget_emit_signal(this, JI_SIGNAL_CHECK_CHANGE);

                // Fire onClick() event
                Event ev(this);
                onClick(ev);

                invalidate();
              }
              break;

            case JI_RADIO:
              {
                this->setSelected(false);
                this->setSelected(true);

                jwidget_emit_signal(this, JI_SIGNAL_RADIO_CHANGE);

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
      if (this->isEnabled() && this->hasCapture()) {
        bool hasMouse = this->hasMouseOver();

        // Switch state when the mouse go out
        if (( hasMouse && this->isSelected() != m_pressedStatus) ||
            (!hasMouse && this->isSelected() == m_pressedStatus)) {
          jwidget_signal_off(this);

          if (hasMouse) {
            this->setSelected(m_pressedStatus);
          }
          else {
            this->setSelected(!m_pressedStatus);
          }

          jwidget_signal_on(this);
        }
      }
      break;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      // TODO theme stuff
      if (this->isEnabled())
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
  this->setSelected(false);

  // Emit JI_SIGNAL_BUTTON_SELECT signal
  jwidget_emit_signal(this, JI_SIGNAL_BUTTON_SELECT);

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

bool RadioButton::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SIGNAL:
      if (getBehaviorType() == JI_RADIO) {
        if (msg->signal.num == JI_SIGNAL_SELECT) {
          deselectRadioGroup();

          jwidget_signal_off(this);
          this->setSelected(true);
          jwidget_signal_on(this);
        }
      }
      break;
  }

  return ButtonBase::onProcessMessage(msg);
}
