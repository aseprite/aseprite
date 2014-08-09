// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SPLITTER_H_INCLUDED
#define UI_SPLITTER_H_INCLUDED
#pragma once

#include "base/override.h"
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
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onResize(ResizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onLoadLayout(LoadLayoutEvent& ev) OVERRIDE;
    void onSaveLayout(SaveLayoutEvent& ev) OVERRIDE;

  private:
    Type m_type;
    double m_pos;
  };

} // namespace ui

#endif
