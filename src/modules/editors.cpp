/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "jinete/jinete.h"

#include "sprite_wrappers.h"
#include "ui_context.h"
#include "core/app.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/editor.h"

#define FIXUP_TOP_WINDOW()			\
  jwindow_remap(app_get_top_window());		\
  jwidget_dirty(app_get_top_window());

JWidget current_editor = NULL;
JWidget box_editors = NULL;

static JList editors;		/* list of "Editor" structures */

static int is_sprite_in_some_editor(Sprite *sprite);
static Sprite *get_more_reliable_sprite();
static JWidget find_next_editor(JWidget widget);
static int count_parents(JWidget widget);

int init_module_editors()
{
  editors = jlist_new();
  return 0;
}

void exit_module_editors()
{
  jlist_free(editors);
}

JWidget create_new_editor()
{
  JWidget editor = editor_new();

  /* add the new editor in the "editors" list */
  if (editor)
    jlist_append(editors, editor);

  return editor;
}

void remove_editor(JWidget editor)
{
  /* remove the new editor from the "editors" list */
  jlist_remove(editors, editor);
}

void refresh_all_editors()
{
  JLink link;

  JI_LIST_FOR_EACH(editors, link)
    jwidget_dirty(reinterpret_cast<JWidget>(link->data));
}

void update_editors_with_sprite(const Sprite *sprite)
{
  JWidget widget;
  JLink link;

  JI_LIST_FOR_EACH(editors, link) {
    widget = reinterpret_cast<JWidget>(link->data);

    if (sprite == editor_get_sprite(widget))
      editor_update(widget);
  }
}

void editors_draw_sprite(const Sprite *sprite, int x1, int y1, int x2, int y2)
{
  JWidget widget;
  JLink link;

  JI_LIST_FOR_EACH(editors, link) {
    widget = reinterpret_cast<JWidget>(link->data);

    if (sprite == editor_get_sprite(widget))
      editor_draw_sprite_safe(widget, x1, y1, x2, y2);
  }
}

/* TODO improve this (with JRegion or something, and without
   recursivity) */
void editors_draw_sprite_tiled(const Sprite *sprite, int x1, int y1, int x2, int y2)
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
  cx2 = cx1+sprite->w-1;
  cy2 = cy1+sprite->h-1;
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
			      MIN(lx1-1, x2-sprite->w), y2);
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
			      x2, MIN(ly1-1, y2-sprite->h));
#endif
  }
}

void editors_hide_sprite(const Sprite *sprite)
{
  UIContext* context = UIContext::instance();
  bool refresh = (context->get_current_sprite() == sprite) ? true: false;

  JLink link;
  JI_LIST_FOR_EACH(editors, link) {
    JWidget widget = reinterpret_cast<JWidget>(link->data);

    if (sprite == editor_get_sprite(widget))
      editor_set_sprite(widget, get_more_reliable_sprite());
  }

  if (refresh) {
    Sprite* sprite = editor_get_sprite(current_editor);

    context->set_current_sprite(sprite);
    app_refresh_screen(sprite);
  }
}

void set_current_editor(JWidget editor)
{
  if (current_editor != editor) {
    if (current_editor)
      jwidget_dirty(jwidget_get_view(current_editor));

    current_editor = editor;

    jwidget_dirty(jwidget_get_view(current_editor));

    UIContext* context = UIContext::instance();
    Sprite* sprite = editor_get_sprite(current_editor);
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

    editor_set_sprite(current_editor, sprite);

    jwidget_dirty(jwidget_get_view(current_editor));

    app_refresh_screen(sprite);
    app_realloc_sprite_list();
  }
}

void set_sprite_in_more_reliable_editor(Sprite* sprite)
{
  JWidget editor, best;
  JLink link;

  /* the current editor */
  best = current_editor;

  /* search for any empty editor */
  if (editor_get_sprite(best)) {
    JI_LIST_FOR_EACH(editors, link) {
      editor = reinterpret_cast<JWidget>(link->data);
      if (!editor_get_sprite(editor)) {
	best = editor;
	break;
      }
    }
  }

  set_current_editor(best);
  set_sprite_in_current_editor(sprite);
}

void split_editor(JWidget editor, int align)
{
  JWidget view = jwidget_get_view(editor);
  JWidget parent_box = jwidget_get_parent(view); /* box or panel */
  JWidget new_panel;
  JWidget new_view;
  JWidget new_editor;

  if (count_parents(editor) > 10) {
    jalert(_("Error<<You can't split the editor more||&Close"));
    return;
  }

  /* create a new box to contain both editors, and a new view to put
     the new editor */
  new_panel = jpanel_new(align);
  new_view = editor_view_new();
  new_editor = create_new_editor();

  /* insert the "new_box" in the same location that the view */
  jwidget_replace_child(parent_box, view, new_panel);

  /* append the new editor */
  jview_attach(new_view, new_editor);

  /* set the sprite for the new editor */
  editor_set_sprite(new_editor, editor_data(editor)->sprite);
  editor_data(new_editor)->zoom = editor_data(editor)->zoom;

  /* expansive widgets */
  jwidget_expansive(new_panel, TRUE);
  jwidget_expansive(new_view, TRUE);

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

    editor_data(new_editor)->offset_x = editor_data(editor)->offset_x;
    editor_data(new_editor)->offset_y = editor_data(editor)->offset_y;
  }

  /* fixup window */
  FIXUP_TOP_WINDOW();

  /* update both editors */
  editor_update(editor);
  editor_update(new_editor);
}

void close_editor(JWidget editor)
{
  JWidget view = jwidget_get_view(editor);
  JWidget parent_box = jwidget_get_parent(view); /* box or panel */
  JWidget other_widget;
  JLink link;

  /* you can't remove all editors */
  if (jlist_length(editors) == 1)
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
    if (next_editor)
      set_current_editor(next_editor);
  }

  /* fixup window */
  FIXUP_TOP_WINDOW();

  /* update all editors */
  JI_LIST_FOR_EACH(editors, link)
    editor_update(reinterpret_cast<JWidget>(link->data));
}

void make_unique_editor(JWidget editor)
{
  JWidget view = jwidget_get_view(editor);
  JLink link, next;
  JWidget child;

  /* it's the unique editor */
  if (jlist_length(editors) == 1)
    return;

  /* remove the editor-view of its parent */
  jwidget_remove_child(jwidget_get_parent(view), view);

  /* remove all children of main_editor_box */
  JI_LIST_FOR_EACH_SAFE(box_editors->children, link, next) {
    child = (JWidget)link->data;

    jwidget_remove_child(box_editors, child);
    jwidget_free(child);
  }

  /* append the editor to main box */
  jwidget_add_child(box_editors, view);

  /* new current editor */
  set_current_editor(editor);

  /* fixup window */
  FIXUP_TOP_WINDOW();

  /* update new editor */
  editor_update(editor);
}

static int is_sprite_in_some_editor(Sprite *sprite)
{
  JWidget widget;
  JLink link;

  JI_LIST_FOR_EACH(editors, link) {
    widget = reinterpret_cast<JWidget>(link->data);

    if (sprite == editor_get_sprite (widget))
      return TRUE;
  }

  return FALSE;
}

/**
 * Returns the next sprite that should be show if we close the current
 * one.
 */
static Sprite *get_more_reliable_sprite()
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
