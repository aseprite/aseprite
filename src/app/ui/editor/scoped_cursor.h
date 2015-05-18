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

  class HideShowDrawingCursor {
  public:
    HideShowDrawingCursor(Editor* editor)
      : m_editor(editor)
      , m_cursorOnScreen(m_editor->cursorOnScreen())
    {
      if (m_cursorOnScreen)
        m_editor->hideDrawingCursor();
    }
    ~HideShowDrawingCursor() {
      if (m_cursorOnScreen)
        m_editor->showDrawingCursor();
    }
  private:
    Editor* m_editor;
    bool m_cursorOnScreen;
  };

} // namespace app

#endif
