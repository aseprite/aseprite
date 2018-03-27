// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_FILTER_TARGET_BUTTONS_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_TARGET_BUTTONS_H_INCLUDED
#pragma once

#include "app/commands/filters/cels_target.h"
#include "app/ui/button_set.h"
#include "app/ui/skin/skin_part.h"
#include "filters/target.h"
#include "obs/signal.h"
#include "ui/tooltips.h"

namespace ui {
  class ButtonBase;
}

namespace app {
  using namespace filters;

  class FilterTargetButtons : public ButtonSet {
  public:
    // Creates a new button to handle "targets" to apply some filter in
    // the a sprite.
    FilterTargetButtons(int imgtype, bool withChannels);

    Target target() const { return m_target; }
    CelsTarget celsTarget() const { return m_celsTarget; }

    void setTarget(const Target target);
    void setCelsTarget(const CelsTarget celsTarget);

    obs::signal<void()> TargetChange;

  protected:
    void onItemChange(Item* item) override;
    void onChannelChange(ui::ButtonBase* button);
    void onImagesChange(ui::ButtonBase* button);

  private:
    void selectTargetButton(Item* item, Target specificTarget);
    void updateFromTarget();
    void updateFromCelsTarget();
    void updateComponentTooltip(Item* item, const char* channelName, int align);
    std::string getCelsTargetText() const;
    std::string getCelsTargetTooltip() const;

    Target m_target;
    CelsTarget m_celsTarget;
    Item* m_red;
    Item* m_green;
    Item* m_blue;
    Item* m_alpha;
    Item* m_gray;
    Item* m_index;
    Item* m_cels;
    ui::TooltipManager m_tooltips;
  };

} // namespace app

#endif
