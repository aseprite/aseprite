// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_SPLITTER_H_INCLUDED
#define UI_SPLITTER_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class Splitter : public Widget
  {
  public:
    enum Type { ByPercentage, ByPixel };

    Splitter(Type type, int align);

    double getPosition() const;
    void setPosition(double pos);

  protected:
    // Events
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onLoadLayout(LoadLayoutEvent& ev) OVERRIDE;
    void onSaveLayout(SaveLayoutEvent& ev) OVERRIDE;

  private:
    void layoutMembers(JRect rect);

    Type m_type;
    double m_pos;
  };

} // namespace ui

#endif
