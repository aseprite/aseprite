/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINETE_JBUTTON_H_INCLUDED
#define JINETE_JBUTTON_H_INCLUDED

#include "jinete/jwidget.h"
#include "Vaca/Signal.h"
#include "Vaca/NonCopyable.h"
#include <vector>

namespace Vaca { class Event; }
using Vaca::Event;

struct BITMAP;

// Generic button
class ButtonBase : public Widget
{
public:
  ButtonBase(const char* text,
	     int type,
	     int behaviorType,
	     int drawType);
  virtual ~ButtonBase();

  void setButtonIcon(BITMAP* icon);
  void setButtonIconAlign(int iconAlign);

  BITMAP* getButtonIcon();
  int getButtonIconAlign();

  int getBehaviorType() const;
  int getDrawType() const;

  // Signals
  Vaca::Signal1<void, Event&> Click; ///< @see onClick

protected:
  // Events
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);

  // New events
  virtual void onClick(Event& ev);

private:
  void generateButtonSelectSignal();

  bool m_pressedStatus;
  int m_behaviorType;
  int m_drawType;
  BITMAP* m_icon;
  int m_iconAlign;
};

// Pushable button to execute commands
class Button : public ButtonBase
{
public:
  Button(const char* text);
};

// Check boxes
class CheckBox : public ButtonBase
{
public:
  CheckBox(const char* text, int drawType = JI_CHECK);
};

//////////////////////////////////////////////////////////////////////
// Radio buttons

// Radio button
class RadioButton : public ButtonBase
{
public:
  RadioButton(const char* text, int radioGroup, int drawType = JI_RADIO);

  int getRadioGroup() const;
  void setRadioGroup(int radioGroup);

  void deselectRadioGroup();

protected:
  // Events
  bool onProcessMessage(JMessage msg);

private:
  int m_radioGroup;
};

#endif
