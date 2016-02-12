// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_PREVIEW_EDITOR_H_INCLUDED
#define APP_UI_PREVIEW_EDITOR_H_INCLUDED
#pragma once

#include "app/ui/document_view.h"
#include "app/ui/editor/editor_observer.h"
#include "doc/frame.h"
#include "app/pref/preferences.h"
#include "ui/window.h"

namespace app {
  class MiniCenterButton;
  class MiniPlayButton;

  class PreviewEditorWindow : public ui::Window
                            , public EditorObserver
                            , public DocumentViewPreviewDelegate {
  public:
    PreviewEditorWindow();
    ~PreviewEditorWindow();

    bool isPreviewEnabled() const { return m_isEnabled; }
    void setPreviewEnabled(bool state);

    void updateUsingEditor(Editor* editor);

    Editor* relatedEditor() const { return m_relatedEditor; }

    // EditorObserver impl
    void onStateChanged(Editor* editor) override;
    void onScrollChanged(Editor* editor) override;
    void onZoomChanged(Editor* editor) override;

    // DocumentViewPreviewDelegate impl
    void onScrollOtherEditor(Editor* editor) override;
    void onDisposeOtherEditor(Editor* editor) override;
    void onPreviewOtherEditor(Editor* editor) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onClose(ui::CloseEvent& ev) override;
    void onWindowResize() override;

  private:
    void uncheckCenterButton();
    bool hasDocument() const;
    DocumentPreferences& docPref();
    void onCenterClicked();
    void onPlayClicked();
    void onPopupSpeed();
    void hideWindow();
    void destroyDocView();
    void saveScrollPref();

    bool m_isEnabled;
    DocumentView* m_docView;
    MiniCenterButton* m_centerButton;
    MiniPlayButton* m_playButton;
    doc::frame_t m_refFrame;
    double m_aniSpeed;
    Editor* m_relatedEditor;
  };

} // namespace app

#endif
