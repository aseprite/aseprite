// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_LAYOUT_SELECTOR_H_INCLUDED
#define APP_UI_LAYOUT_SELECTOR_H_INCLUDED
#pragma once

#include "app/ui/dockable.h"
#include "ui/animated_widget.h"
#include "ui/box.h"
#include "ui/combobox.h"
#include "ui/link_label.h"

#include <memory>

namespace app {

class LayoutSelector : public ui::HBox,
                       public ui::AnimatedWidget,
                       public Dockable {
  enum Ani : int {
    ANI_NONE,
    ANI_EXPANDING,
    ANI_COLLAPSING,
  };

  class LayoutComboBox : public ui::ComboBox {
    void onChange() override;
  };

public:
  LayoutSelector();
  ~LayoutSelector();

  // Dockable impl
  int dockableAt() const override { return ui::TOP | ui::BOTTOM; }

private:
  void onAnimationFrame() override;
  void onAnimationStop(int animation) override;
  void switchSelector();

  LayoutComboBox m_comboBox;
  ui::LinkLabel m_button;
  gfx::Size m_startSize;
  gfx::Size m_endSize;
};

} // namespace app

#endif
