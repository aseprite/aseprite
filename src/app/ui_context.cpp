// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/document.h"
#include "app/document_location.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/settings/ui_settings_impl.h"
#include "app/ui/color_bar.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"
#include "app/ui/tabs.h"
#include "app/ui/timeline.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "base/mutex.h"
#include "doc/sprite.h"

namespace app {

UIContext* UIContext::m_instance = NULL;

UIContext::UIContext()
  : Context(new UISettingsImpl)
  , m_lastSelectedView(NULL)
{
  documents().addObserver(&App::instance()->preferences());

  ASSERT(m_instance == NULL);
  m_instance = this;
}

UIContext::~UIContext()
{
  ASSERT(m_instance == this);
  m_instance = NULL;

  documents().removeObserver(&App::instance()->preferences());

  // The context must be empty at this point. (It's to check if the UI
  // is working correctly, i.e. closing all files when the user can
  // take any action about it.)
  ASSERT(documents().empty());
}

bool UIContext::isUiAvailable() const
{
  return App::instance()->isGui();
}

DocumentView* UIContext::activeView() const
{
  if (!isUiAvailable())
    return NULL;

  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  WorkspaceView* view = workspace->activeView();
  if (DocumentView* docView = dynamic_cast<DocumentView*>(view))
    return docView;
  else
    return NULL;
}

void UIContext::setActiveView(DocumentView* docView)
{
  if (m_lastSelectedView == docView)    // Do nothing case
    return;

  setActiveDocument(docView ? docView->getDocument(): NULL);

  MainWindow* mainWin = App::instance()->getMainWindow();
  if (docView) {
    mainWin->getTabsBar()->selectTab(docView);

    if (mainWin->getWorkspace()->activeView() != docView)
      mainWin->getWorkspace()->setActiveView(docView);
  }

  current_editor = (docView ? docView->getEditor(): NULL);

  if (current_editor)
    current_editor->requestFocus();

  mainWin->getPreviewEditor()->updateUsingEditor(current_editor);
  mainWin->getTimeline()->updateUsingEditor(current_editor);

  // Change the image-type of color bar.
  ColorBar::instance()->setPixelFormat(app_get_current_pixel_format());

  // Restore the palette of the selected document.
  app_refresh_screen();

  // Change the main frame title.
  App::instance()->updateDisplayTitleBar();

  m_lastSelectedView = docView;
}

size_t UIContext::countViewsOf(Document* document) const
{
  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  size_t counter = 0;

  for (auto view : *workspace) {
    if (DocumentView* docView = dynamic_cast<DocumentView*>(view)) {
      if (docView->getDocument() == document) {
        ++counter;
      }
    }
  }

  return counter;
}

DocumentView* UIContext::getFirstDocumentView(Document* document) const
{
  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();

  for (auto view : *workspace) {
    if (DocumentView* docView = dynamic_cast<DocumentView*>(view)) {
      if (docView->getDocument() == document) {
        return docView;
      }
    }
  }

  return NULL;
}

Editor* UIContext::activeEditor()
{
  DocumentView* view = activeView();
  if (view)
    return view->getEditor();
  else
    return NULL;
}

void UIContext::onAddDocument(doc::Document* doc)
{
  Context::onAddDocument(doc);

  // We don't create views in batch mode.
  if (!App::instance()->isGui())
    return;

  // Add a new view for this document
  DocumentView* view = new DocumentView(static_cast<app::Document*>(doc), DocumentView::Normal);

  // Add a tab with the new view for the document
  App::instance()->getMainWindow()->getWorkspace()->addView(view);

  setActiveView(view);
  view->getEditor()->setDefaultScroll();
}

void UIContext::onRemoveDocument(doc::Document* doc)
{
  Context::onRemoveDocument(doc);

  // We don't destroy views in batch mode.
  if (!isUiAvailable())
    return;

  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  DocumentViews docViews;

  // Collect all views related to the document.
  for (Workspace::iterator it=workspace->begin(); it != workspace->end(); ++it) {
    WorkspaceView* view = *it;
    if (DocumentView* docView = dynamic_cast<DocumentView*>(view)) {
      if (docView->getDocument() == doc) {
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
  DocumentView* view = activeView();
  if (view) {
    view->getDocumentLocation(location);
  }
  // Default/dummy location (maybe for batch/command line mode)
  else if (Document* doc = activeDocument()) {
    location->document(doc);
    location->sprite(doc->sprite());
    location->layer(doc->sprite()->indexToLayer(LayerIndex(0)));
    location->frame(0);
  }
}

} // namespace app
