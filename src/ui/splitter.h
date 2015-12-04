// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SPLITTER_H_INCLUDED
#define UI_SPLITTER_H_INCLUDED
#pragma once

#include "ui/widget.h"

namespace ui {

  class Splitter : public Widget
  {
  public:
    enum Type { ByPercentage, ByPixel };

    Splitter(Type type, int align);

    double getPosition() const { return m_pos; }
    void setPosition(double pos);

  protected:
    // Events
    bool onProcessMessage(Message* msg) override;
    void onResize(ResizeEvent& ev) override;
    void onPaint(PaintEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onLoadLayout(LoadLayoutEvent& ev) override;
    void onSaveLayout(SaveLayoutEvent& ev) override;

  private:
    Widget* panel1() const;
    Widget* panel2() const;
    void limitPos();

    Type m_type;
    double m_pos;
  };

} // namespace ui

#endif
