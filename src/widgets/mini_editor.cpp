/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "widgets/mini_editor.h"

#include "gfx/rect.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "skin/skin_theme.h"
#include "ui/base.h"
#include "ui/button.h"
#include "ui/close_event.h"
#include "ui/rect.h"
#include "ui/system.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"
#include "widgets/toolbar.h"

using namespace ui;

namespace widgets {

MiniEditorWindow::MiniEditorWindow()
  : Window(false, "Mini-Editor")
  , m_docView(NULL)
{
  child_spacing = 0;
  setAutoRemap(false);
  setWantFocus(false);

  // Default bounds
  int width = JI_SCREEN_W/4;
  int height = JI_SCREEN_H/4;
  setBounds(gfx::Rect(JI_SCREEN_W - width - jrect_w(ToolBar::instance()->rc),
                      JI_SCREEN_H - height - jrect_h(StatusBar::instance()->rc),
                      width, height));

  load_window_pos(this, "MiniEditor");

  m_isEnabled = get_config_bool("MiniEditor", "Enabled", true);
}

MiniEditorWindow::~MiniEditorWindow()
{
  set_config_bool("MiniEditor", "Enabled", m_isEnabled);
  save_window_pos(this, "MiniEditor");
}

void MiniEditorWindow::setMiniEditorEnabled(bool state)
{
  m_isEnabled = state;

  updateUsingEditor(current_editor);
}

void MiniEditorWindow::onClose(ui::CloseEvent& ev)
{
  Button* closeButton = dynamic_cast<Button*>(ev.getSource());
  if (closeButton != NULL &&
      closeButton->getId() == SkinTheme::kThemeCloseButtonId) {
    // Here we don't use "setMiniEditorEnabled" to change the state of
    // "m_isEnabled" because we're coming from a close event of the
    // window.
    m_isEnabled = false;

    // Redraw the tool bar because it shows the mini editor enabled
    // state. TODO abstract this event
    ToolBar::instance()->invalidate();

    delete m_docView;
    m_docView = NULL;
  }
}

void MiniEditorWindow::updateUsingEditor(Editor* editor)
{
  if (!m_isEnabled || !editor) {
    hideWindow();
    return;
  }

  Document* document = editor->getDocument();
  Editor* miniEditor = (m_docView ? m_docView->getEditor(): NULL);

  // Show the mini editor if it wasn't created yet and the user
  // zoomed in, or if the mini-editor was created and the zoom of
  // both editors is not the same.
  if (document &&
      document->getSprite() &&
      ((!isVisible() && editor->getZoom() > 0) ||
       (isVisible() && (!miniEditor || miniEditor->getZoom() != editor->getZoom())))) {
    if (!isVisible())
      openWindow();

    gfx::Rect visibleBounds = editor->getVisibleSpriteBounds();
    gfx::Point pt = visibleBounds.getCenter();

    // Set the same location as in the given editor.
    if (!miniEditor || miniEditor->getDocument() != document) {
      delete m_docView;
      m_docView = new DocumentView(document, DocumentView::Mini); // MiniEditorDocumentView(document, this);
      addChild(m_docView);

      miniEditor = m_docView->getEditor();
      miniEditor->setZoom(0);
      miniEditor->setState(EditorStatePtr(new EditorState));
      layout();
    }

    miniEditor->centerInSpritePoint(pt.x, pt.y);
  }
  else {
    hideWindow();
  }
}

void MiniEditorWindow::hideWindow()
{
  delete m_docView;
  m_docView = NULL;

  if (isVisible())
    closeWindow(NULL);
}

} // namespace widgets
