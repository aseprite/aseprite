// Aseprite UI Library
// Copyright (C) 2021  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SPLITTER_H_INCLUDED
#define UI_SPLITTER_H_INCLUDED
#pragma once

#include "ui/widget.h"

namespace ui {

class Splitter : public Widget {
public:
  enum Type { ByPercentage, ByPixel };

  Splitter(Type type, int align);

  double getPosition() const { return m_userPos; }
  void setPosition(double pos);

protected:
  // Events
  bool onProcessMessage(Message* msg) override;
  void onInitTheme(InitThemeEvent& ev) override;
  void onResize(ResizeEvent& ev) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void onLoadLayout(LoadLayoutEvent& ev) override;
  void onSaveLayout(SaveLayoutEvent& ev) override;

  virtual void onPositionChange();

private:
  Widget* panel1() const;
  Widget* panel2() const;
  void calcPos();

  Type m_type;
  double m_userPos, m_pos;
  int m_guiscale;
};

} // namespace ui

#endif
