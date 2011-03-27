// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_SCROLL_BAR_H_INCLUDED
#define GUI_SCROLL_BAR_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

class ScrollBar : public Widget
{
public:
  ScrollBar(int align);

  int getPos() const { return m_pos; }
  void setPos(int pos);

  int getSize() const { return m_size; }
  void setSize(int size);

  // For themes
  void getScrollBarThemeInfo(int* pos, int* len);

protected:
  // Events
  bool onProcessMessage(JMessage msg) OVERRIDE;

private:
  void getScrollBarInfo(int* _pos, int* _len, int* _bar_size, int* _viewport_size);

  int m_pos;
  int m_size;

  static int m_wherepos;
  static int m_whereclick;
};

#endif
