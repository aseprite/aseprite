// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_SPLITTER_H_INCLUDED
#define UI_SPLITTER_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class Splitter : public Widget
  {
  public:
    Splitter(int align);

    double getPosition() const;
    void setPosition(double pos);

  protected:
    // Events
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;

  private:
    void layoutMembers(JRect rect);

    double m_pos;
  };

} // namespace ui

#endif
