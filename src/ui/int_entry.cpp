// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/int_entry.h"

#include "gfx/rect.h"
#include "gfx/region.h"
#include "ui/message.h"
#include "ui/popup_window.h"
#include "ui/slider.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <cmath>

using namespace gfx;

namespace ui {

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
  switch (msg->type) {

    // When the mouse enter in this entry, it got the focus and the
    // text is automatically selected.
    case JM_MOUSEENTER:
      requestFocus();
      selectText(0, -1);
      break;

    case JM_BUTTONPRESSED:
      openPopup();
      break;

    case JM_FOCUSLEAVE:
      closePopup();
      break;

    case JM_WHEEL:
      if (isEnabled()) {
        int oldValue = getValue();
        int newValue = oldValue + jmouse_z(0) - jmouse_z(1);
        newValue = MID(m_min, newValue, m_max);
        if (newValue != oldValue)
          setValue(newValue);
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
  m_popupWindow = new PopupWindow(NULL, false);
  m_popupWindow->setAutoRemap(false);
  m_popupWindow->setBounds(rc);

  Region rgn(rc);
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
}

} // namespace ui
