// ASE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_LABEL_H_INCLUDED
#define GUI_LABEL_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

class Label : public Widget
{
public:
  Label(const char *text);

  int getTextColor() const;
  void setTextColor(int color);

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPaint(PaintEvent& ev) OVERRIDE;

private:
  int m_textColor;
};

#endif
