// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_APP_TOOLTIPS_H_INCLUDED
#define APP_UI_APP_TOOLTIPS_H_INCLUDED
#pragma once

#include "app/app.h"
#include "app/pref/preferences.h"
#include "ui/tooltips.h"

namespace app {

class AppTooltipManager : public ui::TooltipManager {
public:
  AppTooltipManager()
  {
    m_delayConn = App::instance()->preferences().experimental.tooltipDelay.AfterChange.connect(
      [this] { updateDelay(); });

    updateDelay();
  }

private:
  void updateDelay() { setDelay(App::instance()->preferences().experimental.tooltipDelay()); }

  obs::scoped_connection m_delayConn;
};

} // namespace app

#endif
