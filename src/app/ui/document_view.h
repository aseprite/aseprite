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

#ifndef APP_UI_DOCUMENT_VIEW_H_INCLUDED
#define APP_UI_DOCUMENT_VIEW_H_INCLUDED
#pragma once

#include "app/document_observer.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "base/compiler_specific.h"
#include "ui/box.h"

namespace ui {
  class View;
}

namespace app {
  class Document;
  class DocumentLocation;
  class Editor;

  class DocumentView : public ui::Box
                     , public TabView
                     , public DocumentObserver
                     , public WorkspaceView {
  public:
    enum Type {
      Normal,
      Mini
    };

    DocumentView(Document* document, Type type);
    ~DocumentView();

    Document* getDocument() const { return m_document; }
    void getDocumentLocation(DocumentLocation* location) const;
    Editor* getEditor() { return m_editor; }

    // TabView implementation
    std::string getTabText() OVERRIDE;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() OVERRIDE { return this; }
    WorkspaceView* cloneWorkspaceView() OVERRIDE;
    void onClonedFrom(WorkspaceView* from) OVERRIDE;

    // DocumentObserver implementation
    void onGeneralUpdate(DocumentEvent& ev) OVERRIDE;
    void onSpritePixelsModified(DocumentEvent& ev) OVERRIDE;
    void onLayerMergedDown(DocumentEvent& ev) OVERRIDE;
    void onAddLayer(DocumentEvent& ev) OVERRIDE;
    void onBeforeRemoveLayer(DocumentEvent& ev) OVERRIDE;
    void onAddFrame(DocumentEvent& ev) OVERRIDE;
    void onRemoveFrame(DocumentEvent& ev) OVERRIDE;
    void onTotalFramesChanged(DocumentEvent& ev) OVERRIDE;
    void onLayerRestacked(DocumentEvent& ev) OVERRIDE;

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;

  private:
    Document* m_document;
    ui::View* m_view;
    Editor* m_editor;
  };

} // namespace app

#endif
