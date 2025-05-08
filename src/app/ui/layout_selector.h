// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
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

class LayoutSelector : public ui::VBox,
                       public ui::AnimatedWidget,
                       public Dockable {
  enum Ani : int {
    ANI_NONE,
    ANI_EXPANDING,
    ANI_COLLAPSING,
  };

  class LayoutItem;

  class LayoutComboBox : public ui::ComboBox {
  public:
    void setLockChange(bool state) { m_lockChange = state; }

  private:
    void onChange() override;
    void onCloseListBox() override;
    LayoutItem* m_selected = nullptr;
    bool m_lockChange = false;
  };

public:
  LayoutSelector(ui::TooltipManager* tooltipManager, ui::Widget* notifications);
  ~LayoutSelector();

  LayoutPtr activeLayout() const;
  const std::string& activeLayoutId() const { return m_activeLayoutId; }

  void addLayout(const LayoutPtr& layout);
  void removeLayout(const LayoutPtr& layout);
  void removeLayout(const std::string& layoutId);
  void updateActiveLayout(const LayoutPtr& layout);
  void switchSelector();
  void switchSelectorFromCommand();
  bool isSelectorVisible() const;

  // Dockable impl
  int dockableAt() const override { return ui::TOP | ui::BOTTOM; }

protected:
  void onInitTheme(ui::InitThemeEvent& ev) override;

private:
  void setupTooltips(ui::TooltipManager* tooltipManager);
  void setActiveLayoutId(const std::string& layoutId);

  void populateComboBox();
  LayoutItem* getItemByLayoutId(const std::string& id);
  void onAnimationFrame() override;
  void onAnimationStop(int animation) override;

  std::string m_activeLayoutId;
  ui::HBox m_top, m_center, m_bottom;
  LayoutComboBox m_comboBox;
  IconButton m_button;
  Widget* m_notifications = nullptr;
  gfx::Size m_startSize;
  gfx::Size m_endSize;
  Layouts m_layouts;
  bool m_switchComboBoxAfterAni = false;
};

} // namespace app

#endif
