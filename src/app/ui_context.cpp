// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/color_bar.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/input_chain.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/ui/workspace.h"
#include "app/ui/workspace_tabs.h"
#include "app/ui_context.h"
#include "base/mutex.h"
#include "doc/site.h"
#include "doc/sprite.h"

namespace app {

UIContext* UIContext::m_instance = nullptr;

UIContext::UIContext()
  : m_lastSelectedDoc(nullptr)
  , m_lastSelectedView(nullptr)
{
  documents().add_observer(&Preferences::instance());

  ASSERT(m_instance == NULL);
  m_instance = this;
}

UIContext::~UIContext()
{
  ASSERT(m_instance == this);
  m_instance = NULL;

  documents().remove_observer(&Preferences::instance());

  // The context must be empty at this point. (It's to check if the UI
  // is working correctly, i.e. closing all files when the user can
  // take any action about it.)
  ASSERT(documents().empty());
}

bool UIContext::isUIAvailable() const
{
  return App::instance()->isGui();
}

DocumentView* UIContext::activeView() const
{
  if (!isUIAvailable())
    return nullptr;

  Workspace* workspace = App::instance()->workspace();
  if (!workspace)
    return nullptr;

  WorkspaceView* view = workspace->activeView();
  if (DocumentView* docView = dynamic_cast<DocumentView*>(view))
    return docView;
  else
    return nullptr;
}

void UIContext::setActiveView(DocumentView* docView)
{
  MainWindow* mainWin = App::instance()->mainWindow();

  // Prioritize workspace for user input.
  App::instance()->inputChain().prioritize(mainWin->getWorkspace());

  // Do nothing cases: 1) the view is already selected, or 2) the view
  // is the a preview.
  if (m_lastSelectedView == docView ||
      (docView && docView->isPreview()))
    return;

  if (docView) {
    mainWin->getTabsBar()->selectTab(docView);

    if (mainWin->getWorkspace()->activeView() != docView)
      mainWin->getWorkspace()->setActiveView(docView);
  }

  current_editor = (docView ? docView->editor(): nullptr);

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

  // TODO all the calls to functions like updateUsingEditor(),
  // setPixelFormat(), app_refresh_screen(), updateDisplayTitleBar()
  // Can be replaced with a ContextObserver listening to the
  // onActiveSiteChange() event.
  notifyActiveSiteChanged();
}

void UIContext::setActiveDocument(Document* document)
{
  bool notify = (m_lastSelectedDoc != document);
  m_lastSelectedDoc = document;

  DocumentView* docView = getFirstDocumentView(document);
  if (docView) {     // The view can be null if we are in --batch mode
    setActiveView(docView);
    notify = false;
  }

  if (notify)
    notifyActiveSiteChanged();
}

DocumentView* UIContext::getFirstDocumentView(doc::Document* document) const
{
  Workspace* workspace = App::instance()->workspace();
  if (!workspace) // Workspace (main window) can be null if we are in --batch mode
    return nullptr;

  for (WorkspaceView* view : *workspace) {
    if (DocumentView* docView = dynamic_cast<DocumentView*>(view)) {
      if (docView->document() == document) {
        return docView;
      }
    }
  }

  return nullptr;
}

DocumentViews UIContext::getAllDocumentViews(doc::Document* document) const
{
  Workspace* workspace = App::instance()->workspace();
  DocumentViews docViews;

  for (WorkspaceView* view : *workspace) {
    if (DocumentView* docView = dynamic_cast<DocumentView*>(view)) {
      if (docView->document() == document) {
        docViews.push_back(docView);
      }
    }
  }

  return docViews;
}

Editor* UIContext::activeEditor()
{
  DocumentView* view = activeView();
  if (view)
    return view->editor();
  else
    return NULL;
}

void UIContext::onAddDocument(doc::Document* doc)
{
  m_lastSelectedDoc = static_cast<app::Document*>(doc);

  // We don't create views in batch mode.
  if (!App::instance()->isGui())
    return;

  // Add a new view for this document
  DocumentView* view = new DocumentView(
    m_lastSelectedDoc,
    DocumentView::Normal,
    App::instance()->mainWindow()->getPreviewEditor());

  // Add a tab with the new view for the document
  App::instance()->workspace()->addView(view);

  setActiveView(view);
  view->editor()->setDefaultScroll();
}

void UIContext::onRemoveDocument(doc::Document* doc)
{
  if (doc == m_lastSelectedDoc)
    m_lastSelectedDoc = nullptr;

  // We don't destroy views in batch mode.
  if (isUIAvailable()) {
    Workspace* workspace = App::instance()->workspace();

    for (DocumentView* docView : getAllDocumentViews(doc)) {
      workspace->removeView(docView);
      delete docView;
    }
  }
}

void UIContext::onGetActiveSite(Site* site) const
{
  DocumentView* view = activeView();
  if (view) {
    view->getSite(site);

    if (site->sprite()) {
      // Selected layers
      Timeline* timeline = App::instance()->timeline();
      if (timeline &&
          timeline->range().enabled()) {
        switch (timeline->range().type()) {
          case DocumentRange::kCels:   site->focus(Site::InCels); break;
          case DocumentRange::kFrames: site->focus(Site::InFrames); break;
          case DocumentRange::kLayers: site->focus(Site::InLayers); break;
        }
        site->selectedLayers(timeline->selectedLayers());
        site->selectedFrames(timeline->selectedFrames());
      }
      else {
        ColorBar* colorBar = ColorBar::instance();
        if (colorBar &&
            colorBar->getPaletteView()->getSelectedEntriesCount() > 0) {
          site->focus(Site::InColorBar);
        }
        else {
          site->focus(Site::InEditor);
        }
      }
    }
  }
  // Default/dummy site (maybe for batch/command line mode)
  else if (!isUIAvailable()) {
    if (Document* doc = m_lastSelectedDoc) {
      site->document(doc);
      site->sprite(doc->sprite());
      site->layer(doc->sprite()->root()->firstLayer());
      site->frame(0);
    }
  }
}

} // namespace app
