/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jinete.h"

#include "sprite_wrappers.h"
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
  app_get_top_window()->dirty();

typedef std::vector<Editor*> EditorList;

Editor* current_editor = NULL;
Widget* box_editors = NULL;

static EditorList editors;

static int is_sprite_in_some_editor(Sprite *sprite);
static Sprite *get_more_reliable_sprite();
static JWidget find_next_editor(JWidget widget);
static int count_parents(JWidget widget);

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

/**
 * Removes the specified editor from the "editors" list.
 *
 * It does not delete the editor.
 */
void remove_editor(Editor* editor)
{
  EditorList::iterator it = std::find(editors.begin(), editors.end(), editor);

  ASSERT(it != editors.end());

  editors.erase(it);
}

void refresh_all_editors()
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    jwidget_dirty(*it);
  }
}

void update_editors_with_sprite(const Sprite* sprite)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = *it;

    if (sprite == editor->getSprite())
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

/* TODO improve this (with JRegion or something, and without
   recursivity) */
void editors_draw_sprite_tiled(const Sprite* sprite, int x1, int y1, int x2, int y2)
{
  int cx1, cy1, cx2, cy2;	/* cel rectangle */
  int lx1, ly1, lx2, ly2;	/* limited rectangle to the cel rectangle */
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

  /* draw the rectangles inside the editor */
  editors_draw_sprite(sprite, lx1, ly1, lx2, ly2);

  /* left */
  if (x1 < cx1 && lx2 < cx2) {
    editors_draw_sprite_tiled(sprite,
			      MAX(lx2+1, cx2+1+(x1-cx1)), y1,
			      cx2, y2);
  }

  /* top */
  if (y1 < cy1 && ly2 < cy2) {
    editors_draw_sprite_tiled(sprite,
			      x1, MAX(ly2+1, cy2+1+(y1-cx1)),
			      x2, cy2);
  }

  /* right */
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

  /* bottom */
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

void editors_hide_sprite(const Sprite* sprite)
{
  UIContext* context = UIContext::instance();
  bool refresh = (context->get_current_sprite() == sprite) ? true: false;

  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = *it;

    if (sprite == editor->getSprite())
      editor->editor_set_sprite(get_more_reliable_sprite());
  }

  if (refresh) {
    Sprite* sprite = current_editor->getSprite();

    context->set_current_sprite(sprite);
    app_refresh_screen(sprite);
  }
}

void set_current_editor(Editor* editor)
{
  if (current_editor != editor) {
    if (current_editor)
      jwidget_dirty(jwidget_get_view(current_editor));

    current_editor = editor;

    jwidget_dirty(jwidget_get_view(current_editor));

    UIContext* context = UIContext::instance();
    Sprite* sprite = current_editor->getSprite();
    context->set_current_sprite(sprite);

    app_refresh_screen(sprite);
    app_realloc_sprite_list();
  }
}

void set_sprite_in_current_editor(Sprite *sprite)
{
  if (current_editor) {
    UIContext* context = UIContext::instance();
    
    context->set_current_sprite(sprite);
    if (sprite != NULL)
      context->send_sprite_to_top(sprite);

    current_editor->editor_set_sprite(sprite);

    jwidget_dirty(jwidget_get_view(current_editor));

    app_refresh_screen(sprite);
    app_realloc_sprite_list();
  }
}

void set_sprite_in_more_reliable_editor(Sprite* sprite)
{
  /* the current editor */
  Editor* best = current_editor;

  /* search for any empty editor */
  if (best->getSprite()) {
    for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
      Editor* editor = *it;

      if (!editor->getSprite()) {
	best = editor;
	break;
      }
    }
  }

  set_current_editor(best);
  set_sprite_in_current_editor(sprite);
}

void split_editor(Editor* editor, int align)
{
  if (count_parents(editor) > 10) {
    jalert(_("Error<<You cannot split this editor more||&Close"));
    return;
  }

  JWidget view = jwidget_get_view(editor);
  JWidget parent_box = jwidget_get_parent(view); /* box or panel */

  /* create a new box to contain both editors, and a new view to put
     the new editor */
  JWidget new_panel = jpanel_new(align);
  JWidget new_view = editor_view_new();
  Editor* new_editor = create_new_editor();

  /* insert the "new_box" in the same location that the view */
  jwidget_replace_child(parent_box, view, new_panel);

  /* append the new editor */
  jview_attach(new_view, new_editor);

  /* set the sprite for the new editor */
  new_editor->editor_set_sprite(editor->getSprite());
  new_editor->editor_set_zoom(editor->editor_get_zoom());

  /* expansive widgets */
  jwidget_expansive(new_panel, true);
  jwidget_expansive(new_view, true);

  /* append both views to the "new_panel" */
  jwidget_add_child(new_panel, view);
  jwidget_add_child(new_panel, new_view);

  /* same position */
  {
    int scroll_x, scroll_y;

    jview_get_scroll(view, &scroll_x, &scroll_y);
    jview_set_scroll(new_view, scroll_x, scroll_y);

    jrect_copy(new_view->rc, view->rc);
    jrect_copy(jview_get_viewport(new_view)->rc,
	       jview_get_viewport(view)->rc);
    jrect_copy(new_editor->rc, editor->rc);

    new_editor->editor_set_offset_x(editor->editor_get_offset_x());
    new_editor->editor_set_offset_y(editor->editor_get_offset_y());
  }

  /* fixup window */
  FIXUP_TOP_WINDOW();

  /* update both editors */
  editor->editor_update();
  new_editor->editor_update();
}

void close_editor(Editor* editor)
{
  JWidget view = jwidget_get_view(editor);
  JWidget parent_box = jwidget_get_parent(view); /* box or panel */
  JWidget other_widget;

  /* you can't remove all editors */
  if (editors.size() == 1)
    return;

  /* deselect the editor */
  if (editor == current_editor)
    current_editor = 0;

  /* remove this editor */
  jwidget_remove_child(parent_box, view);
  jwidget_free(view);

  /* fixup the parent */
  other_widget = reinterpret_cast<JWidget>(jlist_first_data(parent_box->children));

  jwidget_remove_child(parent_box, other_widget);
  jwidget_replace_child(jwidget_get_parent(parent_box),
			  parent_box, other_widget);
  jwidget_free(parent_box);

  /* find next editor to select */
  if (!current_editor) {
    JWidget next_editor = find_next_editor(other_widget);
    if (next_editor) {
      ASSERT(next_editor->type == editor_type());

      set_current_editor(static_cast<Editor*>(next_editor));
    }
  }

  /* fixup window */
  FIXUP_TOP_WINDOW();

  /* update all editors */
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = *it;
    editor->editor_update();
  }
}

void make_unique_editor(Editor* editor)
{
  JWidget view = jwidget_get_view(editor);
  JLink link, next;
  JWidget child;

  /* it's the unique editor */
  if (editors.size() == 1)
    return;

  /* remove the editor-view of its parent */
  jwidget_remove_child(jwidget_get_parent(view), view);

  /* remove all children of main_editor_box */
  JI_LIST_FOR_EACH_SAFE(box_editors->children, link, next) {
    child = (JWidget)link->data;

    jwidget_remove_child(box_editors, child);
    delete child; // widget
  }

  /* append the editor to main box */
  jwidget_add_child(box_editors, view);

  /* new current editor */
  set_current_editor(editor);

  /* fixup window */
  FIXUP_TOP_WINDOW();

  /* update new editor */
  editor->editor_update();
}

static int is_sprite_in_some_editor(Sprite* sprite)
{
  for (EditorList::iterator it = editors.begin(); it != editors.end(); ++it) {
    Editor* editor = *it;

    if (sprite == editor->getSprite())
      return true;
  }
  return false;
}

/**
 * Returns the next sprite that should be show if we close the current
 * one.
 */
static Sprite* get_more_reliable_sprite()
{
  UIContext* context = UIContext::instance();
  const SpriteList& list = context->get_sprite_list();

  for (SpriteList::const_iterator
	 it = list.begin(); it != list.end(); ++it) {
    Sprite* sprite = *it;
    if (!(is_sprite_in_some_editor(sprite)))
      return sprite;
  }

  return NULL;
}

static JWidget find_next_editor(JWidget widget)
{
  JWidget editor = NULL;
  JLink link;

  if (widget->type == JI_VIEW)
    editor = reinterpret_cast<JWidget>(jlist_first_data(jview_get_viewport(widget)->children));
  else {
    JI_LIST_FOR_EACH(widget->children, link)
      if ((editor = find_next_editor(reinterpret_cast<JWidget>(link->data))))
	break;
  }

  return editor;
}

static int count_parents(JWidget widget)
{
  int count = 0;
  while ((widget = jwidget_get_parent(widget)))
    count++;
  return count;
}
