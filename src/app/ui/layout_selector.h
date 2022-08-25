// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_LAYOUT_SELECTOR_H_INCLUDED
#define APP_UI_LAYOUT_SELECTOR_H_INCLUDED
#pragma once

#include "app/ui/dockable.h"
#include "app/ui/icon_button.h"
#include "app/ui/layout.h"
#include "app/ui/layouts.h"
#include "ui/animated_widget.h"
#include "ui/box.h"
#include "ui/combobox.h"

#include <memory>
#include <vector>

namespace ui {
class TooltipManager;
}

namespace app {

class LayoutSelector : public ui::HBox,
                       public ui::AnimatedWidget,
                       public Dockable {
  enum Ani : int {
    ANI_NONE,
    ANI_EXPANDING,
    ANI_COLLAPSING,
  };

  class LayoutItem;

  class LayoutComboBox : public ui::ComboBox {
  private:
    void onChange() override;
    void onCloseListBox() override;
    LayoutItem* m_selected = nullptr;
  };

public:
  LayoutSelector(ui::TooltipManager* tooltipManager);
  ~LayoutSelector();

  void addLayout(const LayoutPtr& layout);

  // Dockable impl
  int dockableAt() const override { return ui::TOP | ui::BOTTOM; }

private:
  void setupTooltips(ui::TooltipManager* tooltipManager);
  void onAnimationFrame() override;
  void onAnimationStop(int animation) override;
  void switchSelector();

  LayoutComboBox m_comboBox;
  IconButton m_button;
  gfx::Size m_startSize;
  gfx::Size m_endSize;
  Layouts m_layouts;
};

} // namespace app

#endif
