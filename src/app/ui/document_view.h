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

#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "doc/document_observer.h"
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
                     , public doc::DocumentObserver
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
    std::string getTabText() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    WorkspaceView* cloneWorkspaceView() override;
    void onWorkspaceViewSelected() override;
    void onClonedFrom(WorkspaceView* from) override;

    // DocumentObserver implementation
    void onGeneralUpdate(doc::DocumentEvent& ev) override;
    void onSpritePixelsModified(doc::DocumentEvent& ev) override;
    void onLayerMergedDown(doc::DocumentEvent& ev) override;
    void onAddLayer(doc::DocumentEvent& ev) override;
    void onBeforeRemoveLayer(doc::DocumentEvent& ev) override;
    void onAddFrame(doc::DocumentEvent& ev) override;
    void onRemoveFrame(doc::DocumentEvent& ev) override;
    void onTotalFramesChanged(doc::DocumentEvent& ev) override;
    void onLayerRestacked(doc::DocumentEvent& ev) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;

  private:
    Document* m_document;
    ui::View* m_view;
    Editor* m_editor;
  };

} // namespace app

#endif
