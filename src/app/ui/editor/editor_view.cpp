// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor_view.h"

#include "app/app.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "she/surface.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"

namespace app {

using namespace app::skin;
using namespace ui;

// static
EditorView::Method EditorView::g_scrollUpdateMethod = Method::KeepOrigin;

// static
void EditorView::SetScrollUpdateMethod(Method method)
{
  g_scrollUpdateMethod = method;
}

EditorView::EditorView(EditorView::Type type)
  : View()
  , m_type(type)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  int l = theme->parts.editorSelected()->getBitmapW()->width();
  int t = theme->parts.editorSelected()->getBitmapN()->height();
  int r = theme->parts.editorSelected()->getBitmapE()->width();
  int b = theme->parts.editorSelected()->getBitmapS()->height();

  setBorder(gfx::Border(l, t, r, b));
  setBgColor(gfx::rgba(0, 0, 0));
  setupScrollbars();

  m_scrollSettingsConn =
    Preferences::instance().editor.showScrollbars.AfterChange.connect(
      Bind(&EditorView::setupScrollbars, this));
}

void EditorView::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  bool selected = false;

  switch (m_type) {

    // Only show the view selected if it is the current editor
    case CurrentEditorMode:
      selected = (editor()->isActive());
      break;

      // Always show selected
    case AlwaysSelected:
      selected = true;
      break;

  }

  theme->drawRect(
    g, getClientBounds(),
    (selected ?
     theme->parts.editorSelected().get():
     theme->parts.editorNormal().get()),
    getBgColor());
}

void EditorView::onResize(ResizeEvent& ev)
{
  Editor* editor = this->editor();
  gfx::Point oldPos;
  if (editor) {
    switch (g_scrollUpdateMethod) {
      case KeepOrigin:
        oldPos = editor->editorToScreen(gfx::Point(0, 0));
        break;
      case KeepCenter:
        oldPos = editor->screenToEditor(getViewportBounds().getCenter());
        break;
    }
  }

  View::onResize(ev);

  if (editor) {
    switch (g_scrollUpdateMethod) {
      case KeepOrigin: {
        // This keeps the same scroll position for the editor
        gfx::Point newPos = editor->editorToScreen(gfx::Point(0, 0));
        gfx::Point oldScroll = getViewScroll();
        editor->setEditorScroll(oldScroll + newPos - oldPos, false);
        break;
      }
      case KeepCenter:
        editor->centerInSpritePoint(oldPos);
        break;
    }
  }
}

void EditorView::onScrollChange()
{
  View::onScrollChange();

  Editor* editor = this->editor();
  ASSERT(editor != NULL);
  if (editor)
    editor->notifyScrollChanged();
}

void EditorView::setupScrollbars()
{
  if (m_type == AlwaysSelected ||
      !Preferences::instance().editor.showScrollbars()) {
    hideScrollBars();
  }
  else {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
    int barsize = theme->dimensions.miniScrollbarSize();

    getHorizontalBar()->setBarWidth(barsize);
    getVerticalBar()->setBarWidth(barsize);

    setup_mini_look(getHorizontalBar());
    setup_mini_look(getVerticalBar());

    showScrollBars();
  }
}

Editor* EditorView::editor()
{
  return static_cast<Editor*>(attachedWidget());
}

} // namespace app
