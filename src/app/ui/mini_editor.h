/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_UI_MINI_EDITOR_VIEW_H_INCLUDED
#define APP_UI_MINI_EDITOR_VIEW_H_INCLUDED

#include "app/ui/document_view.h"
#include "ui/timer.h"
#include "ui/window.h"

namespace app {
  class MiniPlayButton;

  class MiniEditorWindow : public ui::Window {
  public:
    MiniEditorWindow();
    ~MiniEditorWindow();

    bool isMiniEditorEnabled() const { return m_isEnabled; }
    void setMiniEditorEnabled(bool state);

    void updateUsingEditor(Editor* editor);

  protected:
    void onClose(ui::CloseEvent& ev) OVERRIDE;

  private:
    void onPlayClicked();
    void onPlaybackTick();
    void hideWindow();
    void resetTimer();

    bool m_isEnabled;
    DocumentView* m_docView;
    MiniPlayButton* m_playButton;
    ui::Timer m_playTimer;

    // Number of milliseconds to go to the next frame if m_playTimer
    // is activated.
    int m_nextFrameTime;
  };

} // namespace app

#endif
