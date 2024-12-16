// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_TEXTBOX_H_INCLUDED
#define UI_TEXTBOX_H_INCLUDED
#pragma once

#include "ui/widget.h"

namespace ui {

class TextBox : public Widget {
public:
  TextBox(const std::string& text, int align);

protected:
  bool onProcessMessage(Message* msg) override;
  void onPaint(PaintEvent& ev) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void onSetText() override;

private:
  gfx::Point m_oldPos;
};

} // namespace ui

#endif
