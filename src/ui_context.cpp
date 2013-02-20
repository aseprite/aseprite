/* ASEPRITE
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

#include "config.h"

#include "app.h"
#include "base/mutex.h"
#include "base/path.h"
#include "document.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "settings/ui_settings_impl.h"
#include "ui_context.h"
#include "undo/undo_history.h"
#include "widgets/color_bar.h"
#include "widgets/document_view.h"
#include "widgets/editor/editor.h"
#include "widgets/main_window.h"
#include "widgets/mini_editor.h"
#include "widgets/tabs.h"
#include "widgets/workspace.h"

#include <allegro/file.h>
#include <allegro/system.h>

UIContext* UIContext::m_instance = NULL;

using namespace widgets;

UIContext::UIContext()
  : Context(new UISettingsImpl)
{
  ASSERT(m_instance == NULL);
  m_instance = this;
  m_activeView = NULL;
}

UIContext::~UIContext()
{
  // No views at this point.
  ASSERT(m_allViews.empty());
  ASSERT(m_activeView == NULL);

  ASSERT(m_instance == this);
  m_instance = NULL;
}

widgets::DocumentView* UIContext::getActiveView()
{
  return m_activeView;
}

void UIContext::setActiveView(widgets::DocumentView* docView)
{
  m_activeView = docView;
  current_editor = (docView ? docView->getEditor(): NULL);

  App::instance()->getMainWindow()->getTabsBar()->selectTab(docView);
  App::instance()->getMainWindow()->getWorkspace()->setActiveView(docView);

  if (current_editor)
    current_editor->requestFocus();

  setActiveDocument(docView ? docView->getDocument(): NULL);

  App::instance()->getMainWindow()->getMiniEditor()->updateUsingEditor(current_editor);

  // Restore the palette of the selected document.
  app_refresh_screen(docView ? docView->getDocument(): NULL);
}

size_t UIContext::countViewsOf(Document* document) const
{
  size_t counter = 0;
  for (DocumentViews::const_iterator
         it=m_allViews.begin(), end=m_allViews.end(); it != end; ++it) {
    DocumentView* view = *it;
    if (view->getDocument() == document)
      ++counter;
  }
  return counter;
}

Editor* UIContext::getActiveEditor()
{
  if (m_activeView)
    return m_activeView->getEditor();
  else
    return NULL;
}

void UIContext::onAddDocument(Document* document)
{
  // base method
  Context::onAddDocument(document);

  // Add a new view for this document
  DocumentView* view = new DocumentView(document, DocumentView::Normal);
  m_allViews.push_back(view);

  // Add a tab with the new view for the document
  App::instance()->getMainWindow()->getTabsBar()->addTab(view);
  App::instance()->getMainWindow()->getWorkspace()->addView(view);

  setActiveView(view);
  view->getEditor()->setDefaultScroll();

  // Rebuild the list of tabs
  app_rebuild_documents_tabs();
}

void UIContext::onRemoveDocument(Document* document)
{
  Context::onRemoveDocument(document);

  // Remove all views of this document
  for (DocumentViews::iterator it=m_allViews.begin(); it != m_allViews.end(); ) {
    DocumentView* view = *it;

    if (view->getDocument() == document) {
      App::instance()->getMainWindow()->getTabsBar()->removeTab(view);
      App::instance()->getMainWindow()->getWorkspace()->removeView(view);

      // We cannot point as "active view" this view that we're destroying.
      if (view == m_activeView)
        m_activeView = NULL;

      delete view;
      it = m_allViews.erase(it);
    }
    else
      ++it;
  }

  // Rebuild the tabs
  app_rebuild_documents_tabs();
}

void UIContext::onSetActiveDocument(Document* document)
{
  Context::onSetActiveDocument(document);

  if (!document)
    setActiveView(NULL);

  // Select the first view with the given document.
  for (DocumentViews::iterator it=m_allViews.begin(), end=m_allViews.end();
       it != end; ++it) {
    DocumentView* view = *it;

    if (view->getDocument() == document) {
      setActiveView(view);
      break;
    }
  }

  // Change the image-type of color bar.
  ColorBar::instance()->setPixelFormat(app_get_current_pixel_format());

  // Change the main frame title.
  base::string defaultTitle = PACKAGE " v" VERSION;
  base::string title;
  if (document) {
    // Prepend the document's filename.
    title += base::get_file_name(document->getFilename());
    title += " - ";
  }
  title += defaultTitle;
  set_window_title(title.c_str());
}
