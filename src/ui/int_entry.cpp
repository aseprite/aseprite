// Aseprite UI Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/int_entry.h"

#include "base/clamp.h"
#include "base/scoped_value.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "os/font.h"
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

namespace ui {

using namespace gfx;

IntEntry::IntEntry(int min, int max, SliderDelegate* sliderDelegate)
  : Entry(int(std::floor(std::log10(double(max))))+1, "")
  , m_min(min)
  , m_max(max)
  , m_slider(m_min, m_max, m_min, sliderDelegate)
  , m_popupWindow(nullptr)
  , m_changeFromSlider(false)
{
  m_slider.setFocusStop(false); // In this way the IntEntry doesn't lost the focus
  m_slider.setTransparent(true);
  m_slider.Change.connect(&IntEntry::onChangeSlider, this);
  initTheme();
}

IntEntry::~IntEntry()
{
  closePopup();
}

int IntEntry::getValue() const
{
  int value = m_slider.convertTextToValue(text());
  return base::clamp(value, m_min, m_max);
}

void IntEntry::setValue(int value)
{
  value = base::clamp(value, m_min, m_max);

  setText(m_slider.convertValueToText(value));

  if (m_popupWindow && !m_changeFromSlider)
    m_slider.setValue(value);

  onValueChange();
}

bool IntEntry::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    // Reset value if it's out of bounds when focus is lost
    case kFocusLeaveMessage:
      setValue(base::clamp(getValue(), m_min, m_max));
      deselectText();
      break;

    case kMouseDownMessage:
      requestFocus();
      captureMouse();

      openPopup();
      selectAllText();
      return true;

    case kMouseMoveMessage:
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        Widget* pick = manager()->pick(mouseMsg->position());
        if (pick == &m_slider) {
          releaseMouse();

          MouseMessage mouseMsg2(kMouseDownMessage,
                                 mouseMsg->pointerType(),
                                 mouseMsg->button(),
                                 mouseMsg->modifiers(),
                                 mouseMsg->position());
          m_slider.sendMessage(&mouseMsg2);
        }
      }
      break;

    case kMouseWheelMessage:
      if (isEnabled()) {
        int oldValue = getValue();
        int newValue = oldValue
          + static_cast<MouseMessage*>(msg)->wheelDelta().x
          - static_cast<MouseMessage*>(msg)->wheelDelta().y;
        newValue = base::clamp(newValue, m_min, m_max);
        if (newValue != oldValue) {
          setValue(newValue);
          selectAllText();
        }
        return true;
      }
      break;

    case kKeyDownMessage:
      if (hasFocus() && !isReadOnly()) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        int chr = keymsg->unicodeChar();
        if (chr >= 32 && (chr < '0' || chr > '9')) {
          // "Eat" all keys that aren't number
          return true;
        }
        // Else we use the default Entry processing function which
        // will process keys like Left/Right arrows, clipboard
        // handling, etc.
      }
      break;
  }
  return Entry::onProcessMessage(msg);
}

void IntEntry::onInitTheme(InitThemeEvent& ev)
{
  Entry::onInitTheme(ev);
  m_slider.initTheme();       // The slider might not be in the popup window
  if (m_popupWindow)
    m_popupWindow->initTheme();
}

void IntEntry::onSizeHint(SizeHintEvent& ev)
{
  int trailing = font()->textLength(getSuffix());
  trailing = std::max(trailing, 2*theme()->getEntryCaretSize(this).w);

  int min_w = font()->textLength(m_slider.convertValueToText(m_min));
  int max_w = font()->textLength(m_slider.convertValueToText(m_max)) + trailing;

  int w = std::max(min_w, max_w);
  int h = textHeight();

  w += border().width();
  h += border().height();

  ev.setSizeHint(w, h);
}

void IntEntry::onChange()
{
  Entry::onChange();
  onValueChange();
}

void IntEntry::onValueChange()
{
  // Do nothing
}

void IntEntry::openPopup()
{
  m_slider.setValue(getValue());

  m_popupWindow = new TransparentPopupWindow(PopupWindow::ClickBehavior::CloseOnClickInOtherWindow);
  m_popupWindow->setAutoRemap(false);
  m_popupWindow->addChild(&m_slider);
  m_popupWindow->Close.connect(&IntEntry::onPopupClose, this);

  Rect rc = bounds();
  gfx::Size sz = m_popupWindow->sizeHint();
  rc.w = 128*guiscale();
  if (rc.x+rc.w > ui::display_w())
    rc.x = rc.x-rc.w+bounds().w;
  if (rc.y+rc.h+sz.h < ui::display_h())
    rc.y += rc.h;
  else
    rc.y -= sz.h;
  m_popupWindow->setBounds(rc);

  Region rgn(rc.createUnion(bounds()));
  rgn.createUnion(rgn, Region(bounds()));
  m_popupWindow->setHotRegion(rgn);

  m_popupWindow->openWindow();
}

void IntEntry::closePopup()
{
  if (m_popupWindow) {
    removeSlider();

    m_popupWindow->closeWindow(nullptr);
    delete m_popupWindow;
    m_popupWindow = nullptr;
  }
}

void IntEntry::onChangeSlider()
{
  base::ScopedValue<bool> lockFlag(m_changeFromSlider, true, false);
  setValue(m_slider.getValue());
  selectAllText();
}

void IntEntry::onPopupClose(CloseEvent& ev)
{
  removeSlider();

  deselectText();
  releaseFocus();
}

void IntEntry::removeSlider()
{
  if (m_popupWindow &&
      m_slider.parent() == m_popupWindow) {
    m_popupWindow->removeChild(&m_slider);
  }
}

} // namespace ui
