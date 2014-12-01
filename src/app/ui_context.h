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

#ifndef APP_UI_CONTEXT_H_INCLUDED
#define APP_UI_CONTEXT_H_INCLUDED
#pragma once

#include "app/context.h"
#include "doc/documents_observer.h"

namespace app {
  class DocumentView;
  class Editor;

  typedef std::vector<DocumentView*> DocumentViews;

  class UIContext : public app::Context {
  public:
    static UIContext* instance() { return m_instance; }

    UIContext();
    virtual ~UIContext();

    bool isUiAvailable() const override;

    DocumentView* activeView() const;
    void setActiveView(DocumentView* documentView);

    // Returns the number of views that the given document has.
    size_t countViewsOf(Document* document) const;

    DocumentView* getFirstDocumentView(Document* document) const;

    // Returns the current editor. It can be null.
    Editor* activeEditor();

    // Returns the active editor for the given document, or creates a
    // new one if it's necessary.
    Editor* getEditorFor(Document* document);

  protected:
    virtual void onAddDocument(doc::Document* doc) override;
    virtual void onRemoveDocument(doc::Document* doc) override;
    virtual void onGetActiveLocation(DocumentLocation* location) const override;

  private:
    DocumentView* m_lastSelectedView;
    static UIContext* m_instance;
  };

} // namespace app

#endif
