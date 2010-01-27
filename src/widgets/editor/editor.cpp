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

/* #define DRAWSPRITE_DOUBLEBUFFERED */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <allegro.h>

#include "jinete/jinete.h"

#include "ui_context.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/skinneable_theme.h"
#include "modules/palettes.h"
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

static bool use_dither = false;

static bool editor_view_msg_proc(JWidget widget, JMessage msg);

JWidget editor_view_new()
{
  JWidget widget = jview_new();
  SkinneableTheme* theme = static_cast<SkinneableTheme*>(widget->theme);
  int l = theme->get_part(PART_EDITOR_SELECTED_W)->w;
  int t = theme->get_part(PART_EDITOR_SELECTED_N)->h;
  int r = theme->get_part(PART_EDITOR_SELECTED_E)->w;
  int b = theme->get_part(PART_EDITOR_SELECTED_S)->h;

  jwidget_set_border(widget, l, t, r, b);
  jview_without_bars(widget);
  jwidget_add_hook(widget, JI_WIDGET, editor_view_msg_proc, NULL);

  return widget;
}

Editor::Editor()
  : Widget(editor_type())
{
  m_state = EDIT_STANDBY;
  m_sprite = NULL;
  m_zoom = 0;

  m_cursor_thick = 0;
  m_cursor_screen_x = 0;
  m_cursor_screen_y = 0;
  m_cursor_editor_x = 0;
  m_cursor_editor_y = 0;
  m_old_cursor_thick = 0;

  m_cursor_candraw = false;
  m_alt_pressed = false;
  m_ctrl_pressed = false;
  m_space_pressed = false;

  m_offset_x = 0;
  m_offset_y = 0;
  m_mask_timer_id = jmanager_add_timer(this, 100);
  m_offset_count = 0;

  m_refresh_region = NULL;

  jwidget_focusrest(this, true);
}

Editor::~Editor()
{
  jmanager_remove_timer(m_mask_timer_id);
  remove_editor(this);
}

int editor_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

void Editor::editor_set_sprite(Sprite* sprite)
{
  if (jwidget_has_mouse(this))
    jmanager_free_mouse();

  m_sprite = sprite;
  if (m_sprite) {
    m_zoom = m_sprite->preferred.zoom;

    editor_update();
    editor_set_scroll(m_offset_x + m_sprite->preferred.scroll_x,
		      m_offset_y + m_sprite->preferred.scroll_y,
		      false);
  }
  else {
    editor_update();
    editor_set_scroll(0, 0, false);
  }

  dirty();
}

/* sets the scroll position of the editor */
void Editor::editor_set_scroll(int x, int y, int use_refresh_region)
{
  JWidget view = jwidget_get_view(this);
  int old_scroll_x, old_scroll_y;
  JRegion region = NULL;
  int thick = m_cursor_thick;

  if (thick)
    editor_clean_cursor();

  if (use_refresh_region) {
    region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
    jview_get_scroll(view, &old_scroll_x, &old_scroll_y);
  }

  jview_set_scroll(view, x, y);

  if (m_sprite) {
    m_sprite->preferred.scroll_x = x - m_offset_x;
    m_sprite->preferred.scroll_y = y - m_offset_y;
    m_sprite->preferred.zoom = m_zoom;
  }

  if (use_refresh_region) {
    int new_scroll_x, new_scroll_y;

    jview_get_scroll(view, &new_scroll_x, &new_scroll_y);

    /* move screen with blits */
    jwidget_scroll(this, region,
		   old_scroll_x - new_scroll_x,
		   old_scroll_y - new_scroll_y);
    
    jregion_free(region);
    /* m_widget->flags &= ~JI_DIRTY; */
  }
/*   else { */
/*     if (m_refresh_region) { */
/*       jregion_free (m_refresh_region); */
/*       m_refresh_region = NULL; */
/*     } */
/*   } */

  if (thick)
    editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);
}

void Editor::editor_update()
{
  JWidget view = jwidget_get_view(this);
  jview_update(view);
}

/**
 * Draws the sprite of the editor in the screen using
 * the @ref render_sprite routine.
 *
 * @warning You should setup the clip of the @ref ji_screen before to
 *          call this routine.
 */
void Editor::editor_draw_sprite(int x1, int y1, int x2, int y2)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int source_x, source_y, dest_x, dest_y, width, height;
  int scroll_x, scroll_y;

  /* get scroll */

  jview_get_scroll(view, &scroll_x, &scroll_y);

  /* output information */

  source_x = x1 << m_zoom;
  source_y = y1 << m_zoom;
  dest_x   = vp->x1 - scroll_x + m_offset_x + source_x;
  dest_y   = vp->y1 - scroll_y + m_offset_y + source_y;
  width    = (x2 - x1 + 1) << m_zoom;
  height   = (y2 - y1 + 1) << m_zoom;

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

  if (source_x+width > (m_sprite->w << m_zoom)) {
    width = (m_sprite->w << m_zoom) - source_x;
  }

  if (source_y+height > (m_sprite->h << m_zoom)) {
    height = (m_sprite->h << m_zoom) - source_y;
  }

  /* draw the sprite */

  if ((width > 0) && (height > 0)) {
    /* generate the rendered image */
    Image *rendered = render_sprite(m_sprite,
				    source_x, source_y,
				    width, height,
				    m_sprite->frame,
				    m_zoom);

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

      CurrentSpriteRgbMap rgbmap;
      if (bitmap_color_depth(screen) == 8) {
	image_to_allegro(rendered, ji_screen, dest_x, dest_y);
      }
      else {
	PALETTE rgbpal;
	Palette *pal = sprite_get_palette(m_sprite,
					  m_sprite->frame);
	palette_to_allegro(pal, rgbpal);

	select_palette(rgbpal);
	image_to_allegro(rendered, ji_screen, dest_x, dest_y);
	unselect_palette();
      }

      release_bitmap(ji_screen);

      image_free(rendered);
#endif
    }
  }

  jrect_free(vp);

  // Draw the grid
  if (get_view_grid())
    this->drawGrid();
}

/**
 * Draws the sprite taking care of the whole clipping region.
 *
 * For each rectangle calls @ref editor_draw_sprite.
 */
void Editor::editor_draw_sprite_safe(int x1, int y1, int x2, int y2)
{
  JRegion region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
  int c, nrects = JI_REGION_NUM_RECTS(region);
  int cx1, cy1, cx2, cy2;
  JRect rc;

  get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

  for (c=0, rc=JI_REGION_RECTS(region);
       c<nrects;
       c++, rc++) {
    add_clip_rect(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
    editor_draw_sprite(x1, y1, x2, y2);
    set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);
  }

  jregion_free(region);
}

/**
 * Draws the boundaries, really this routine doesn't use the "mask"
 * field of the sprite, only the "bound" field (so you can have other
 * mask in the sprite and could be showed other boundaries), to
 * regenerate boundaries, use the sprite_generate_mask_boundaries()
 * routine.
 */
void Editor::editor_draw_mask()
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;
  BoundSeg *seg;
  int c, x, y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  dotted_mode(m_offset_count);

  x = vp->x1 - scroll_x + m_offset_x;
  y = vp->y1 - scroll_y + m_offset_y;

  for (c=0; c<m_sprite->bound.nseg; c++) {
    seg = &m_sprite->bound.seg[c];

    x1 = seg->x1<<m_zoom;
    y1 = seg->y1<<m_zoom;
    x2 = seg->x2<<m_zoom;
    y2 = seg->y2<<m_zoom;

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

void Editor::editor_draw_mask_safe()
{
  if ((m_sprite) && (m_sprite->bound.seg)) {
    int thick = m_cursor_thick;

    JRegion region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
    int c, nrects = JI_REGION_NUM_RECTS(region);
    JRect rc;

    acquire_bitmap(ji_screen);

    if (thick)
      editor_clean_cursor();
    else
      jmouse_hide();

    for (c=0, rc=JI_REGION_RECTS(region);
	 c<nrects;
	 c++, rc++) {
      set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
      editor_draw_mask();
    }
    set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
    jregion_free(region);

    /* draw the cursor */
    if (thick) {
      editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);
    }
    else
      jmouse_show();

    release_bitmap(ji_screen);
  }
}

void Editor::drawGrid()
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  JRect grid = get_grid();
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int c;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  scroll_x = vp->x1 - scroll_x + m_offset_x;
  scroll_y = vp->y1 - scroll_y + m_offset_y;

  x1 = ji_screen->cl;
  y1 = ji_screen->ct;
  x2 = ji_screen->cr-1;
  y2 = ji_screen->cb-1;

  screen_to_editor(x1, y1, &u1, &v1);
  screen_to_editor(x2, y2, &u2, &v2);

  jrect_moveto(grid,
	       (grid->x1 % jrect_w(grid)) - jrect_w(grid),
	       (grid->y1 % jrect_h(grid)) - jrect_h(grid));

  u1 = ((u1-grid->x1) / jrect_w(grid)) - 1;
  v1 = ((v1-grid->y1) / jrect_h(grid)) - 1;
  u2 = ((u2-grid->x1) / jrect_w(grid)) + 1;
  v2 = ((v2-grid->y1) / jrect_h(grid)) + 1;

  grid->x1 <<= m_zoom;
  grid->y1 <<= m_zoom;
  grid->x2 <<= m_zoom;
  grid->y2 <<= m_zoom;

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

void Editor::screen_to_editor(int xin, int yin, int *xout, int *yout)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  *xout = (xin - vp->x1 + scroll_x - m_offset_x) >> m_zoom;
  *yout = (yin - vp->y1 + scroll_y - m_offset_y) >> m_zoom;

  jrect_free(vp);
}

void Editor::editor_to_screen(int xin, int yin, int *xout, int *yout)
{
  JWidget view = jwidget_get_view(this);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  *xout = (vp->x1 - scroll_x + m_offset_x + (xin << m_zoom));
  *yout = (vp->y1 - scroll_y + m_offset_y + (yin << m_zoom));

  jrect_free(vp);
}

void Editor::show_drawing_cursor()
{
  assert(m_sprite != NULL);

  if (!m_cursor_thick && m_cursor_candraw) {
    jmouse_hide();
    editor_draw_cursor(jmouse_x(0), jmouse_y(0));
    jmouse_show();
  }
}

void Editor::hide_drawing_cursor()
{
  if (m_cursor_thick) {
    jmouse_hide();
    editor_clean_cursor();
    jmouse_show();
  }
}

void Editor::editor_update_statusbar_for_standby()
{
  char buf[256];
  int x, y;

  screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);

  /* for eye-dropper */
  if (m_alt_pressed) {
    color_t color = color_from_image(m_sprite->imgtype,
				     sprite_getpixel(m_sprite, x, y));
    if (color_type(color) != COLOR_TYPE_MASK) {
      usprintf(buf, "%s ", _("Color"));
      color_to_formalstring(m_sprite->imgtype,
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
     ((m_sprite->mask->bitmap)?
      m_sprite->mask->w:
      m_sprite->w),
     ((m_sprite->mask->bitmap)?
      m_sprite->mask->h:
      m_sprite->h),
     _("Frame"), m_sprite->frame+1,
     buf);
}

void Editor::editor_refresh_region()
{
  if (this->update_region) {
    int thick = m_cursor_thick;

    if (thick)
      editor_clean_cursor();
    else
      jmouse_hide();

    /* TODO check if this work!!!! */
/*     jwidget_redraw_region */
/*       (jwindow_get_manager (jwidget_get_window (widget)), */
/*        m_refresh_region); */
/*     jwidget_redraw_region (widget, m_refresh_region); */
    jwidget_flush_redraw(this);
    jmanager_dispatch_messages(ji_get_default_manager());

/*     if (m_refresh_region) { */
/*       jregion_free (m_refresh_region); */
/*       m_refresh_region = NULL; */
/*     } */

    if (thick) {
      editor_draw_cursor(m_cursor_screen_x, m_cursor_screen_y);
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
	JWidget child = reinterpret_cast<JWidget>(jlist_first_data(viewport->children));
	JRect pos = jwidget_get_rect(widget);
	SkinneableTheme* theme = static_cast<SkinneableTheme*>(widget->theme);

	theme->draw_bounds(pos->x1, pos->y1,
			   pos->x2-1, pos->y2-1,
			   (child == current_editor) ? PART_EDITOR_SELECTED_NW:
						       PART_EDITOR_NORMAL_NW, false);

	jrect_free(pos);
      }
      return TRUE;

  }
  return FALSE;
}

/**********************************************************************/
/* message handler for the editor */

static int click_last_x = 0;
static int click_last_y = 0;

bool Editor::msg_proc(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      editor_request_size(&msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_CLOSE:
      // if (m_refresh_region)
      // 	jregion_free(m_refresh_region);
      break;

    case JM_DRAW: {
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);

      if (m_old_cursor_thick == 0) {
	m_old_cursor_thick = m_cursor_thick;
      }

      if (m_cursor_thick)
	editor_clean_cursor();

      /* without sprite */
      if (!m_sprite) {
	JWidget view = jwidget_get_view(this);
	JRect vp = jview_get_viewport_position(view);

	jdraw_rectfill(vp, theme->get_editor_face_color());
	draw_emptyset_symbol(vp, makecol(64, 64, 64));

	jrect_free(vp);
      }
      /* draw the sprite */
      else {
	int x1, y1, x2, y2;

	use_dither = get_config_bool("Options", "Dither", use_dither);
	
	/* draw the background outside of image */
	x1 = this->rc->x1 + m_offset_x;
	y1 = this->rc->y1 + m_offset_y;
	x2 = x1 + (m_sprite->w << m_zoom) - 1;
	y2 = y1 + (m_sprite->h << m_zoom) - 1;

	jrectexclude(ji_screen,
		     this->rc->x1, this->rc->y1,
		     this->rc->x2-1, this->rc->y2-1,
		     x1-1, y1-1, x2+1, y2+2, theme->get_editor_face_color());

	/* draw the sprite in the editor */
	editor_draw_sprite(0, 0, m_sprite->w-1, m_sprite->h-1);

	/* draw the sprite boundary */
	rect(ji_screen, x1-1, y1-1, x2+1, y2+1, theme->get_editor_sprite_border());
	hline(ji_screen, x1-1, y2+2, x2+1, theme->get_editor_sprite_bottom_edge());

	/* draw the mask */
	if (m_sprite->bound.seg) {
	  editor_draw_mask();
	  jmanager_start_timer(m_mask_timer_id);
	}
	else {
	  jmanager_stop_timer(m_mask_timer_id);
	}

	if (msg->draw.count == 0
	    && m_old_cursor_thick != 0) {
	  editor_draw_cursor(jmouse_x(0), jmouse_y(0));
	}
      }

      if (msg->draw.count == 0)
	m_old_cursor_thick = 0;

      return TRUE;
    }

    case JM_TIMER:
      if (msg->timer.timer_id == m_mask_timer_id) {
	if (m_sprite) {
	  editor_draw_mask_safe();

	  /* set offset to make selection-movement effect */
	  if (m_offset_count < 7)
	    m_offset_count++;
	  else
	    m_offset_count = 0;
	}
      }
      break;

    case JM_MOUSEENTER:
      /* when the mouse enter to the editor, we can calculate the
	 'cursor_candraw' field to avoid a heavy if-condition in the
	 'editor_setcursor' routine */
      editor_update_candraw();

      if (msg->any.shifts & KB_ALT_FLAG) m_alt_pressed = true;
      if (msg->any.shifts & KB_CTRL_FLAG) m_ctrl_pressed = true;
      if (key[KEY_SPACE]) m_space_pressed = true;
      break;

    case JM_MOUSELEAVE:
      hide_drawing_cursor();

      if (m_alt_pressed) m_alt_pressed = false;
      if (m_ctrl_pressed) m_ctrl_pressed = false;
      if (m_space_pressed) m_space_pressed = false;
      break;

    case JM_BUTTONPRESSED: {
      UIContext* context = UIContext::instance();

      set_current_editor(this);
      context->set_current_sprite(m_sprite);

      if (!m_sprite)
	break;

      /* move the scroll */
      if (msg->mouse.middle || m_space_pressed) {
	m_state = EDIT_MOVING_SCROLL;

	editor_setcursor(msg->mouse.x, msg->mouse.y);
      }
      /* move frames position */
      else if (m_ctrl_pressed) {
	if ((m_sprite->layer) &&
	    (m_sprite->layer->type == GFXOBJ_LAYER_IMAGE)) {
	  /* TODO you can move the `Background' with tiled mode */
	  if (m_sprite->layer->is_background()) {
	    jalert(_(PACKAGE
		     "<<You can't move the `Background' layer."
		     "||&Close"));
	  }
	  else if (!m_sprite->layer->is_moveable()) {
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
      else if (m_alt_pressed) {
	Command* eyedropper_cmd = 
	  CommandsModule::instance()->get_command_by_name(CommandId::eyedropper);

	Params params;
	params.set("target", msg->mouse.right ? "background": "foreground");

	UIContext::instance()->execute_command(eyedropper_cmd, &params);
	return true;
      }
      /* draw */
      else if (current_tool && m_sprite->layer) {
	m_state = EDIT_DRAWING;
      }

      /* set capture */
      jwidget_hard_capture_mouse(this);
    }

    case JM_MOTION: {
      if (!m_sprite)
	break;

      /* move the scroll */
      if (m_state == EDIT_MOVING_SCROLL) {
	JWidget view = jwidget_get_view(this);
	JRect vp = jview_get_viewport_position(view);
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	editor_set_scroll(scroll_x+jmouse_x(1)-jmouse_x(0),
			  scroll_y+jmouse_y(1)-jmouse_y(0), TRUE);

	jmouse_control_infinite_scroll(vp);
	jrect_free(vp);
      }
      /* draw */
      else if (m_state == EDIT_DRAWING) {
	JWidget colorbar = app_get_colorbar();
	color_t fg = colorbar_get_fg_color(colorbar);
	color_t bg = colorbar_get_bg_color(colorbar);

	/* call the tool-control routine */
	control_tool(this, current_tool,
		     msg->mouse.left ? fg: bg,
		     msg->mouse.left ? bg: fg,
		     msg->mouse.left);

	m_state = EDIT_STANDBY;
	jwidget_release_mouse(this);
      }
      else {
	/* Draw cursor */
	if (m_cursor_thick) {
	  int x, y;

	  /* jmouse_set_cursor(JI_CURSOR_NULL); */

	  x = msg->mouse.x;
	  y = msg->mouse.y;

	  /* Redraw it only when the mouse change to other pixel (not
	     when the mouse moves only).  */
	  if ((m_cursor_screen_x != x) || (m_cursor_screen_y != y)) {
	    jmouse_hide();
	    editor_clean_cursor();
	    editor_draw_cursor(x, y);
	    jmouse_show();
	  }
	}

	/* status bar text */
	if (m_state == EDIT_STANDBY) {
	  editor_update_statusbar_for_standby();
	}
	else if (m_state == EDIT_MOVING_SCROLL) {
	  int x, y;
	  screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);
	  statusbar_set_text
	    (app_get_statusbar(), 0,
	     "Pos %3d %3d (Size %3d %3d)", x, y,
	     m_sprite->w, m_sprite->h);
	}
      }
      return TRUE;
    }

    case JM_BUTTONRELEASED:
      if (!m_sprite)
	break;

      m_state = EDIT_STANDBY;
      editor_setcursor(msg->mouse.x, msg->mouse.y);

      jwidget_release_mouse(this);
      return TRUE;

    case JM_KEYPRESSED:
      if (editor_keys_toset_zoom(msg->key.scancode) ||
	  editor_keys_toset_brushsize(msg->key.scancode))
	return TRUE;

      if (jwidget_has_mouse(this)) {
	switch (msg->key.scancode) {
	  
	  /* eye-dropper is activated with ALT key */
	  case KEY_ALT:
	    m_alt_pressed = TRUE;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return TRUE;

	  case KEY_LCONTROL:
	  case KEY_RCONTROL:
	    m_ctrl_pressed = TRUE;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return TRUE;

	  case KEY_SPACE:
	    m_space_pressed = TRUE;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return TRUE;
	}
      }
      break;

    case JM_KEYRELEASED:
      switch (msg->key.scancode) {

	/* eye-dropper is deactivated with ALT key */
	case KEY_ALT:
	  if (m_alt_pressed) {
	    m_alt_pressed = FALSE;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return TRUE;
	  }
	  break;

	  case KEY_LCONTROL:
	  case KEY_RCONTROL:
	    if (m_ctrl_pressed) {
	      m_ctrl_pressed = FALSE;
	      editor_setcursor(jmouse_x(0), jmouse_y(0));
	      return TRUE;
	    }
	    break;

	case KEY_SPACE:
	  if (m_space_pressed) {
	    /* we have to clear all the KEY_SPACE in buffer */
	    clear_keybuf();

	    m_space_pressed = FALSE;
	    editor_setcursor(jmouse_x(0), jmouse_y(0));
	    return TRUE;
	  }
	  break;
      }
      break;

    case JM_WHEEL:
      if (m_state == EDIT_STANDBY) {
	/* there are and sprite in the editor, there is the mouse inside*/
	if (m_sprite &&
	    jwidget_has_mouse(this)) {
	  int dz = jmouse_z(1) - jmouse_z(0);

	  /* with the ALT */
	  if (!(msg->any.shifts & (KB_SHIFT_FLAG |
				   KB_ALT_FLAG |
				   KB_CTRL_FLAG))) {
	    JWidget view = jwidget_get_view(this);
	    JRect vp = jview_get_viewport_position(view);
	    int x, y, zoom;

	    x = 0;
	    y = 0;
	    zoom = MID(MIN_ZOOM, m_zoom-dz, MAX_ZOOM);

	    /* zoom */
	    if (m_zoom != zoom) {
	      /* TODO: este pedazo de código es igual que el de la
		 rutina: editor_keys_toset_zoom, tengo que intentar
		 unir ambos, alguna función como:
		 editor_set_zoom_and_center_in_mouse */
	      screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);

	      x = m_offset_x - jrect_w(vp)/2 + ((1<<zoom)>>1) + (x << zoom);
	      y = m_offset_y - jrect_h(vp)/2 + ((1<<zoom)>>1) + (y << zoom);

	      if ((m_cursor_editor_x != (vp->x1+vp->x2)/2) ||
		  (m_cursor_editor_y != (vp->y1+vp->y2)/2)) {
		int use_refresh_region = (m_zoom == zoom) ? TRUE: FALSE;

		m_zoom = zoom;

		editor_update();
		editor_set_scroll(x, y, use_refresh_region);

		jmouse_set_position((vp->x1+vp->x2)/2, (vp->y1+vp->y2)/2);
	      }
	    }

	    jrect_free(vp);
	  }
	  else {
	    JWidget view = jwidget_get_view(this);
	    JRect vp = jview_get_viewport_position(view);
	    int scroll_x, scroll_y;
	    int dx = 0;
	    int dy = 0;
	    int thick = m_cursor_thick;

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
	      editor_clean_cursor();
	    editor_set_scroll(scroll_x+dx, scroll_y+dy, TRUE);
	    if (thick)
	      editor_draw_cursor(jmouse_x(0), jmouse_y(0));
	    jmouse_show();

	    jrect_free(vp);
	  }
	}
      }
      break;

    case JM_SETCURSOR:
      editor_setcursor(msg->mouse.x, msg->mouse.y);
      return TRUE;

  }

  return Widget::msg_proc(msg);
}

/**
 * Returns size for the editor viewport
 */
void Editor::editor_request_size(int *w, int *h)
{
  if (m_sprite) {
    JWidget view = jwidget_get_view(this);
    JRect vp = jview_get_viewport_position(view);

    m_offset_x = jrect_w(vp) - 1;
    m_offset_y = jrect_h(vp) - 1;

    *w = (m_sprite->w << m_zoom) + m_offset_x*2;
    *h = (m_sprite->h << m_zoom) + m_offset_y*2;

    jrect_free(vp);
  }
  else {
    *w = 4;
    *h = 4;
  }
}

void Editor::editor_setcursor(int x, int y)
{
  switch (m_state) {

    case EDIT_MOVING_SCROLL:
      hide_drawing_cursor();
      jmouse_set_cursor(JI_CURSOR_SCROLL);
      break;

    case EDIT_DRAWING:
      jmouse_set_cursor(JI_CURSOR_NULL);
      show_drawing_cursor();
      break;

    case EDIT_STANDBY:
      if (m_sprite) {
	editor_update_candraw(); /* TODO remove this */

	/* eyedropper */
	if (m_alt_pressed) {
	  hide_drawing_cursor();
	  jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	}
	/* move layer */
	else if (m_ctrl_pressed) {
	  hide_drawing_cursor();
	  jmouse_set_cursor(JI_CURSOR_MOVE);
	}
	/* scroll */
	else if (m_space_pressed) {
	  hide_drawing_cursor();
	  jmouse_set_cursor(JI_CURSOR_SCROLL);
	}
	/* draw */
	else if (m_cursor_candraw) {
	  jmouse_set_cursor(JI_CURSOR_NULL);
	  show_drawing_cursor();
	}
	/* forbidden */
	else {
	  hide_drawing_cursor();
	  jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
	}
      }
      else {
	hide_drawing_cursor();
	jmouse_set_cursor(JI_CURSOR_NORMAL);
      }
      break;

  }
}

/* TODO this routine should be called in a change of context */
void Editor::editor_update_candraw()
{
  m_cursor_candraw =
    (m_sprite != NULL &&
     m_sprite->layer != NULL &&
     m_sprite->layer->is_image() &&
     m_sprite->layer->is_readable() &&
     m_sprite->layer->is_writable() /* && */
     /* layer_get_cel(m_sprite->layer, m_sprite->frame) != NULL */
     );
}
