// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_BUTTON_H_INCLUDED
#define GUI_BUTTON_H_INCLUDED

#include "base/signal.h"
#include "gui/widget.h"
#include <vector>

struct BITMAP;

class Event;

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
  Signal1<void, Event&> Click;

protected:
  // Events
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);
  void onPaint(PaintEvent& ev);

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
