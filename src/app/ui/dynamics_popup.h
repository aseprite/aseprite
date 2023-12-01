// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DYNAMICS_POPUP_H_INCLUDED
#define APP_UI_DYNAMICS_POPUP_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/tools/dynamics.h"
#include "app/tools/velocity.h"
#include "app/ui/button_set.h"
#include "base/time.h"
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
      virtual void setMaxSize(int size) = 0;
      virtual void setMaxAngle(int angle) = 0;
      virtual void onDynamicsChange(const tools::DynamicsOptions& dynamicsOptions) = 0;
    };
    DynamicsPopup(Delegate* delegate);

    tools::DynamicsOptions getDynamics() const;
    void setOptionsGridVisibility(bool state);
    void loadDynamicsPref(bool sameInAllTools);
    void saveDynamicsPref(bool sameInAllTools);
    std::string ditheringMatrixName() const;
    void refreshVisibility();
    bool sharedSettings() const;

  private:
    class ThresholdSlider;

    void setCheck(int i, bool state);
    bool isCheck(int i) const;
    void onValuesChange(ButtonSet::Item* item);
    void updateFromToText();
    void updateWidgetsWithBrush();
    bool onProcessMessage(ui::Message* msg) override;

    Delegate* m_delegate;
    gen::Dynamics* m_dynamics;
    // Used to memorize the 'stabilizer factor' slider value.
    // This helps to save the 'stabilizer factor' even if
    // 'stabilizer' check isn't selected.
    int m_stabilizerFactorBackup;
    DitheringSelector* m_ditheringSel;
    gfx::Region m_hotRegion;
    ThresholdSlider* m_pressureThreshold;
    ThresholdSlider* m_velocityThreshold;
    tools::VelocitySensor m_velocity;
    tools::ColorFromTo m_fromTo;
  };

} // namespace app

#endif
