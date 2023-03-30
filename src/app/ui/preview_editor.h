// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PREVIEW_EDITOR_H_INCLUDED
#define APP_UI_PREVIEW_EDITOR_H_INCLUDED
#pragma once

#include "app/ui/doc_view.h"
#include "app/ui/editor/editor_observer.h"
#include "doc/frame.h"
#include "app/pref/preferences.h"
#include "ui/window.h"

namespace app {
  class MiniCenterButton;
  class MiniPlayButton;

  class PreviewEditorWindow : public ui::Window
                            , public EditorObserver
                            , public DocViewPreviewDelegate {
  public:
    PreviewEditorWindow();
    ~PreviewEditorWindow();

    bool isPreviewEnabled() const { return m_isEnabled; }
    void setPreviewEnabled(bool state);
    void pressPlayButton();

    void updateUsingEditor(Editor* editor);
    Editor* previewEditor() const;
    Editor* relatedEditor() const { return m_relatedEditor; }

    // EditorObserver impl
    void onStateChanged(Editor* editor) override;
    void onScrollChanged(Editor* editor) override;
    void onZoomChanged(Editor* editor) override;

    // DocViewPreviewDelegate impl
    void onScrollOtherEditor(Editor* editor) override;
    void onDisposeOtherEditor(Editor* editor) override;
    void onPreviewOtherEditor(Editor* editor) override;
    void onTagChangeEditor(Editor* editor, DocEvent& ev) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onInitTheme(ui::InitThemeEvent& ev) override;
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
    void adjustPlayingTag();

    bool m_isEnabled;
    DocView* m_docView;
    MiniCenterButton* m_centerButton;
    MiniPlayButton* m_playButton;
    doc::frame_t m_refFrame;
    double m_aniSpeed;
    Editor* m_relatedEditor;
    // This flag indicates that the preview editor is being opened, it is used to avoid
    // an infinite recursive loop when PreviewEditorWindow::updateUsingEditor calls the
    // openWindow() method.
    bool m_opening;
  };

} // namespace app

#endif
