// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_SCOPED_CURSOR_H_INCLUDED
#define APP_UI_EDITOR_SCOPED_CURSOR_H_INCLUDED
#pragma once

namespace app {

  class ShowHideDrawingCursor {
  public:
    ShowHideDrawingCursor(Editor* editor) : m_editor(editor) {
      m_editor->showDrawingCursor();
    }
    ~ShowHideDrawingCursor() {
      m_editor->hideDrawingCursor();
    }
  private:
    Editor* m_editor;
  };

  class HideShowDrawingCursor {
  public:
    HideShowDrawingCursor(Editor* editor) : m_editor(editor) {
      m_editor->hideDrawingCursor();
    }
    ~HideShowDrawingCursor() {
      m_editor->showDrawingCursor();
    }
  private:
    Editor* m_editor;
  };

} // namespace app

#endif
