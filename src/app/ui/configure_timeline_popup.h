/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_UI_CONFIGURE_TIMELINE_POPUP_H_INCLUDED
#define APP_UI_CONFIGURE_TIMELINE_POPUP_H_INCLUDED
#pragma once

#include "app/settings/document_settings.h"
#include "base/override.h"
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
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
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
