// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/int_entry.h"

#include "gfx/rect.h"
#include "gfx/region.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/popup_window.h"
#include "ui/slider.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <cmath>

namespace ui {

using namespace gfx;

IntEntry::IntEntry(int min, int max)
  : Entry(std::ceil(std::log10((double)max))+1, "")
  , m_min(min)
  , m_max(max)
  , m_popupWindow(NULL)
  , m_slider(NULL)
{
}

IntEntry::~IntEntry()
{
  closePopup();
}

int IntEntry::getValue() const
{
  int value = getTextInt();
  return MID(m_min, value, m_max);
}

void IntEntry::setValue(int value)
{
  value = MID(m_min, value, m_max);

  setTextf("%d", value);

  if (m_slider != NULL)
    m_slider->setValue(value);

  onValueChange();
}

bool IntEntry::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    // When the mouse enter in this entry, it got the focus and the
    // text is automatically selected.
    case kMouseEnterMessage:
      requestFocus();
      break;

    // Reset value if it's out of bounds when focus is lost
    case kFocusLeaveMessage:
      setValue(MID(m_min, getValue(), m_max));
      break;

    case kMouseDownMessage:
      openPopup();
      break;

    case kMouseMoveMessage:
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        Widget* pick = getManager()->pick(mouseMsg->position());
        if (pick == m_slider) {
          releaseMouse();

          MouseMessage mouseMsg2(kMouseDownMessage,
            mouseMsg->buttons(),
            mouseMsg->position());
          m_slider->sendMessage(&mouseMsg2);
        }
      }
      break;

    case kMouseWheelMessage:
      if (isEnabled()) {
        int oldValue = getValue();
        int newValue = oldValue + jmouse_z(0) - jmouse_z(1);
        newValue = MID(m_min, newValue, m_max);
        if (newValue != oldValue) {
          setValue(newValue);
          selectAllText();
        }
        return true;
      }
      break;
  }
  return Entry::onProcessMessage(msg);
}

void IntEntry::onEntryChange()
{
  Entry::onEntryChange();
  onValueChange();
}

void IntEntry::onValueChange()
{
  // Do nothing
}

void IntEntry::openPopup()
{
  Rect rc = getBounds();
  rc.y += rc.h;
  rc.h += 2*jguiscale();
  rc.w = 128*jguiscale();
  if (rc.x+rc.w > JI_SCREEN_W)
    rc.x = rc.x - rc.w + getBounds().w;

  m_popupWindow = new PopupWindow("", false);
  m_popupWindow->setAutoRemap(false);
  m_popupWindow->setBounds(rc);
  m_popupWindow->setBgColor(rgba(0, 0, 0, 0));

  Region rgn(rc.createUnion(getBounds()));
  rgn.createUnion(rgn, Region(getBounds()));
  m_popupWindow->setHotRegion(rgn);

  m_slider = new Slider(m_min, m_max, getValue());
  m_slider->setFocusStop(false); // In this way the IntEntry doesn't lost the focus
  m_slider->Change.connect(&IntEntry::onChangeSlider, this);
  m_popupWindow->addChild(m_slider);

  m_popupWindow->openWindow();
}

void IntEntry::closePopup()
{
  if (m_popupWindow) {
    m_popupWindow->closeWindow(NULL);
    delete m_popupWindow;
    m_popupWindow = NULL;
    m_slider = NULL;
  }
}

void IntEntry::onChangeSlider()
{
  setValue(m_slider->getValue());
  selectAllText();
}

} // namespace ui
