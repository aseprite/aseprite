// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_VIEW_H_INCLUDED
#define APP_UI_EDITOR_VIEW_H_INCLUDED
#pragma once

#include "ui/view.h"
#include "app/settings/settings_observers.h"

namespace app {
  class Editor;

  // TODO hardcoded scroll bar width should be get from skin.xml file
  const int kEditorViewScrollbarWidth = 6;

  class EditorView : public ui::View,
                     public GlobalSettingsObserver {
  public:
    enum Type { CurrentEditorMode, AlwaysSelected };

    EditorView(Type type);
    ~EditorView();

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onScrollChange() override;

    // GlobalSettingsObserver impl
    void onSetShowSpriteEditorScrollbars(bool state) override;

  private:
    Editor* editor();
    void setupScrollbars();

    Type m_type;
  };

} // namespace app

#endif  // APP_UI_EDITOR_VIEW_H_INCLUDED
