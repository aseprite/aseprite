/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef WIDGETS_POPUP_FRAME_PIN_H_INCLUDED
#define WIDGETS_POPUP_FRAME_PIN_H_INCLUDED

#include "gui/button.h"
#include "gui/popup_frame.h"

class PopupFramePin : public PopupFrame
{
public:
  PopupFramePin(const char* text, bool close_on_buttonpressed);

protected:
  virtual bool onProcessMessage(Message* msg) OVERRIDE;
  virtual void onHitTest(HitTestEvent& ev) OVERRIDE;

  // The pin. Your derived class must add this pin in some place of
  // the frame as a children, and you must to remove the pin from the
  // parent in your class's dtor.
  CheckBox* getPin() { return &m_pin; }

private:
  void onPinClick(Event& ev);

  CheckBox m_pin;
};

#endif
