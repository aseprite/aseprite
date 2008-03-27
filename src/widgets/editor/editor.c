/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

/* #define DRAWSPRITE_DOUBLEBUFFERED */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <allegro.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/brush.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/palette.h"
#include "raster/path.h"
#include "raster/quant.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/boundary.h"
#include "util/misc.h"
#include "util/recscr.h"
#include "util/render.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

#define has_shifts(msg,shift)			\
  (((msg)->any.shifts & (shift)) == (shift))

#define has_only_shifts(msg,shift)		\
  (((msg)->any.shifts & (KB_SHIFT_FLAG |	\
		         KB_ALT_FLAG |		\
		         KB_CTRL_FLAG)) == (shift))

/* editor's states */
enum {
  EDIT_STANDBY,
  EDIT_MOVING_SCROLL,
  EDIT_DRAWING,
};

static int use_dither = FALSE;

static bool editor_view_msg_proc(JWidget widget, JMessage msg);
static bool editor_msg_proc(JWidget widget, JMessage msg);
static void editor_request_size(JWidget widget, int *w, int *h);
static void editor_draw_sprite_boundary(JWidget widget);
static void editor_setcursor(JWidget widget, int x, int y);

JWidget editor_view_new(void)
{
  JWidget widget = jview_new();

  jwidget_set_border(widget, 3, 3, 3, 3);
  jview_without_bars(widget);
  jwidget_add_hook(widget, JI_WIDGET, editor_view_msg_proc, NULL);

  return widget;
}

JWidget editor_new(void)
{
  JWidget widget = jwidget_new(editor_type());
  Editor *editor = jnew0(Editor, 1);

  editor->widget = widget;
  editor->state = EDIT_STANDBY;
  editor->mask_timer_id = jmanager_add_timer(widget, 100);

  editor->cursor_thick = 0;
  editor->old_cursor_thick = 0;
  editor->cursor_candraw = FALSE;
  editor->alt_pressed = FALSE;
  editor->ctrl_pressed = FALSE;
  editor->space_pressed = FALSE;

  jwidget_add_hook(widget, editor_type(), editor_msg_proc, editor);
  jwidget_focusrest(widget, TRUE);

  return widget;
}

int editor_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

Editor *editor_data(JWidget widget)
{
  return jwidget_get_data(widget, editor_type());
}

Sprite *editor_get_sprite(JWidget widget)
{
  return editor_data(widget)->sprite;
}

void editor_set_sprite(JWidget widget, Sprite *sprite)
{
  Editor *editor = editor_data(widget);

  if (jwidget_has_mouse(widget))
    jmanager_free_mouse();

  editor->sprite = sprite;
  if (editor->sprite) {
    editor->zoom = editor->sprite->preferred.zoom;

    editor_update(widget);
    editor_set_scroll(widget,
		      editor->offset_x + editor->sprite->preferred.scroll_x,
		      editor->offset_y + editor->sprite->preferred.scroll_y,
		      FALSE);
  }
  else {
    editor_update(widget);
    editor_set_scroll(widget, 0, 0, FALSE);
  }

  jwidget_dirty(widget);
}

/* sets the scroll position of the editor */
void editor_set_scroll(JWidget widget, int x, int y, int use_refresh_region)
{
  Editor *editor = editor_data(widget);
  JWidget view = jwidget_get_view(widget);
  int old_scroll_x, old_scroll_y;
  JRegion region = NULL;
  int thick = editor->cursor_thick;

  if (thick) {
    editor_clean_cursor(widget);
  }

  if (use_refresh_region) {
    region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
    jview_get_scroll(view, &old_scroll_x, &old_scroll_y);
  }

  jview_set_scroll(view, x, y);

  if (editor->sprite) {
    editor->sprite->preferred.scroll_x = x - editor->offset_x;
    editor->sprite->preferred.scroll_y = y - editor->offset_y;
    editor->sprite->preferred.zoom = editor->zoom;
  }

  if (use_refresh_region) {
    int new_scroll_x, new_scroll_y;

    jview_get_scroll(view, &new_scroll_x, &new_scroll_y);

    /* move screen with blits */
    if (old_scroll_x != new_scroll_x || old_scroll_y != new_scroll_y) {
      int offset_x = old_scroll_x - new_scroll_x;
      int offset_y = old_scroll_y - new_scroll_y;
      JRegion reg2 = jregion_new(NULL, 0);
      int c, nrects;
      JRect rc;

      jregion_copy(reg2, region);
      jregion_translate(reg2, offset_x, offset_y);
      jregion_intersect(reg2, reg2, region);

      nrects = JI_REGION_NUM_RECTS(reg2);

      if (!thick)
	jmouse_hide();

      /* blit directly screen to screen *************************************/
      if (is_linear_bitmap(ji_screen) && nrects == 1) {
	rc = JI_REGION_RECTS(reg2);
	blit(ji_screen, ji_screen,
	     rc->x1-offset_x, rc->y1-offset_y,
	     rc->x1, rc->y1, jrect_w(rc), jrect_h(rc));
      }
      /* blit saving areas and copy them ************************************/
      else if (nrects > 1) {
	JList images = jlist_new();
	BITMAP *bmp;
	JLink link;

	for (c=0, rc=JI_REGION_RECTS(reg2);
	     c<nrects;
	     c++, rc++) {
	  bmp = create_bitmap(jrect_w(rc), jrect_h(rc));
	  blit(ji_screen, bmp,
	       rc->x1-offset_x, rc->y1-offset_y, 0, 0, bmp->w, bmp->h);
	  jlist_append(images, bmp);
	}

	for (c=0, rc=JI_REGION_RECTS(reg2), link=jlist_first(images);
	     c<nrects;
	     c++, rc++, link=link->next) {
	  bmp = link->data;
	  blit(bmp, ji_screen, 0, 0, rc->x1, rc->y1, bmp->w, bmp->h);
	  destroy_bitmap(bmp);
	}

	jlist_free(images);
      }
      /**********************************************************************/
      if (!thick)
	jmouse_show();

/*       if (editor->refresh_region) */
/* 	jregion_free(editor->refresh_region); */

/*       editor->refresh_region = jregion_new(NULL, 0); */
/*       jregion_copy(editor->refresh_region, region); */
/*       jregion_subtract(editor->refresh_region, editor->refresh_region, reg2); */
      /* TODO jwidget_validate */
      jregion_union(widget->update_region, widget->update_region, region);
      jregion_subtract(widget->update_region, widget->update_region, reg2);

      /* refresh the update_region */
      jwidget_flush_redraw(widget);
      jmanager_dispatch_messages(ji_get_default_manager());

      jregion_free(reg2);
    }

    jregion_free(region);
    /* editor->widget->flags &= ~JI_DIRTY; */
  }
/*   else { */
/*     if (editor->refresh_region) { */
/*       jregion_free (editor->refresh_region); */
/*       editor->refresh_region = NULL; */
/*     } */
/*   } */

  if (thick) {
    editor_draw_cursor(widget, editor->cursor_screen_x, editor->cursor_screen_y);
  }
}

void editor_update(JWidget widget)
{
  JWidget view = jwidget_get_view(widget);

  jview_update(view);
}

/**
 * Draws the sprite of the editor in the screen using
 * the @ref render_sprite routine.
 *
 * @warning You should setup the clip of the @ref ji_screen before to
 *          call this routine.
 */
void editor_draw_sprite(JWidget widget, int x1, int y1, int x2, int y2)
{
  Editor *editor = editor_data(widget);
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  int source_x, source_y, dest_x, dest_y, width, height;
  int scroll_x, scroll_y;

  /* get scroll */

  jview_get_scroll(view, &scroll_x, &scroll_y);

  /* output information */

  source_x = x1 << editor->zoom;
  source_y = y1 << editor->zoom;
  dest_x   = vp->x1 - scroll_x + editor->offset_x + source_x;
  dest_y   = vp->y1 - scroll_y + editor->offset_y + source_y;
  width    = (x2 - x1 + 1) << editor->zoom;
  height   = (y2 - y1 + 1) << editor->zoom;

  /* clip from viewport */

  if (dest_x < vp->x1) {
    source_x += vp->x1 - dest_x;
    width -= vp->x1 - dest_x;
    dest_x = vp->x1;
  }

  if (dest_y < vp->y1) {
    source_y += vp->y1 - dest_y;
    height -= vp->y1 - dest_y;
    dest_y = vp->y1;
  }

  if (dest_x+width-1 > vp->x2-1)
    width = vp->x2-dest_x;

  if (dest_y+height-1 > vp->y2-1)
    height = vp->y2-dest_y;

  /* clip from screen */

  if (dest_x < ji_screen->cl) {
    source_x += ji_screen->cl - dest_x;
    width -= ji_screen->cl - dest_x;
    dest_x = ji_screen->cl;
  }

  if (dest_y < ji_screen->ct) {
    source_y += ji_screen->ct - dest_y;
    height -= ji_screen->ct - dest_y;
    dest_y = ji_screen->ct;
  }

  if (dest_x+width-1 >= ji_screen->cr)
    width = ji_screen->cr-dest_x;

  if (dest_y+height-1 >= ji_screen->cb)
    height = ji_screen->cb-dest_y;

  /* clip from sprite */

  if (source_x < 0) {
    width += source_x;
    dest_x -= source_x;
    source_x = 0;
  }

  if (source_y < 0) {
    height += source_y;
    dest_y -= source_y;
    source_y = 0;
  }

  if (source_x+width > (editor->sprite->w << editor->zoom)) {
    width = (editor->sprite->w << editor->zoom) - source_x;
  }

  if (source_y+height > (editor->sprite->h << editor->zoom)) {
    height = (editor->sprite->h << editor->zoom) - source_y;
  }

  /* draw the sprite */

  if ((width > 0) && (height > 0)) {
    /* generate the rendered image */
    Image *rendered = render_sprite(editor->sprite,
				    source_x, source_y,
				    width, height,
				    editor->sprite->frame,
				    editor->zoom);

    /* dithering */
    if (use_dither &&
	rendered &&
	rendered->imgtype == IMAGE_RGB &&
	bitmap_color_depth(ji_screen) == 8) {
      Image *rgb_rendered = rendered;
      rendered = image_rgb_to_indexed(rgb_rendered, source_x, source_y,
				      orig_rgb_map,
				      get_current_palette());
      image_free(rgb_rendered);
    }

    if (rendered) {
#ifdef DRAWSPRITE_DOUBLEBUFFERED
      BITMAP *bmp = create_bitmap(width, height);

      use_current_sprite_rgb_map();
      image_to_allegro(rendered, bmp, 0, 0);
      blit(bmp, ji_screen, 0, 0, dest_x, dest_y, width, height);
      restore_rgb_map();

      image_free(rendered);
      destroy_bitmap(bmp);
#else

      acquire_bitmap(ji_screen);

      use_current_sprite_rgb_map();
      if (bitmap_color_depth(screen) == 8) {
	image_to_allegro(rendered, ji_screen, dest_x, dest_y);
      }
      else {
	PALETTE rgbpal;
	Palette *pal = sprite_get_palette(editor->sprite,
					  editor->sprite->frame);
	palette_to_allegro(pal, rgbpal);

	select_palette(rgbpal);
	image_to_allegro(rendered, ji_screen, dest_x, dest_y);
	unselect_palette();
      }
      restore_rgb_map();

      release_bitmap(ji_screen);

      image_free(rendered);
#endif
    }
  }

  jrect_free(vp);
}

/**
 * Draws the sprite taking care of the whole clipping region.
 *
 * For each rectangle calls @ref editor_draw_sprite.
 */
void editor_draw_sprite_safe(JWidget widget, int x1, int y1, int x2, int y2)
{
  JRegion region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
  int c, nrects = JI_REGION_NUM_RECTS(region);
  JRect rc;

  for (c=0, rc=JI_REGION_RECTS(region);
       c<nrects;
       c++, rc++) {
    set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
    editor_draw_sprite(widget, x1, y1, x2, y2);
  }
  set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);

  jregion_free(region);
}

/**
 * Draws the boundaries, really this routine doesn't use the "mask"
 * field of the sprite, only the "bound" field (so you can have other
 * mask in the sprite and could be showed other boundaries), to
 * regenerate boundaries, use the sprite_generate_mask_boundaries()
 * routine.
 */
void editor_draw_mask(JWidget widget)
{
  Editor *editor = editor_data(widget);
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;
  BoundSeg *seg;
  int c, x, y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  dotted_mode(editor->offset_count);

  x = vp->x1 - scroll_x + editor->offset_x;
  y = vp->y1 - scroll_y + editor->offset_y;

  for (c=0; c<editor->sprite->bound.nseg; c++) {
    seg = &editor->sprite->bound.seg[c];

    x1 = seg->x1<<editor->zoom;
    y1 = seg->y1<<editor->zoom;
    x2 = seg->x2<<editor->zoom;
    y2 = seg->y2<<editor->zoom;

#if 1				/* bounds inside mask */
    if (!seg->open)
#else				/* bounds outside mask */
    if (seg->open)
#endif
      {
	if (x1 == x2) {
	  x1--;
	  x2--;
	  y2--;
	}
	else {
	  y1--;
	  y2--;
	  x2--;
	}
      }
    else
      {
	if (x1 == x2) {
	  y2--;
	}
	else {
	  x2--;
	}
      }

    line(ji_screen, x+x1, y+y1, x+x2, y+y2, 0);
  }

  dotted_mode(-1);

  jrect_free(vp);
}

void editor_draw_mask_safe(JWidget widget)
{
  Editor *editor = editor_data(widget);

  if ((editor->sprite) && (editor->sprite->bound.seg)) {
    int thick = editor->cursor_thick;

    JRegion region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
    int c, nrects = JI_REGION_NUM_RECTS(region);
    JRect rc;

    acquire_bitmap(ji_screen);

    if (thick)
      editor_clean_cursor(widget);
    else
      jmouse_hide();

    for (c=0, rc=JI_REGION_RECTS(region);
	 c<nrects;
	 c++, rc++) {
      set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
      editor_draw_mask(widget);
    }
    set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
    jregion_free(region);

    /* draw the cursor */
    if (thick) {
      editor_draw_cursor(widget, editor->cursor_screen_x, editor->cursor_screen_y);
    }
    else
      jmouse_show();

    release_bitmap(ji_screen);
  }
}

void editor_draw_grid(JWidget widget)
{
  Editor *editor = editor_data(widget);
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  JRect grid = get_grid();
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int c;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  scroll_x = vp->x1 - scroll_x + editor->offset_x;
  scroll_y = vp->y1 - scroll_y + editor->offset_y;

  x1 = ji_screen->cl;
  y1 = ji_screen->ct;
  x2 = ji_screen->cr-1;
  y2 = ji_screen->cb-1;

  screen_to_editor(widget, x1, y1, &u1, &v1);
  screen_to_editor(widget, x2, y2, &u2, &v2);

  jrect_moveto(grid,
	       (grid->x1 % jrect_w(grid)) - jrect_w(grid),
	       (grid->y1 % jrect_h(grid)) - jrect_h(grid));

  u1 = ((u1-grid->x1) / jrect_w(grid)) - 1;
  v1 = ((v1-grid->y1) / jrect_h(grid)) - 1;
  u2 = ((u2-grid->x1) / jrect_w(grid)) + 1;
  v2 = ((v2-grid->y1) / jrect_h(grid)) + 1;

  grid->x1 <<= editor->zoom;
  grid->y1 <<= editor->zoom;
  grid->x2 <<= editor->zoom;
  grid->y2 <<= editor->zoom;

  x1 = scroll_x+grid->x1+u1*jrect_w(grid);
  x2 = scroll_x+grid->x1+u2*jrect_w(grid);

  for (c=v1; c<=v2; c++)
    hline(ji_screen, x1, scroll_y+grid->y1+c*jrect_h(grid), x2, makecol(0, 0, 255));

  y1 = scroll_y+grid->y1+v1*jrect_h(grid);
  y2 = scroll_y+grid->y1+v2*jrect_h(grid);

  for (c=u1; c<=u2; c++)
    vline(ji_screen, scroll_x+grid->x1+c*jrect_w(grid), y1, y2, makecol(0, 0, 255));

  jrect_free(grid);
  jrect_free(vp);
}

void editor_draw_grid_safe(JWidget widget)
{
  if (get_view_grid()) {
    JRegion region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
    int c, nrects = JI_REGION_NUM_RECTS(region);
    JRect rc;

    for (c=0, rc=JI_REGION_RECTS(region);
	 c<nrects;
	 c++, rc++) {
      set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
      editor_draw_grid(widget);
    }
    set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);

    jregion_free(region);
  }
}

/* void editor_draw_layer_boundary(JWidget widget) */
/* { */
/*   int x, y, x1, y1, x2, y2; */
/*   Sprite *sprite = editor_get_sprite(widget); */
/*   Image *image = GetImage2(sprite, &x, &y, NULL); */

/*   if (image) { */
/*     editor_to_screen(widget, x, y, &x1, &y1); */
/*     editor_to_screen(widget, x+image->w, y+image->h, &x2, &y2); */
/*     rectdotted(ji_screen, x1-1, y1-1, x2, y2, */
/* 	       makecol(255, 255, 0), makecol(0, 0, 255)); */
/*   } */
/* } */

/* void editor_draw_layer_boundary_safe(JWidget widget) */
/* { */
/*   JRegion region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS); */
/*   int c, nrects = JI_REGION_NUM_RECTS(region); */
/*   JRect rc; */

/*   for (c=0, rc=JI_REGION_RECTS(region); */
/*        c<nrects; */
/*        c++, rc++) { */
/*     set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1); */
/*     editor_draw_layer_boundary(widget); */
/*   } */
/*   set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1); */

/*   jregion_free(region); */
/* } */

/* void editor_update_layer_boundary(JWidget widget) */
/* { */
/*   int x, y, x1, y1, x2, y2; */
/*   Sprite *sprite = editor_get_sprite(widget); */
/*   Image *image = GetImage2(sprite, &x, &y, NULL); */
/*   Editor *editor = editor_data(widget); */

/*   if (editor->rect_data) { */
/*     rectrestore(editor->rect_data); */
/*     rectdiscard(editor->rect_data); */
/*     editor->rect_data = NULL; */
/*   } */

/*   if (image) { */
/*     editor_to_screen(widget, x, y, &x1, &y1); */
/*     editor_to_screen(widget, x+image->w, y+image->h, &x2, &y2); */

/*     editor->rect_data = rectsave(ji_screen, x1-1, y1-1, x2, y2); */
/*   } */
/* } */

void editor_draw_path(JWidget widget, int draw_extras)
{
#if 0
  Sprite *sprite = editor_get_sprite(widget);

  if (sprite->path) {
    GList *it;
    PathNode *node;
    int points[8], x1, y1, x2, y2;
    JWidget color_bar = app_get_color_bar();
    int splines_color = get_color_for_allegro(bitmap_color_depth(ji_screen),
					      color_bar_get_color(color_bar, 0));
    int extras_color = get_color_for_allegro(bitmap_color_depth(ji_screen),
					     color_bar_get_color(color_bar, 1));

    splines_color = makecol(255, 0, 0);
    extras_color = makecol(0, 0, 255);

    /* draw splines */
    for (it=sprite->path->nodes; it; it=it->next) {
      node = it->data;

      if (node->n) {
	editor_to_screen(widget, node->x, node->y, points+0, points+1);
	editor_to_screen(widget, node->nx, node->ny, points+2, points+3);
	editor_to_screen(widget, node->n->px, node->n->py, points+4, points+5);
	editor_to_screen(widget, node->n->x, node->n->y, points+6, points+7);

	spline(ji_screen, points, splines_color);
      }
    }

    /* draw extras */
    if (draw_extras) {
      /* draw control lines */
      for (it=sprite->path->nodes; it; it=it->next) {
	node = it->data;

	if (node->p) {
	  editor_to_screen(widget, node->px, node->py, &x1, &y1);
	  editor_to_screen(widget, node->x, node->y, &x2, &y2);

	  line(ji_screen, x1, y1, x2, y2, extras_color);
	}

	if (node->n) {
	  editor_to_screen(widget, node->x, node->y, &x1, &y1);
	  editor_to_screen(widget, node->nx, node->ny, &x2, &y2);

	  line(ji_screen, x1, y1, x2, y2, extras_color);
	}
      }

      /* draw the control points */
      for (it=sprite->path->nodes; it; it=it->next) {
	node = it->data;

	if (node->p) {
	  editor_to_screen(widget, node->px, node->py, &x1, &y1);
	  rectfill(ji_screen, x1-1, y1-1, x1, y1, extras_color);
	}

	if (node->n) {
	  editor_to_screen(widget, node->nx, node->ny, &x1, &y1);
	  rectfill(ji_screen, x1-1, y1-1, x1, y1, extras_color);
	}
      }

      /* draw the node */
      for (it=sprite->path->nodes; it; it=it->next) {
	node = it->data;

	editor_to_screen(widget, node->x, node->y, &x1, &y1);
	rectfill(ji_screen, x1-1, y1-1, x1+1, y1+1, extras_color);
      }
    }
  }
#endif
}

void editor_draw_path_safe(JWidget widget, int draw_extras)
{
  JRegion region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
  int c, nrects = JI_REGION_NUM_RECTS(region);
  JRect rc;

  for (c=0, rc=JI_REGION_RECTS(region);
       c<nrects;
       c++, rc++) {
    set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
    editor_draw_path(widget, draw_extras);
  }
  set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);

  jregion_free(region);
}

void screen_to_editor(JWidget widget, int xin, int yin, int *xout, int *yout)
{
  Editor *editor = editor_data(widget);
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;

  jview_get_scroll (view, &scroll_x, &scroll_y);

  *xout = (xin - vp->x1 + scroll_x - editor->offset_x) >> editor->zoom;
  *yout = (yin - vp->y1 + scroll_y - editor->offset_y) >> editor->zoom;

  jrect_free (vp);
}

void editor_to_screen(JWidget widget, int xin, int yin, int *xout, int *yout)
{
  Editor *editor = editor_data (widget);
  JWidget view = jwidget_get_view (widget);
  JRect vp = jview_get_viewport_position (view);
  int scroll_x, scroll_y;

  jview_get_scroll (view, &scroll_x, &scroll_y);

  *xout = (vp->x1 - scroll_x + editor->offset_x + (xin << editor->zoom));
  *yout = (vp->y1 - scroll_y + editor->offset_y + (yin << editor->zoom));

  jrect_free (vp);
}

void show_drawing_cursor(JWidget widget)
{
  Editor *editor = editor_data(widget);

  assert(editor->sprite != NULL);

  if (!editor->cursor_thick && editor->cursor_candraw) {
    jmouse_hide();
    editor_draw_cursor(widget, jmouse_x(0), jmouse_y(0));
    jmouse_show();
  }
}

void hide_drawing_cursor(JWidget widget)
{
  Editor *editor = editor_data(widget);

  if (editor->cursor_thick) {
    jmouse_hide();
    editor_clean_cursor(widget);
    jmouse_show();
  }
}

void editor_update_statusbar_for_standby(JWidget widget)
{
  Editor *editor = editor_data(widget);
  char buf[256];
  int x, y;

  screen_to_editor(widget, jmouse_x(0), jmouse_y(0), &x, &y);

  /* for eye-dropper */
  if (editor->alt_pressed) {
    color_t color = color_from_image(editor->sprite->imgtype,
				     sprite_getpixel(editor->sprite, x, y));
    if (color_type(color) != COLOR_TYPE_MASK) {
      usprintf(buf, "%s ", _("Color"));
      color_to_formalstring(editor->sprite->imgtype,
			    color,
			    buf+ustrlen(buf),
			    sizeof(buf)-ustrlen(buf), TRUE);
    }
    else {
      usprintf(buf, "%s", _("Transparent"));
    }
  }
  else {
    ustrcpy(buf, empty_string);
  }

  statusbar_set_text
    (app_get_statusbar(), 0,
     "%s %3d %3d (%s %3d %3d) [%s %d] %s",
     _("Pos"), x, y,
     _("Size"),
     ((editor->sprite->mask->bitmap)?
      editor->sprite->mask->w:
      editor->sprite->w),
     ((editor->sprite->mask->bitmap)?
      editor->sprite->mask->h:
      editor->sprite->h),
     _("Frame"), editor->sprite->frame+1,
     buf);
}

void editor_refresh_region(JWidget widget)
{
  /*   if (editor->refresh_region) { */
  if (widget->update_region) {
    Editor *editor = editor_data(widget);
    int thick = editor->cursor_thick;

    if (thick)
      editor_clean_cursor(widget);
    else
      jmouse_hide();

    /* TODO check if this work!!!! */
/*     jwidget_redraw_region */
/*       (jwindow_get_manager (jwidget_get_window (widget)), */
/*        editor->refresh_region); */
/*     jwidget_redraw_region (widget, editor->refresh_region); */
    jwidget_flush_redraw(widget);
    jmanager_dispatch_messages(ji_get_default_manager());

/*     if (editor->refresh_region) { */
/*       jregion_free (editor->refresh_region); */
/*       editor->refresh_region = NULL; */
/*     } */

    if (thick) {
      editor_draw_cursor(widget, editor->cursor_screen_x, editor->cursor_screen_y);
    }
    else
      jmouse_show();
  }
}

/**********************************************************************/
/* message handler for the a view widget that contains an editor */

static bool editor_view_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      /* this avoid the displacement of the widgets in the viewport */

      jrect_copy(widget->rc, &msg->setpos.rect);
      jview_update(widget);
      return TRUE;

    case JM_DRAW:
      {
	JWidget viewport = jview_get_viewport(widget);
	JWidget child = jlist_first_data(viewport->children);
	JRect pos = jwidget_get_rect(widget);
	int has_focus;

	if (child == current_editor)
	  has_focus = TRUE;
	else
	  has_focus = FALSE;

	if (has_focus) {
	  /* 1st border */
	  jdraw_rect(pos, ji_color_selected());

	  /* 2nd border */
	  jrect_shrink(pos, 1);
	  jdraw_rect(pos, ji_color_selected());

	  /* 3rd border */
	  jrect_shrink(pos, 1);
	  jdraw_rectedge(pos, makecol(128, 128, 128), makecol(255, 255, 255));
	}
	else {
	  /* 1st border */
	  jdraw_rect(pos, makecol(192, 192, 192));

	  /* 2nd border */
	  jrect_shrink(pos, 1);
	  jdraw_rect(pos, makecol(192, 192, 192));

	  /* 3rd border */
	  jrect_shrink(pos, 1);
	  jdraw_rectedge(pos, makecol(128, 128, 128), makecol(255, 255, 255));
	}

	jrect_free(pos);
      }
      return TRUE;

  }
  return FALSE;
}

/**********************************************************************/
/* message handler for the editor */

static bool editor_msg_proc(JWidget widget, JMessage msg)
{
  Editor *editor = editor_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jmanager_remove_timer(editor->mask_timer_id);
      remove_editor(widget);
      jfree(editor_data(widget));
      break;

    case JM_REQSIZE:
      editor_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_CLOSE:
      if (editor->refresh_region)
	jregion_free(editor->refresh_region);
      break;

    case JM_DRAW: {
      if (editor->old_cursor_thick == 0) {
	editor->old_cursor_thick = editor->cursor_thick;
      }

      if (editor->cursor_thick != 0)
	editor_clean_cursor(widget);

      /* without sprite */
      if (!editor->sprite) {
	JWidget view = jwidget_get_view(widget);
	JRect vp = jview_get_viewport_position(view);
	
	jdraw_rectfill(vp, makecol(128, 128, 128));
	draw_emptyset_symbol(vp, makecol(64, 64, 64));

	jrect_free(vp);
      }
      /* draw the sprite */
      else {
	int x1, y1, x2, y2;

	use_dither = get_config_bool("Options", "Dither", use_dither);
	
	/* draw the background outside of image */
	x1 = widget->rc->x1 + editor->offset_x;
	y1 = widget->rc->y1 + editor->offset_y;
	x2 = x1 + (editor->sprite->w << editor->zoom) - 1;
	y2 = y1 + (editor->sprite->h << editor->zoom) - 1;

	jrectexclude(ji_screen,
		     widget->rc->x1, widget->rc->y1,
		     widget->rc->x2-1, widget->rc->y2-1,
		     x1, y1, x2, y2, makecol(128, 128, 128));

	/* draw the sprite in the editor */
	editor_draw_sprite(widget, 0, 0,
			   editor->sprite->w-1, editor->sprite->h-1);

	/* draw the sprite boundary */
	editor_draw_sprite_boundary(widget);

	if (editor->rect_data) {
	  /* destroy the layer-bound information */
	  rectdiscard(editor->rect_data);
	  editor->rect_data = NULL;
	}

	/* draw the grid */
	if (get_view_grid())
	  editor_draw_grid(widget);

	/* draw path */
	editor_draw_path(widget, FALSE/* , */
			  /* (current_editor == widget) && */
/* 			  (current_tool == &ase_tool_path) */);

	/* draw the mask */
	if (editor->sprite->bound.seg) {
	  editor_draw_mask(widget);
	  jmanager_start_timer(editor->mask_timer_id);
	}
	else {
	  jmanager_stop_timer(editor->mask_timer_id);
	}

	if (msg->draw.count == 0
	    && editor->old_cursor_thick != 0) {
	  editor_draw_cursor(widget, jmouse_x(0), jmouse_y(0));
	}
      }

      if (msg->draw.count == 0)
	editor->old_cursor_thick = 0;

      return TRUE;
    }

    case JM_TIMER:
      if (msg->timer.timer_id == editor->mask_timer_id) {
	if (editor->sprite) {
	  editor_draw_mask_safe(widget);

	  /* set offset to make selection-movement effect */
	  if (editor->offset_count < 7)
	    editor->offset_count++;
	  else
	    editor->offset_count = 0;
	}
      }
      break;

    case JM_MOUSEENTER:
      /* when the mouse enter to the editor, we can calculate the
	 'cursor_candraw' field to avoid a heavy if-condition in the
	 'editor_setcursor' routine */
      editor->cursor_candraw =
	(editor->sprite != NULL &&
	 !sprite_is_locked(editor->sprite) &&
	 editor->sprite->layer != NULL &&
	 layer_is_image(editor->sprite->layer) &&
	 layer_is_readable(editor->sprite->layer) &&
	 layer_is_writable(editor->sprite->layer) &&
	 layer_get_cel(editor->sprite->layer,
		       editor->sprite->frame) != NULL);

      if (msg->any.shifts & KB_ALT_FLAG) editor->alt_pressed = TRUE;
      if (msg->any.shifts & KB_CTRL_FLAG) editor->ctrl_pressed = TRUE;
      /* TODO another way to get the KEY_SPACE state? */
      if (key[KEY_SPACE]) editor->space_pressed = TRUE;
      break;

    case JM_MOUSELEAVE:
      hide_drawing_cursor(widget);

      if (editor->alt_pressed) editor->alt_pressed = FALSE;
      if (editor->ctrl_pressed) editor->ctrl_pressed = FALSE;
      if (editor->space_pressed) editor->space_pressed = FALSE;
      break;

    case JM_BUTTONPRESSED: {
      set_current_editor(widget);
      set_current_sprite(editor->sprite);

      if (!editor->sprite)
	break;

      /* move the scroll */
      if ((msg->mouse.middle && has_only_shifts(msg, 0)) ||
	  (msg->mouse.left && editor->space_pressed)) {
	editor->state = EDIT_MOVING_SCROLL;

	editor_setcursor(widget, msg->mouse.x, msg->mouse.y);
      }
      /* move frames position */
      else if (editor->ctrl_pressed) {
	if ((editor->sprite->layer) &&
	    (editor->sprite->layer->gfxobj.type == GFXOBJ_LAYER_IMAGE)) {
	  /* TODO you can move the `Background' with tiled mode */
	  if (layer_is_background(editor->sprite->layer)) {
	    jalert(_(PACKAGE
		     "<<You can't move the `Background' layer."
		     "||&Close"));
	  }
	  else if (!layer_is_moveable(editor->sprite->layer)) {
	    jalert(_(PACKAGE
		     "<<The layer movement is locked."
		     "||&Close"));
	  }
	  else {
	    bool click2 = get_config_bool("Options", "MoveClick2", FALSE);
	    interactive_move_layer(click2 ? MODE_CLICKANDCLICK:
					    MODE_CLICKANDRELEASE,
				   TRUE, NULL);
	  }
	  return TRUE;
	}
      }
      /* call the eyedropper command */
      else if (editor->alt_pressed) {
	Command *command = command_get_by_name(CMD_EYEDROPPER_TOOL);
	if (command_is_enabled(command, NULL))
	  command_execute(command, msg->mouse.right ? "background": NULL);
	return TRUE;
      }
      /* draw */
      else if (current_tool) {
	editor->state = EDIT_DRAWING;
      }

      /* set capture */
      jwidget_hard_capture_mouse(widget);
    }

    case JM_MOTION: {
      if (!editor->sprite)
	break;

      /* move the scroll */
      if (editor->state == EDIT_MOVING_SCROLL) {
	JWidget view = jwidget_get_view(widget);
	JRect vp = jview_get_viewport_position(view);
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	editor_set_scroll(widget,
			  scroll_x+jmouse_x(1)-jmouse_x(0),
			  scroll_y+jmouse_y(1)-jmouse_y(0), TRUE);

	jmouse_control_infinite_scroll(vp);
	jrect_free(vp);
      }
      /* draw */
      else if (editor->state == EDIT_DRAWING) {
	JWidget colorbar = app_get_colorbar();
	color_t fg = colorbar_get_fg_color(colorbar);
	color_t bg = colorbar_get_bg_color(colorbar);

	/* call the tool-control routine */
	control_tool(widget, current_tool,
		     msg->mouse.left ? fg: bg,
		     msg->mouse.left ? bg: fg,
		     msg->mouse.left);

	editor->state = EDIT_STANDBY;
	jwidget_release_mouse(widget);
      }
      else {
	/* Draw cursor */
	if (editor->cursor_thick) {
	  int x, y;

	  /* jmouse_set_cursor(JI_CURSOR_NULL); */

	  x = msg->mouse.x;
	  y = msg->mouse.y;

	  /* Redraw it only when the mouse change to other pixel (not
	     when the mouse moves only).  */
	  if ((editor->cursor_screen_x != x) || (editor->cursor_screen_y != y)) {
	    jmouse_hide();
	    editor_clean_cursor(widget);
	    editor_draw_cursor(widget, x, y);
	    jmouse_show();
	  }
	}

	/* status bar text */
	if (editor->state == EDIT_STANDBY) {
	  editor_update_statusbar_for_standby(widget);
	}
	else if (editor->state == EDIT_MOVING_SCROLL) {
	  int x, y;
	  screen_to_editor(widget, jmouse_x(0), jmouse_y(0), &x, &y);
	  statusbar_set_text
	    (app_get_statusbar(), 0,
	     "Pos %3d %3d (Size %3d %3d)", x, y,
	     editor->sprite->w, editor->sprite->h);
	}
      }
      return TRUE;
    }

    case JM_BUTTONRELEASED:
      if (!editor->sprite)
	break;

      editor->state = EDIT_STANDBY;
      editor_setcursor(widget, msg->mouse.x, msg->mouse.y);

      jwidget_release_mouse(widget);
      return TRUE;

    case JM_KEYPRESSED:
      if (editor_keys_toset_zoom(widget, msg->key.scancode) ||
	  editor_keys_toset_brushsize(widget, msg->key.scancode))
	return TRUE;

      if (jwidget_has_mouse(widget)) {
	switch (msg->key.scancode) {
	  
	  /* eye-dropper is activated with ALT key */
	  case KEY_ALT:
	    editor->alt_pressed = TRUE;
	    editor_setcursor(widget, jmouse_x(0), jmouse_y(0));
	    return TRUE;

	  case KEY_LCONTROL:
	  case KEY_RCONTROL:
	    editor->ctrl_pressed = TRUE;
	    editor_setcursor(widget, jmouse_x(0), jmouse_y(0));
	    return TRUE;

	  case KEY_SPACE:
	    editor->space_pressed = TRUE;
	    editor_setcursor(widget, jmouse_x(0), jmouse_y(0));
	    return TRUE;
	}
      }
      break;

    case JM_KEYRELEASED:
      switch (msg->key.scancode) {

	/* eye-dropper is deactivated with ALT key */
	case KEY_ALT:
	  if (editor->alt_pressed) {
	    editor->alt_pressed = FALSE;
	    editor_setcursor(widget, jmouse_x(0), jmouse_y(0));
	    return TRUE;
	  }
	  break;

	  case KEY_LCONTROL:
	  case KEY_RCONTROL:
	    if (editor->ctrl_pressed) {
	      editor->ctrl_pressed = FALSE;
	      editor_setcursor(widget, jmouse_x(0), jmouse_y(0));
	      return TRUE;
	    }
	    break;

	case KEY_SPACE:
	  if (editor->space_pressed) {
	    /* we have to clear all the KEY_SPACE in buffer */
	    clear_keybuf();

	    editor->space_pressed = FALSE;
	    editor_setcursor(widget, jmouse_x(0), jmouse_y(0));
	    return TRUE;
	  }
	  break;
      }
      break;

    case JM_WHEEL:
      if (editor->state == EDIT_STANDBY) {
	/* there are and sprite in the editor, there is the mouse inside*/
	if (editor->sprite &&
	    jwidget_has_mouse(widget)) {
	  int dz = jmouse_z(1) - jmouse_z(0);

	  /* with the ALT */
	  if (has_only_shifts(msg, KB_ALT_FLAG)) {
	    JWidget view = jwidget_get_view(widget);
	    JRect vp = jview_get_viewport_position(view);
	    int x, y, zoom;

	    x = 0;
	    y = 0;
	    zoom = MID(MIN_ZOOM, editor->zoom-dz, MAX_ZOOM);

	    /* zoom */
	    if (editor->zoom != zoom) {
	      /* TODO: este pedazo de código es igual que el de la
		 rutina: editor_keys_toset_zoom, tengo que intentar
		 unir ambos, alguna función como:
		 editor_set_zoom_and_center_in_mouse */
	      screen_to_editor(widget, jmouse_x(0), jmouse_y(0), &x, &y);

	      x = editor->offset_x - jrect_w(vp)/2 + ((1<<zoom)>>1) + (x << zoom);
	      y = editor->offset_y - jrect_h(vp)/2 + ((1<<zoom)>>1) + (y << zoom);

	      if ((editor->cursor_editor_x != (vp->x1+vp->x2)/2) ||
		  (editor->cursor_editor_y != (vp->y1+vp->y2)/2)) {
		int use_refresh_region = (editor->zoom == zoom) ? TRUE: FALSE;

		editor->zoom = zoom;

		editor_update(widget);
		editor_set_scroll(widget, x, y, use_refresh_region);

		jmouse_set_position((vp->x1+vp->x2)/2, (vp->y1+vp->y2)/2);
	      }
	    }

	    jrect_free(vp);
	  }
	  else {
	    JWidget view = jwidget_get_view(widget);
	    JRect vp = jview_get_viewport_position(view);
	    int scroll_x, scroll_y;
	    int dx = 0;
	    int dy = 0;
	    int thick = editor->cursor_thick;

	    if (has_shifts(msg, KB_CTRL_FLAG))
	      dx = dz * jrect_w(vp);
	    else
	      dy = dz * jrect_h(vp);

	    if (has_shifts(msg, KB_SHIFT_FLAG)) {
	      dx /= 2;
	      dy /= 2;
	    }
	    else {
	      dx /= 10;
	      dy /= 10;
	    }
		
	    jview_get_scroll(view, &scroll_x, &scroll_y);

	    jmouse_hide();
	    if (thick)
	      editor_clean_cursor(widget);
	    editor_set_scroll(widget, scroll_x+dx, scroll_y+dy, TRUE);
	    if (thick)
	      editor_draw_cursor(widget, jmouse_x(0), jmouse_y(0));
	    jmouse_show();

	    jrect_free(vp);
	  }
	}
      }
      break;

    case JM_SETCURSOR:
      editor_setcursor(widget, msg->mouse.x, msg->mouse.y);
      return TRUE;

  }

  return FALSE;
}

/**
 * Returns size for the editor viewport
 */
static void editor_request_size(JWidget widget, int *w, int *h)
{
  Editor *editor = editor_data(widget);

  if (editor->sprite) {
    JWidget view = jwidget_get_view(widget);
    JRect vp = jview_get_viewport_position(view);

    editor->offset_x = jrect_w(vp) - 1;
    editor->offset_y = jrect_h(vp) - 1;

    *w = (editor->sprite->w << editor->zoom) + editor->offset_x*2;
    *h = (editor->sprite->h << editor->zoom) + editor->offset_y*2;

    jrect_free (vp);
  }
  else {
    *w = 4;
    *h = 4;
  }
}

static void editor_draw_sprite_boundary(JWidget widget)
{
  Sprite *sprite = editor_get_sprite(widget);
  int x1, y1, x2, y2;

  assert(sprite != NULL);

  editor_to_screen(widget, 0, 0, &x1, &y1);
  editor_to_screen(widget, sprite->w, sprite->h, &x2, &y2);
  rect(ji_screen, x1-1, y1-1, x2, y2, makecol(0, 0, 0));
}

static void editor_setcursor(JWidget widget, int x, int y)
{
  Editor *editor = editor_data(widget);

  switch (editor->state) {

    case EDIT_MOVING_SCROLL:
      hide_drawing_cursor(widget);
      jmouse_set_cursor(JI_CURSOR_SCROLL);
      break;

    case EDIT_DRAWING:
      jmouse_set_cursor(JI_CURSOR_NULL);
      show_drawing_cursor(widget);
      break;

    case EDIT_STANDBY:
      if (editor->sprite) {
	/* eyedropper */
	if (editor->alt_pressed) {
	  hide_drawing_cursor(widget);
	  jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	}
	/* move layer */
	else if (editor->ctrl_pressed) {
	  hide_drawing_cursor(widget);
	  jmouse_set_cursor(JI_CURSOR_MOVE);
	}
	/* scroll */
	else if (editor->space_pressed) {
	  hide_drawing_cursor(widget);
	  jmouse_set_cursor(JI_CURSOR_SCROLL);
	}
	/* draw */
	else if (editor->cursor_candraw) {
	  jmouse_set_cursor(JI_CURSOR_NULL);
	  show_drawing_cursor(widget);
	}
	/* forbidden */
	else {
	  hide_drawing_cursor(widget);
	  jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
	}
      }
      else {
	hide_drawing_cursor(widget);
	jmouse_set_cursor(JI_CURSOR_NORMAL);
      }
      break;

  }
}
