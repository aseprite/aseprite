// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_PREVIEW_EDITOR_H_INCLUDED
#define APP_UI_PREVIEW_EDITOR_H_INCLUDED
#pragma once

#include "app/ui/document_view.h"
#include "doc/frame.h"
#include "ui/timer.h"
#include "ui/window.h"

namespace app {
  class MiniCenterButton;
  class MiniPlayButton;

  class PreviewEditorWindow : public ui::Window {
  public:
    PreviewEditorWindow();
    ~PreviewEditorWindow();

    bool isPreviewEnabled() const { return m_isEnabled; }
    void setPreviewEnabled(bool state);

    void updateUsingEditor(Editor* editor);
    void uncheckCenterButton();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onClose(ui::CloseEvent& ev) override;
    void onWindowResize() override;

  private:
    void onCenterClicked();
    void onPlayClicked();
    void onPlaybackTick();
    void hideWindow();

    bool m_isEnabled;
    DocumentView* m_docView;
    MiniCenterButton* m_centerButton;
    MiniPlayButton* m_playButton;
    ui::Timer m_playTimer;

    // Number of milliseconds to go to the next frame if m_playTimer
    // is activated.
    int m_nextFrameTime;
    int m_curFrameTick;

    bool m_pingPongForward;
    doc::frame_t m_refFrame;
  };

} // namespace app

#endif
