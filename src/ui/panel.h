// Aseprite UI Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_PANEL_H_INCLUDED
#define UI_PANEL_H_INCLUDED
#pragma once

#include "ui/box.h"

namespace ui {

  class Panel : public VBox {
  public:
    Panel();

    void showChild(Widget* widget);
    void showAllChildren();

  protected:
    virtual void onResize(ResizeEvent& ev) override;
    virtual void onSizeHint(SizeHintEvent& ev) override;

  private:
    bool m_multiple;
  };

} // namespace ui

#endif
