// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "os/surface.h"
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
  m_scrollSettingsConn =
    Preferences::instance().editor.showScrollbars.AfterChange.connect(
      base::Bind(&EditorView::setupScrollbars, this));

  InitTheme.connect(
    [this]{
      SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
      setBgColor(gfx::rgba(0, 0, 0)); // TODO Move this color to theme.xml
      setStyle(theme->styles.editorView());
      setupScrollbars();
    });
  initTheme();
}

void EditorView::onPaint(PaintEvent& ev)
{
  switch (m_type) {

    // Only show the view selected if it is the current editor
    case CurrentEditorMode:
      if (editor()->isActive())
        enableFlags(SELECTED);
      else
        disableFlags(SELECTED);
      break;

      // Always show selected
    case AlwaysSelected:
      enableFlags(SELECTED);
      break;

  }

  View::onPaint(ev);
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
    // Hide the brush preview to avoid leaving a cursor trail.
    HideBrushPreview hide(editor->brushPreview());
    View::onSetViewScroll(pt);
  }
}

void EditorView::onScrollRegion(ui::ScrollRegionEvent& ev)
{
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

    horizontalBar()->setStyle(theme->styles.miniScrollbar());
    verticalBar()->setStyle(theme->styles.miniScrollbar());
    horizontalBar()->setThumbStyle(theme->styles.miniScrollbarThumb());
    verticalBar()->setThumbStyle(theme->styles.miniScrollbarThumb());

    showScrollBars();
  }
}

Editor* EditorView::editor()
{
  return static_cast<Editor*>(attachedWidget());
}

} // namespace app
