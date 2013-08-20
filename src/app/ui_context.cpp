/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/ui/color_bar.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/mini_editor.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace.h"
#include "base/mutex.h"
#include "base/path.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "raster/sprite.h"
#include "app/settings/ui_settings_impl.h"
#include "app/ui_context.h"
#include "undo/undo_history.h"

#include <allegro/file.h>
#include <allegro/system.h>

namespace app {

UIContext* UIContext::m_instance = NULL;

UIContext::UIContext()
  : Context(new UISettingsImpl)
{
  ASSERT(m_instance == NULL);
  m_instance = this;
}

UIContext::~UIContext()
{
  ASSERT(m_instance == this);
  m_instance = NULL;
}

DocumentView* UIContext::getActiveView() const
{
  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  WorkspaceView* view = workspace->getActiveView();
  if (DocumentView* docView = dynamic_cast<DocumentView*>(view))
    return docView;
  else
    return NULL;
}

void UIContext::setActiveView(DocumentView* docView)
{
  if (docView != NULL) {
    App::instance()->getMainWindow()->getTabsBar()->selectTab(docView);

    if (App::instance()->getMainWindow()->getWorkspace()->getActiveView() != docView)
      App::instance()->getMainWindow()->getWorkspace()->setActiveView(docView);
  }

  current_editor = (docView ? docView->getEditor(): NULL);

  if (current_editor)
    current_editor->requestFocus();

  App::instance()->getMainWindow()->getMiniEditor()->updateUsingEditor(current_editor);

  // Change the image-type of color bar.
  ColorBar::instance()->setPixelFormat(app_get_current_pixel_format());

  // Restore the palette of the selected document.
  app_refresh_screen();

  // Change the main frame title.
  base::string defaultTitle = PACKAGE " v" VERSION;
  base::string title;
  if (docView) {
    // Prepend the document's filename.
    title += base::get_file_name(docView->getDocument()->getFilename());
    title += " - ";
  }
  title += defaultTitle;
  set_window_title(title.c_str());
}

size_t UIContext::countViewsOf(Document* document) const
{
  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  size_t counter = 0;

  for (Workspace::iterator it=workspace->begin(); it != workspace->end(); ++it) {
    WorkspaceView* view = *it;
    if (DocumentView* docView = dynamic_cast<DocumentView*>(view)) {
      if (docView->getDocument() == document) {
        ++counter;
      }
    }
  }

  return counter;
}

Editor* UIContext::getActiveEditor()
{
  DocumentView* activeView = getActiveView();
  if (activeView)
    return activeView->getEditor();
  else
    return NULL;
}

void UIContext::onAddDocument(Document* document)
{
  // base method
  Context::onAddDocument(document);

  // Add a new view for this document
  DocumentView* view = new DocumentView(document, DocumentView::Normal);

  // Add a tab with the new view for the document
  App::instance()->getMainWindow()->getWorkspace()->addView(view);

  setActiveView(view);
  view->getEditor()->setDefaultScroll();
}

void UIContext::onRemoveDocument(Document* document)
{
  Context::onRemoveDocument(document);

  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  DocumentViews docViews;

  // Collect all views related to the document.
  for (Workspace::iterator it=workspace->begin(); it != workspace->end(); ++it) {
    WorkspaceView* view = *it;
    if (DocumentView* docView = dynamic_cast<DocumentView*>(view)) {
      if (docView->getDocument() == document) {
        docViews.push_back(docView);
      }
    }
  }

  for (DocumentViews::iterator it=docViews.begin(); it != docViews.end(); ++it) {
    DocumentView* docView = *it;
    workspace->removeView(docView);
    delete docView;
  }
}

void UIContext::onGetActiveLocation(DocumentLocation* location) const
{
  DocumentView* activeView = getActiveView();
  if (activeView)
    activeView->getDocumentLocation(location);
}

} // namespace app
