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

#include "widgets/document_view.h"

#include "app.h"
#include "base/path.h"
#include "document_event.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "ui/accelerator.h"
#include "ui/message.h"
#include "ui/view.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_customization_delegate.h"
#include "widgets/editor/editor_view.h"
#include "widgets/main_window.h"
#include "widgets/mini_editor.h"
#include "widgets/workspace.h"

using namespace ui;
using namespace widgets;

class AppEditor : public Editor,
                  public EditorObserver,
                  public EditorCustomizationDelegate
{
public:
  AppEditor(Document* document) : Editor(document) {
    addObserver(this);
    setCustomizationDelegate(this);
  }

  ~AppEditor() {
    removeObserver(this);
    setCustomizationDelegate(NULL);
  }

  // EditorObserver implementation
  void dispose() OVERRIDE {
    App::instance()->getMainWindow()->getMiniEditor()->updateUsingEditor(NULL);
  }

  void scrollChanged(Editor* editor) OVERRIDE {
    App::instance()->getMainWindow()->getMiniEditor()->updateUsingEditor(this);
  }

  void stateChanged(Editor* editor) OVERRIDE {
    // Do nothing
  }

  // EditorCustomizationDelegate implementation
  tools::Tool* getQuickTool(tools::Tool* currentTool) OVERRIDE {
    return get_selected_quicktool(currentTool);
  }

  bool isCopySelectionKeyPressed() OVERRIDE {
    Accelerator* accel = get_accel_to_copy_selection();
    if (accel)
      return accel->checkFromAllegroKeyArray();
    else
      return false;
  }

  bool isSnapToGridKeyPressed() OVERRIDE {
    Accelerator* accel = get_accel_to_snap_to_grid();
    if (accel)
      return accel->checkFromAllegroKeyArray();
    else
      return false;
  }

  bool isAngleSnapKeyPressed() OVERRIDE {
    Accelerator* accel = get_accel_to_angle_snap();
    if (accel)
      return accel->checkFromAllegroKeyArray();
    else
      return false;
  }

  bool isMaintainAspectRatioKeyPressed() OVERRIDE {
    Accelerator* accel = get_accel_to_maintain_aspect_ratio();
    if (accel)
      return accel->checkFromAllegroKeyArray();
    else
      return false;
  }

  bool isLockAxisKeyPressed() OVERRIDE {
    Accelerator* accel = get_accel_to_lock_axis();
    if (accel)
      return accel->checkFromAllegroKeyArray();
    else
      return false;
  }
};

DocumentView::DocumentView(Document* document, Type type)
  : Box(JI_VERTICAL)
  , m_document(document)
  , m_view(new EditorView(type == Normal ? EditorView::CurrentEditorMode:
                                           EditorView::AlwaysSelected))
  , m_editor(type == Normal ? new AppEditor(document):
                              new Editor(document))
{
  addChild(m_view);

  m_view->attachToView(m_editor);
  m_view->setExpansive(true);
  m_view->hideScrollBars();

  m_editor->setDocumentView(this);
  m_document->addObserver(this);
}

DocumentView::~DocumentView()
{
  m_document->removeObserver(this);
  delete m_editor;
}

std::string DocumentView::getTabText()
{
  std::string str = base::get_file_name(m_document->getFilename());

  // Add an asterisk if the document is modified.
  if (m_document->isModified())
    str += "*";

  return str;
}

bool DocumentView::onProcessMessage(Message* msg)
{
  switch (msg->type) {
    case JM_FOCUSENTER:
      m_editor->requestFocus();
      break;
  }
  return Box::onProcessMessage(msg);
}

void DocumentView::onGeneralUpdate(DocumentEvent& ev)
{
  if (m_editor->isVisible())
    m_editor->updateEditor();
}

void DocumentView::onSpritePixelsModified(DocumentEvent& ev)
{
  if (m_editor->isVisible())
    m_editor->drawSpriteClipped(ev.region());
}
