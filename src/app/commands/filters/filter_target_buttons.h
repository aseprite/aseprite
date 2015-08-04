// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_FILTERS_FILTER_TARGET_BUTTONS_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_TARGET_BUTTONS_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_part.h"
#include "base/signal.h"
#include "filters/target.h"
#include "ui/box.h"

namespace ui {
  class ButtonBase;
}

namespace app {
  using namespace filters;

  class FilterTargetButtons : public ui::Box {
  public:
    // Creates a new button to handle "targets" to apply some filter in
    // the a sprite.
    FilterTargetButtons(int imgtype, bool withChannels);

    Target getTarget() const { return m_target; }
    void setTarget(Target target);

    Signal0<void> TargetChange;

  protected:
    void onChannelChange(ui::ButtonBase* button);
    void onImagesChange(ui::ButtonBase* button);

  private:
    void selectTargetButton(const char* name, Target specificTarget);
    skin::SkinPartPtr getTargetNormalIcon() const;
    skin::SkinPartPtr getTargetSelectedIcon() const;

    Target m_target;
  };

} // namespace app

#endif
