// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_CONFIGURE_TIMELINE_POPUP_H_INCLUDED
#define APP_UI_CONFIGURE_TIMELINE_POPUP_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "doc/anidir.h"
#include "ui/popup_window.h"

namespace ui {
  class Button;
  class CheckBox;
  class RadioButton;
  class Slider;
}

namespace app {
  namespace gen {
    class TimelineConf;
  }

  class Doc;

  class ConfigureTimelinePopup : public ui::PopupWindow {
  public:
    ConfigureTimelinePopup();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onChangePosition();
    void onChangeFirstFrame();
    void onChangeType();
    void onOpacity();
    void onOpacityStep();
    void onResetOnionskin();
    void onLoopTagChange();
    void onCurrentLayerChange();
    void onPositionChange();

    void onZoomChange();
    void onThumbEnabledChange();
    void onThumbOverlayEnabledChange();
    void onThumbOverlaySizeChange();

  private:
    void updateWidgetsFromCurrentSettings();
    Doc* doc();
    DocumentPreferences& docPref();

    app::gen::TimelineConf* m_box;
    bool m_lockUpdates;
  };

} // namespace app

#endif
