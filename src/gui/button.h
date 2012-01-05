// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_BUTTON_H_INCLUDED
#define GUI_BUTTON_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "gui/widget.h"

#include <vector>

struct BITMAP;

class Event;

class IButtonIcon
{
public:
  virtual ~IButtonIcon() { }
  virtual void destroy() = 0;
  virtual int getWidth() = 0;
  virtual int getHeight() = 0;
  virtual BITMAP* getNormalIcon() = 0;
  virtual BITMAP* getSelectedIcon() = 0;
  virtual BITMAP* getDisabledIcon() = 0;
  virtual int getIconAlign() = 0;
};

// Generic button
class ButtonBase : public Widget
{
public:
  ButtonBase(const char* text,
             int type,
             int behaviorType,
             int drawType);
  virtual ~ButtonBase();

  int getBehaviorType() const;
  int getDrawType() const;

  // Sets the interface used to get icons for the button depending its
  // state. This interface is deleted automatically in the ButtonBase dtor.
  void setIconInterface(IButtonIcon* iconInterface);

  // Used by the current theme to draw the button icon.
  IButtonIcon* getIconInterface() const { return m_iconInterface; }

  // Signals
  Signal1<void, Event&> Click;

protected:
  // Events
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
  void onPaint(PaintEvent& ev) OVERRIDE;

  // New events
  virtual void onClick(Event& ev);

private:
  void generateButtonSelectSignal();

  bool m_pressedStatus;
  int m_behaviorType;
  int m_drawType;
  IButtonIcon* m_iconInterface;
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
  bool onProcessMessage(Message* msg) OVERRIDE;

private:
  int m_radioGroup;
};

#endif
