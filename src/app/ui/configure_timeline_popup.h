// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_CONFIGURE_TIMELINE_POPUP_H_INCLUDED
#define APP_UI_CONFIGURE_TIMELINE_POPUP_H_INCLUDED
#pragma once

#include "app/settings/document_settings.h"
#include "ui/popup_window.h"

namespace ui {
  class Button;
  class CheckBox;
  class RadioButton;
  class Slider;
}

namespace app {

  class ConfigureTimelinePopup : public ui::PopupWindow {
  public:
    ConfigureTimelinePopup();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onChangeType();
    void onOpacity();
    void onOpacityStep();
    void onResetOnionskin();
    void onSetLoopSection();
    void onResetLoopSection();
    void onAniDir(IDocumentSettings::AniDir aniDir);

  private:
    void updateWidgetsFromCurrentSettings();
    IDocumentSettings* docSettings();

    ui::RadioButton* m_merge;
    ui::RadioButton* m_tint;
    ui::Slider* m_opacity;
    ui::Slider* m_opacityStep;
    ui::Button* m_resetOnionskin;
    ui::Button* m_setLoopSection;
    ui::Button* m_resetLoopSection;
    ui::RadioButton* m_normalDir;
    ui::RadioButton* m_reverseDir;
    ui::RadioButton* m_pingPongDir;
    bool m_lockUpdates;
  };

} // namespace app

#endif
