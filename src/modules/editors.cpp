/* ASE - Allegro Sprite Editor
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

#include "modules/editors.h"

#include "app.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "ui_context.h"
#include "util/misc.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_customization_delegate.h"
#include "widgets/editor/editor_view.h"
#include "widgets/popup_frame_pin.h"
#include "widgets/statebar.h"

#include <algorithm>
#include <vector>

#define FIXUP_TOP_WINDOW()                      \
  app_get_top_window()->remap_window();         \
  app_get_top_window()->invalidate();

class EditorItem
{
public:
  enum Type { Normal, Mini };

  EditorItem(Editor* editor, Type type)
    : m_editor(editor)
    , m_type(type)
  { }

  Editor* getEditor() const { return m_editor; }
  Type getType() const { return m_type; }

private:
  Editor* m_editor;
  Type m_type;
};

typedef std::vector<EditorItem> EditorList;

Editor* current_editor = NULL;
Widget* box_editors = NULL;

static EditorList editors;

static Frame* mini_editor_frame = NULL;
static bool mini_editor_enabled = true; // True if the user wants to use the mini editor
static Editor* mini_editor = NULL;

static int is_document_in_some_editor(Document* document);
static Document* get_more_reliable_document();
static Widget* find_next_editor(Widget* widget);
static int count_parents(Widget* widget);

static void create_mini_editor_frame();
static void hide_mini_editor_frame();
static void update_mini_editor_frame(Editor* editor);
static void on_mini_editor_frame_close(CloseEvent& ev);

class WrappedEditor : public Editor,
                      public EditorListener,
                      public EditorCustomizationDelegate
{
public:
  WrappedEditor() {
    addListener(this);
    setCustomizationDelegate(this);
  }

  ~WrappedEditor() {
    removeListener(this);
    setCustomizationDelegate(NULL);
  }

  // EditorListener implementation
  void dispose() OVERRIDE {
    // Do nothing
  }

  void scrollChanged(Editor* editor) OVERRIDE {
    update_mini_editor_frame(editor);
  }

  void documentChanged(Editor* editor) OVERRIDE {
    if (editor == current_editor)
      update_mini_editor_frame(editor);
  }

  void stateChanged(Editor* editor) OVERRIDE {
    // Do nothing
  }

  // EditorCustomizationDelegate implementation
  tools::Tool* getQuickTool(tools::Tool* currentTool) OVERRIDE {
    return get_selected_quicktool(currentTool);
  }

  bool isCopySelectionKeyPressed() OVERRIDE {
    JAccel accel = get_accel_to_copy_selection();
    if (accel)
      return jaccel_check_from_key(accel);
    else
      return false;
  }

};

class MiniEditor : public Editor
{
public:
  MiniEditor() {
  }

  bool changePreferredSettings() OVERRIDE {
    return false;
  }
};

int init_module_editors()
{
  mini_editor_enabled = get_config_bool("MiniEditor", "Enabled", true);
  return 0;
}

void exit_module_editors()
{
  set_config_bool("MiniEditor", "Enabled", mini_editor_enabled);

  if (mini_editor_frame) {
    save_window_pos(mini_editor_frame, "MiniEditor");

    delete mini_editor_frame;
    mini_editor_frame = NULL;
  }

  ASSERT(editors.empty());
}

Editor* create_new_editor()
{
  Editor* editor = new WrappedEditor();
  editors.push_back(EditorItem(editor, EditorItem::Normal));
  return editor;
}

// Removes the specified editor from the "editors" list.
// It does not delete the editor.
void remove_editor(Editor* editor)
{
  for (EditorList::iterator
         it = editors.begin(),
         end = editors.end(); it != end; ++it) {
    if (it->getEditor() == editor) {
      editors.erase(it);
      return;
    }
  }

  ASSERT(false && "Editor not found in the list");
}

void refresh_all_editors()
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    it->getEditor()->invalidate();
  }
}

void update_editors_with_document(const Document* document)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = it->getEditor();

    if (document == editor->getDocument())
      editor->updateEditor();
  }
}

void editors_draw_sprite(const Sprite* sprite, int x1, int y1, int x2, int y2)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = it->getEditor();

    if (sprite == editor->getSprite() && editor->isVisible())
      editor->drawSpriteSafe(x1, y1, x2, y2);
  }
}

// TODO improve this (with JRegion or something, and without recursivity).
void editors_draw_sprite_tiled(const Sprite* sprite, int x1, int y1, int x2, int y2)
{
  int cx1, cy1, cx2, cy2;       // Cel rectangle.
  int lx1, ly1, lx2, ly2;       // Limited rectangle to the cel rectangle.
#ifdef TILED_IN_LAYER
  Image *image = GetImage2(sprite, &cx1, &cy1, NULL);
  cx2 = cx1+image->w-1;
  cy2 = cy1+image->h-1;
#else
  cx1 = 0;
  cy1 = 0;
  cx2 = cx1+sprite->getWidth()-1;
  cy2 = cy1+sprite->getHeight()-1;
#endif

  lx1 = MAX(x1, cx1);
  ly1 = MAX(y1, cy1);
  lx2 = MIN(x2, cx2);
  ly2 = MIN(y2, cy2);

  // Draw the rectangles inside the editor.
  editors_draw_sprite(sprite, lx1, ly1, lx2, ly2);

  // Left.
  if (x1 < cx1 && lx2 < cx2) {
    editors_draw_sprite_tiled(sprite,
                              MAX(lx2+1, cx2+1+(x1-cx1)), y1,
                              cx2, y2);
  }

  // Top.
  if (y1 < cy1 && ly2 < cy2) {
    editors_draw_sprite_tiled(sprite,
                              x1, MAX(ly2+1, cy2+1+(y1-cx1)),
                              x2, cy2);
  }

  // Right.
  if (x2 >= cx2+1 && lx1 > cx1) {
#ifdef TILED_IN_LAYER
    editors_draw_sprite_tiled(sprite,
                              cx1, y1,
                              MIN(lx1-1, x2-image->w), y2);
#else
    editors_draw_sprite_tiled(sprite,
                              cx1, y1,
                              MIN(lx1-1, x2-sprite->getWidth()), y2);
#endif
  }

  // Bottom.
  if (y2 >= cy2+1 && ly1 > cy1) {
#if TILED_IN_LAYER
    editors_draw_sprite_tiled(sprite,
                              x1, cy1,
                              x2, MIN(ly1-1, y2-image->h));
#else
    editors_draw_sprite_tiled(sprite,
                              x1, cy1,
                              x2, MIN(ly1-1, y2-sprite->getHeight()));
#endif
  }
}

void editors_hide_document(const Document* document)
{
  UIContext* context = UIContext::instance();
  Document* activeDocument = context->getActiveDocument();
  Sprite* activeSprite = (activeDocument ? activeDocument->getSprite(): NULL);
  bool refresh = (activeSprite == document->getSprite()) ? true: false;

  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = it->getEditor();

    if (document == editor->getDocument())
      editor->setDocument(get_more_reliable_document());
  }

  if (refresh) {
    Document* document = current_editor->getDocument();

    context->setActiveDocument(document);
    app_refresh_screen(document);
  }
}

void set_current_editor(Editor* editor)
{
  if (current_editor != editor) {
    // Here we check if the specified editor in the parameter is the
    // mini-editor, in this case, we cannot put the mini-editor as the
    // current one.
    for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
      if (it->getEditor() == editor) {
        if (it->getType() != EditorItem::Normal) {
          // Avoid setting the mini-editor as the current one
          return;
        }
        break;
      }
    }

    if (current_editor)
      View::getView(current_editor)->invalidate();

    current_editor = editor;

    View::getView(current_editor)->invalidate();

    UIContext* context = UIContext::instance();
    Document* document = current_editor->getDocument();
    context->setActiveDocument(document);

    app_refresh_screen(document);
    app_rebuild_documents_tabs();

    update_mini_editor_frame(editor);
  }
}

void set_document_in_current_editor(Document* document)
{
  if (current_editor) {
    UIContext* context = UIContext::instance();

    context->setActiveDocument(document);
    if (document != NULL)
      context->sendDocumentToTop(document);

    current_editor->setDocument(document);

    View::getView(current_editor)->invalidate();

    app_refresh_screen(document);
    app_rebuild_documents_tabs();
  }
}

void set_document_in_more_reliable_editor(Document* document)
{
  // The current editor
  Editor* best = current_editor;

  // Search for any empty editor
  if (best->getDocument()) {
    for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
      // Avoid using abnormal editors (mini, etc.)
      if (it->getType() != EditorItem::Normal)
        continue;

      Editor* editor = it->getEditor();

      if (!editor->getDocument()) {
        best = editor;
        break;
      }
    }
  }

  set_current_editor(best);
  set_document_in_current_editor(document);
}

void split_editor(Editor* editor, int align)
{
  if (count_parents(editor) > 10) {
    Alert::show("Error<<You cannot split this editor more||&Close");
    return;
  }

  View* view = View::getView(editor);
  JWidget parent_box = view->getParent(); // Box or panel.

  // Create a new box to contain both editors, and a new view to put the new editor.
  JWidget new_panel = jpanel_new(align);
  View* new_view = new EditorView(EditorView::CurrentEditorMode);
  Editor* new_editor = create_new_editor();

  // Insert the "new_box" in the same location that the view.
  parent_box->replaceChild(view, new_panel);

  // Append the new editor.
  new_view->attachToView(new_editor);

  // Set the sprite for the new editor.
  new_editor->setDocument(editor->getDocument());
  new_editor->setZoom(editor->getZoom());

  // Expansive widgets.
  jwidget_expansive(new_panel, true);
  jwidget_expansive(new_view, true);

  // Append both views to the "new_panel".
  new_panel->addChild(view);
  new_panel->addChild(new_view);

  // Same position.
  {
    new_view->setViewScroll(view->getViewScroll());

    jrect_copy(new_view->rc, view->rc);
    jrect_copy(new_view->getViewport()->rc, view->getViewport()->rc);
    jrect_copy(new_editor->rc, editor->rc);

    new_editor->setOffsetX(editor->getOffsetX());
    new_editor->setOffsetY(editor->getOffsetY());
  }

  // Fixup window.
  FIXUP_TOP_WINDOW();

  // Update both editors.
  editor->updateEditor();
  new_editor->updateEditor();
}

void close_editor(Editor* editor)
{
  View* view = View::getView(editor);
  JWidget parent_box = view->getParent(); // Box or panel
  JWidget other_widget;

  // You can't remove all (normal) editors.
  int normalEditors = 0;
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    if (it->getType() == EditorItem::Normal)
      normalEditors++;
  }
  if (normalEditors == 1) // In this case we avoid to remove the last normal editor
    return;

  // Deselect the editor.
  if (editor == current_editor)
    current_editor = 0;

  // Remove this editor.
  parent_box->removeChild(view);
  jwidget_free(view);

  // Fixup the parent.
  other_widget = reinterpret_cast<JWidget>(jlist_first_data(parent_box->children));

  parent_box->removeChild(other_widget);
  parent_box->getParent()->replaceChild(parent_box, other_widget);
  jwidget_free(parent_box);

  // Find next editor to select.
  if (!current_editor) {
    JWidget next_editor = find_next_editor(other_widget);
    if (next_editor) {
      ASSERT(next_editor->type == editor_type());

      set_current_editor(static_cast<Editor*>(next_editor));
    }
  }

  // Fixup window.
  FIXUP_TOP_WINDOW();

  // Update all editors.
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = it->getEditor();
    editor->updateEditor();
  }
}

void make_unique_editor(Editor* editor)
{
  View* view = View::getView(editor);
  JLink link, next;
  JWidget child;

  // It's the unique editor.
  if (editors.size() == 1)
    return;

  // Remove the editor-view of its parent.
  view->getParent()->removeChild(view);

  // Remove all children of main_editor_box.
  JI_LIST_FOR_EACH_SAFE(box_editors->children, link, next) {
    child = (JWidget)link->data;

    box_editors->removeChild(child);
    delete child; // widget
  }

  // Append the editor to main box.
  box_editors->addChild(view);

  // New current editor.
  set_current_editor(editor);

  // Fixup window.
  FIXUP_TOP_WINDOW();

  // Update new editor.
  editor->updateEditor();
}

bool is_mini_editor_enabled()
{
  return mini_editor_enabled;
}

void enable_mini_editor(bool state)
{
  mini_editor_enabled = state;

  update_mini_editor_frame(current_editor);
}

static int is_document_in_some_editor(Document* document)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = it->getEditor();

    if (document == editor->getDocument())
      return true;
  }
  return false;
}

// Returns the next sprite that should be show if we close the current one.
static Document* get_more_reliable_document()
{
  UIContext* context = UIContext::instance();
  const Documents& docs = context->getDocuments();

  for (Documents::const_iterator
         it = docs.begin(), end = docs.end(); it != end; ++it) {
    Document* document = *it;
    if (!(is_document_in_some_editor(document)))
      return document;
  }

  return NULL;
}

static Widget* find_next_editor(Widget* widget)
{
  Widget* editor = NULL;
  JLink link;

  if (widget->type == JI_VIEW) {
    editor = reinterpret_cast<Widget*>(jlist_first_data(static_cast<View*>(widget)->getViewport()->children));
  }
  else {
    JI_LIST_FOR_EACH(widget->children, link)
      if ((editor = find_next_editor(reinterpret_cast<JWidget>(link->data))))
        break;
  }

  return editor;
}

static int count_parents(Widget* widget)
{
  int count = 0;
  while ((widget = widget->getParent()))
    count++;
  return count;
}

static void create_mini_editor_frame()
{
  // Create mini-editor
  mini_editor_frame = new Frame(false, "Mini-Editor");
  mini_editor_frame->child_spacing = 0;
  mini_editor_frame->set_autoremap(false);
  mini_editor_frame->set_wantfocus(false);

  // Hook Close button to disable mini-editor when the frame is closed.
  mini_editor_frame->Close.connect(&on_mini_editor_frame_close);

  // Create the new for the mini editor
  View* newView = new EditorView(EditorView::AlwaysSelected);
  jwidget_expansive(newView, true);

  // Create mini editor
  mini_editor = new MiniEditor();
  editors.push_back(EditorItem(mini_editor, EditorItem::Mini));

  newView->attachToView(mini_editor);

  mini_editor_frame->addChild(newView);

  // Default bounds
  int width = JI_SCREEN_W/4;
  int height = JI_SCREEN_H/4;
  mini_editor_frame->setBounds
    (gfx::Rect(JI_SCREEN_W - width - jrect_w(app_get_toolbar()->rc),
               JI_SCREEN_H - height - jrect_h(app_get_statusbar()->rc),
               width, height));

  load_window_pos(mini_editor_frame, "MiniEditor");
}

static void hide_mini_editor_frame()
{
  if (mini_editor_frame &&
      mini_editor_frame->isVisible()) {
    mini_editor_frame->closeWindow(NULL);
  }
}

static void update_mini_editor_frame(Editor* editor)
{
  if (!mini_editor_enabled || !editor) {
    hide_mini_editor_frame();
    return;
  }

  Document* document = editor->getDocument();

  // Show the mini editor if it wasn't created yet and the user
  // zoomed in, or if the mini-editor was created and the zoom of
  // both editors is not the same.
  if (document && document->getSprite() &&
      ((!mini_editor && editor->getZoom() > 0) ||
       (mini_editor && mini_editor->getZoom() != editor->getZoom()))) {
    // If the mini frame does not exist, create it
    if (!mini_editor_frame)
      create_mini_editor_frame();

    if (!mini_editor_frame->isVisible())
      mini_editor_frame->open_window_bg();

    gfx::Rect visibleBounds = editor->getVisibleSpriteBounds();
    gfx::Point pt = visibleBounds.getCenter();

    // Set the same location as in the given editor.
    if (mini_editor->getDocument() != document) {
      mini_editor->setDocument(document);
      mini_editor->setZoom(0);
      mini_editor->setState(EditorStatePtr(new EditorState));
    }

    mini_editor->centerInSpritePoint(pt.x, pt.y);
  }
  else {
    hide_mini_editor_frame();
  }
}

static void on_mini_editor_frame_close(CloseEvent& ev)
{
  if (ev.getTrigger() == CloseEvent::ByUser) {
    // Here we don't use "enable_mini_editor" to change the state of
    // "mini_editor_enabled" because we're coming from a close event
    // of the frame.
    mini_editor_enabled = false;

    // Redraw the tool bar because it shows the mini editor enabled state.
    // TODO abstract this event
    app_get_toolbar()->invalidate();
  }
}
