/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include <vector>
#include <algorithm>

#include "gui/gui.h"

#include "document_wrappers.h"
#include "ui_context.h"
#include "app.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/editor.h"

#define FIXUP_TOP_WINDOW()			\
  app_get_top_window()->remap_window();		\
  app_get_top_window()->invalidate();

typedef std::vector<Editor*> EditorList;

Editor* current_editor = NULL;
Widget* box_editors = NULL;

static EditorList editors;

static int is_document_in_some_editor(Document* document);
static Document* get_more_reliable_document();
static Widget* find_next_editor(Widget* widget);
static int count_parents(Widget* widget);

int init_module_editors()
{
  return 0;
}

void exit_module_editors()
{
  editors.clear();
}

Editor* create_new_editor()
{
  Editor* editor = new Editor();
  editors.push_back(editor);
  return editor;
}

// Removes the specified editor from the "editors" list.
// It does not delete the editor.
void remove_editor(Editor* editor)
{
  EditorList::iterator it = std::find(editors.begin(), editors.end(), editor);

  ASSERT(it != editors.end());

  editors.erase(it);
}

void refresh_all_editors()
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    (*it)->invalidate();
  }
}

void update_editors_with_document(const Document* document)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = *it;

    if (document == editor->getDocument())
      editor->editor_update();
  }
}

void editors_draw_sprite(const Sprite* sprite, int x1, int y1, int x2, int y2)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = *it;

    if (sprite == editor->getSprite())
      editor->editor_draw_sprite_safe(x1, y1, x2, y2);
  }
}

// TODO improve this (with JRegion or something, and without recursivity).
void editors_draw_sprite_tiled(const Sprite* sprite, int x1, int y1, int x2, int y2)
{
  int cx1, cy1, cx2, cy2;	// Cel rectangle.
  int lx1, ly1, lx2, ly2;	// Limited rectangle to the cel rectangle.
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
    Editor* editor = *it;

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
    if (current_editor)
      View::getView(current_editor)->invalidate();

    current_editor = editor;

    View::getView(current_editor)->invalidate();

    UIContext* context = UIContext::instance();
    Document* document = current_editor->getDocument();
    context->setActiveDocument(document);

    app_refresh_screen(document);
    app_rebuild_documents_tabs();
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
      Editor* editor = *it;

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
  View* new_view = editor_view_new();
  Editor* new_editor = create_new_editor();

  // Insert the "new_box" in the same location that the view.
  jwidget_replace_child(parent_box, view, new_panel);

  // Append the new editor.
  new_view->attachToView(new_editor);

  // Set the sprite for the new editor.
  new_editor->setDocument(editor->getDocument());
  new_editor->editor_set_zoom(editor->editor_get_zoom());

  // Expansive widgets.
  jwidget_expansive(new_panel, true);
  jwidget_expansive(new_view, true);

  // Append both views to the "new_panel".
  jwidget_add_child(new_panel, view);
  jwidget_add_child(new_panel, new_view);

  // Same position.
  {
    new_view->setViewScroll(view->getViewScroll());

    jrect_copy(new_view->rc, view->rc);
    jrect_copy(new_view->getViewport()->rc, view->getViewport()->rc);
    jrect_copy(new_editor->rc, editor->rc);

    new_editor->editor_set_offset_x(editor->editor_get_offset_x());
    new_editor->editor_set_offset_y(editor->editor_get_offset_y());
  }

  // Fixup window.
  FIXUP_TOP_WINDOW();

  // Update both editors.
  editor->editor_update();
  new_editor->editor_update();
}

void close_editor(Editor* editor)
{
  View* view = View::getView(editor);
  JWidget parent_box = view->getParent(); // Box or panel
  JWidget other_widget;

  // You can't remove all editors.
  if (editors.size() == 1)
    return;

  // Deselect the editor.
  if (editor == current_editor)
    current_editor = 0;

  // Remove this editor.
  jwidget_remove_child(parent_box, view);
  jwidget_free(view);

  // Fixup the parent.
  other_widget = reinterpret_cast<JWidget>(jlist_first_data(parent_box->children));

  jwidget_remove_child(parent_box, other_widget);
  jwidget_replace_child(parent_box->getParent(), parent_box, other_widget);
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
    Editor* editor = *it;
    editor->editor_update();
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
  jwidget_remove_child(view->getParent(), view);

  // Remove all children of main_editor_box.
  JI_LIST_FOR_EACH_SAFE(box_editors->children, link, next) {
    child = (JWidget)link->data;

    jwidget_remove_child(box_editors, child);
    delete child; // widget
  }

  // Append the editor to main box.
  jwidget_add_child(box_editors, view);

  // New current editor.
  set_current_editor(editor);

  // Fixup window.
  FIXUP_TOP_WINDOW();

  // Update new editor.
  editor->editor_update();
}

static int is_document_in_some_editor(Document* document)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = *it;

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
