// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DYNAMICS_POPUP_H_INCLUDED
#define APP_UI_DYNAMICS_POPUP_H_INCLUDED
#pragma once

#include "app/tools/dynamics.h"
#include "app/ui/button_set.h"
#include "doc/brush.h"
#include "gfx/region.h"
#include "ui/popup_window.h"

namespace app {
  namespace gen {
    class Dynamics;
  }

  class DitheringSelector;

  class DynamicsPopup : public ui::PopupWindow {
  public:
    class Delegate {
    public:
      virtual ~Delegate() { }
      virtual doc::BrushRef getActiveBrush() = 0;
    };
    DynamicsPopup(Delegate* delegate);

    tools::DynamicsOptions getDynamics() const;

  private:
    void setCheck(int i, bool state);
    bool isCheck(int i) const;
    void onValuesChange(ButtonSet::Item* item);
    bool onProcessMessage(ui::Message* msg) override;

    Delegate* m_delegate;
    gen::Dynamics* m_dynamics;
    DitheringSelector* m_ditheringSel;
    gfx::Region m_hotRegion;
  };

} // namespace app

#endif
