// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
#include "ui/scroll_region_event.h"

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
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  int l = theme->parts.editorSelected()->bitmapW()->width();
  int t = theme->parts.editorSelected()->bitmapN()->height();
  int r = theme->parts.editorSelected()->bitmapE()->width();
  int b = theme->parts.editorSelected()->bitmapS()->height();

  setBorder(gfx::Border(l, t, r, b));
  setBgColor(gfx::rgba(0, 0, 0));
  setupScrollbars();

  m_scrollSettingsConn =
    Preferences::instance().editor.showScrollbars.AfterChange.connect(
      base::Bind(&EditorView::setupScrollbars, this));
}

void EditorView::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
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
    g, clientBounds(),
    (selected ?
     theme->parts.editorSelected().get():
     theme->parts.editorNormal().get()),
    bgColor());
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
        oldPos = editor->screenToEditor(viewportBounds().center());
        break;
    }
  }

  View::onResize(ev);

  if (editor) {
    switch (g_scrollUpdateMethod) {
      case KeepOrigin: {
        // This keeps the same scroll position for the editor
        gfx::Point newPos = editor->editorToScreen(gfx::Point(0, 0));
        gfx::Point oldScroll = viewScroll();
        editor->setEditorScroll(oldScroll + newPos - oldPos);
        break;
      }
      case KeepCenter:
        editor->centerInSpritePoint(oldPos);
        break;
    }
  }
}

void EditorView::onSetViewScroll(const gfx::Point& pt)
{
  Editor* editor = this->editor();
  if (editor) {
    // We have to hide the brush preview to scroll (without this,
    // keyboard shortcuts to scroll when the brush preview is visible
    // will leave brush previews all over the screen).
    HideBrushPreview hide(editor->brushPreview());
    View::onSetViewScroll(pt);
  }
}

void EditorView::onScrollRegion(ui::ScrollRegionEvent& ev)
{
  View::onScrollRegion(ev);

  gfx::Region& region = ev.region();
  Editor* editor = this->editor();
  ASSERT(editor);
  if (editor) {
    gfx::Region invalidRegion;
    editor->getInvalidDecoratoredRegion(invalidRegion);
    region.createSubtraction(region, invalidRegion);
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
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    int barsize = theme->dimensions.miniScrollbarSize();

    horizontalBar()->setBarWidth(barsize);
    verticalBar()->setBarWidth(barsize);

    setup_mini_look(horizontalBar());
    setup_mini_look(verticalBar());

    showScrollBars();
  }
}

Editor* EditorView::editor()
{
  return static_cast<Editor*>(attachedWidget());
}

} // namespace app
