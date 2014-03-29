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

#include "app/ui/document_view.h"

#include "app/app.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/main_window.h"
#include "app/ui/mini_editor.h"
#include "app/ui/workspace.h"
#include "base/path.h"
#include "app/document_event.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/accelerator.h"
#include "ui/message.h"
#include "ui/view.h"

namespace app {

using namespace ui;

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

  void onScrollChanged(Editor* editor) OVERRIDE {
    App::instance()->getMainWindow()->getMiniEditor()->updateUsingEditor(this);
  }

  void onStateChanged(Editor* editor) OVERRIDE {
    // Do nothing
  }

  void onFrameChanged(Editor* editor) OVERRIDE {
    App::instance()->getMainWindow()->getMiniEditor()->updateUsingEditor(this);
  }

  void onLayerChanged(Editor* editor) OVERRIDE {
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
  , m_editor(type == Normal ?
    new AppEditor(document):
    new Editor(document, Editor::kNoneFlag)) // Don't show grid/mask in mini preview
{
  addChild(m_view);

  m_view->attachToView(m_editor);
  m_view->setExpansive(true);

  m_editor->setDocumentView(this);
  m_document->addObserver(this);
}

DocumentView::~DocumentView()
{
  m_document->removeObserver(this);
  delete m_editor;
}

void DocumentView::getDocumentLocation(DocumentLocation* location) const
{
  m_editor->getDocumentLocation(location);
}

std::string DocumentView::getTabText()
{
  std::string str = base::get_file_name(m_document->getFilename());

  // Add an asterisk if the document is modified.
  if (m_document->isModified())
    str += "*";

  return str;
}

WorkspaceView* DocumentView::cloneWorkspaceView()
{
  return new DocumentView(m_document, Normal);
}

void DocumentView::onClonedFrom(WorkspaceView* from)
{
  Editor* newEditor = getEditor();
  Editor* srcEditor = static_cast<DocumentView*>(from)->getEditor();

  newEditor->setLayer(srcEditor->getLayer());
  newEditor->setFrame(srcEditor->getFrame());
  newEditor->setZoom(srcEditor->getZoom());

  View::getView(newEditor)
    ->setViewScroll(View::getView(srcEditor)->getViewScroll());
}

bool DocumentView::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kFocusEnterMessage:
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

void DocumentView::onLayerMergedDown(DocumentEvent& ev)
{
  m_editor->setLayer(ev.targetLayer());
}

void DocumentView::onAddLayer(DocumentEvent& ev)
{
  if (current_editor == m_editor) {
    ASSERT(ev.layer() != NULL);
    m_editor->setLayer(ev.layer());
  }
}

void DocumentView::onBeforeRemoveLayer(DocumentEvent& ev)
{
  Sprite* sprite = ev.sprite();
  Layer* layer = ev.layer();

  // If the layer that was removed is the selected one
  if (layer == m_editor->getLayer()) {
    LayerFolder* parent = layer->getParent();
    Layer* layer_select = NULL;

    // Select previous layer, or next layer, or the parent (if it is
    // not the main layer of sprite set).
    if (layer->getPrevious())
      layer_select = layer->getPrevious();
    else if (layer->getNext())
      layer_select = layer->getNext();
    else if (parent != sprite->getFolder())
      layer_select = parent;

    m_editor->setLayer(layer_select);
  }
}

void DocumentView::onAddFrame(DocumentEvent& ev)
{
  if (current_editor == m_editor)
    m_editor->setFrame(ev.frame());
  else if (m_editor->getFrame() > ev.frame())
    m_editor->setFrame(m_editor->getFrame().next());
}

void DocumentView::onRemoveFrame(DocumentEvent& ev)
{
  // Adjust current frame of all editors that are in a frame more
  // advanced that the removed one.
  if (m_editor->getFrame() > ev.frame()) {
    m_editor->setFrame(m_editor->getFrame().previous());
  }
  // If the editor was in the previous "last frame" (current value of
  // getTotalFrames()), we've to adjust it to the new last frame
  // (getLastFrame())
  else if (m_editor->getFrame() >= m_editor->getSprite()->getTotalFrames()) {
    m_editor->setFrame(m_editor->getSprite()->getLastFrame());
  }
}

void DocumentView::onTotalFramesChanged(DocumentEvent& ev)
{
  if (m_editor->getFrame() >= m_editor->getSprite()->getTotalFrames()) {
    m_editor->setFrame(m_editor->getSprite()->getLastFrame());
  }
}

void DocumentView::onLayerRestacked(DocumentEvent& ev)
{
  m_editor->invalidate();
}

} // namespace app
