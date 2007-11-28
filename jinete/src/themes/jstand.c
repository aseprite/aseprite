/* Jinete - a GUI library
 * Copyright (c) 2003, 2004, 2005, 2007, David A. Capello
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the Jinete nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete.h"
#include "jinete/intern.h"

/* data related */
#include "stdicons.c"
#include "pcx2data.c"

#define CHARACTER_LENGTH(f, c)	((f)->vtable->char_length ((f), (c)))

#define BGCOLOR			(get_bg_color(widget))

#define COLOR_FOREGROUND	makecol(0, 0, 0)
#define COLOR_DISABLED		makecol(128, 128, 128)
#define COLOR_FACE		makecol(210, 200, 190)
#define COLOR_HOTFACE		makecol(250, 240, 230)
#define COLOR_SELECTED		makecol(44, 76, 145)
#define COLOR_BACKGROUND	makecol(255, 255, 255)

/* "icons_data" indexes */
enum {
  FIRST_CURSOR = 0,
  LAST_CURSOR = 11,
  ICON_CHECK_EDGE = 12,
  ICON_CHECK_MARK,
  ICON_CLOSE,
  ICON_MENU_MARK,
  ICON_RADIO_EDGE,
  ICON_RADIO_MARK,
  ICONS,
};

static struct {
  bool mask : 1;
  unsigned char *data;
} icons_data[ICONS] = {
  { FALSE, default_theme_cnormal },
  { FALSE, default_theme_cnoradd },
  { FALSE, default_theme_chand },
  { FALSE, default_theme_cmove },
  { FALSE, default_theme_csizetl },
  { FALSE, default_theme_csizet },
  { FALSE, default_theme_csizetr },
  { FALSE, default_theme_csizel },
  { FALSE, default_theme_csizer },
  { FALSE, default_theme_csizebl },
  { FALSE, default_theme_csizeb },
  { FALSE, default_theme_csizebr },
  { FALSE, default_theme_ichecke },
  { FALSE, default_theme_icheckm },
  { FALSE, default_theme_iclose },
  {  TRUE, default_theme_imenum },
  { FALSE, default_theme_iradioe },
  { FALSE, default_theme_iradiom },
};

static BITMAP *icons_bitmap[ICONS] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static void theme_destroy(void);
static void theme_regen(void);
static BITMAP *theme_set_cursor(int type, int *focus_x, int *focus_y);
static void theme_init_widget(JWidget widget);
static JRegion theme_get_window_mask(JWidget widget);
static void theme_map_decorative_widget(JWidget widget);
static int theme_color_foreground(void);
static int theme_color_disabled(void);
static int theme_color_face(void);
static int theme_color_hotface(void);
static int theme_color_selected(void);
static int theme_color_background(void);
static void theme_draw_box(JWidget widget);
static void theme_draw_button(JWidget widget);
static void theme_draw_check(JWidget widget);
static void theme_draw_entry(JWidget widget);
static void theme_draw_label(JWidget widget);
static void theme_draw_listbox(JWidget widget);
static void theme_draw_listitem(JWidget widget);
static void theme_draw_menu(JWidget widget);
static void theme_draw_menuitem(JWidget widget);
static void theme_draw_panel(JWidget widget);
static void theme_draw_radio(JWidget widget);
static void theme_draw_separator(JWidget widget);
static void theme_draw_slider(JWidget widget);
static void theme_draw_textbox(JWidget widget);
static void theme_draw_view(JWidget widget);
static void theme_draw_view_scrollbar(JWidget widget);
static void theme_draw_view_viewport(JWidget widget);
static void theme_draw_window(JWidget widget);

static int get_bg_color(JWidget widget);
static void draw_textstring(const char *t, int fg_color, int bg_color,
			    bool fill_bg, JWidget widget, const JRect rect,
			    int selected_offset);
static void draw_entry_cursor(JWidget widget, int x, int y);
static void draw_icons(int x, int y, JWidget widget, int edge_icon);
static void draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel);
static void less_bevel(int *bevel);

static bool theme_button_msg_proc (JWidget widget, JMessage msg);

JTheme jtheme_new_standard (void)
{
  JTheme theme;

  theme = jtheme_new ();
  if (!theme)
    return NULL;

  theme->name = "Standard Theme";
  theme->check_icon_size = 8;
  theme->radio_icon_size = 8;
  theme->scrollbar_size = 12;

  theme->destroy = theme_destroy;
  theme->regen = theme_regen;
  theme->set_cursor = theme_set_cursor;
  theme->init_widget = theme_init_widget;
  theme->get_window_mask = theme_get_window_mask;
  theme->map_decorative_widget = theme_map_decorative_widget;
  theme->color_foreground = theme_color_foreground;
  theme->color_disabled = theme_color_disabled;
  theme->color_face = theme_color_face;
  theme->color_hotface = theme_color_hotface;
  theme->color_selected = theme_color_selected;
  theme->color_background = theme_color_background;

  jtheme_set_method(theme, JI_BOX, theme_draw_box);
  jtheme_set_method(theme, JI_BUTTON, theme_draw_button);
  jtheme_set_method(theme, JI_CHECK, theme_draw_check);
  jtheme_set_method(theme, JI_ENTRY, theme_draw_entry);
  jtheme_set_method(theme, JI_LABEL, theme_draw_label);
  jtheme_set_method(theme, JI_LISTBOX, theme_draw_listbox);
  jtheme_set_method(theme, JI_LISTITEM, theme_draw_listitem);
  jtheme_set_method(theme, JI_MENU, theme_draw_menu);
  jtheme_set_method(theme, JI_MENUITEM, theme_draw_menuitem);
  jtheme_set_method(theme, JI_PANEL, theme_draw_panel);
  jtheme_set_method(theme, JI_RADIO, theme_draw_radio);
  jtheme_set_method(theme, JI_SEPARATOR, theme_draw_separator);
  jtheme_set_method(theme, JI_SLIDER, theme_draw_slider);
  jtheme_set_method(theme, JI_TEXTBOX, theme_draw_textbox);
  jtheme_set_method(theme, JI_VIEW, theme_draw_view);
  jtheme_set_method(theme, JI_VIEW_SCROLLBAR, theme_draw_view_scrollbar);
  jtheme_set_method(theme, JI_VIEW_VIEWPORT, theme_draw_view_viewport);
  jtheme_set_method(theme, JI_WINDOW, theme_draw_window);

  return theme;
}

static void theme_destroy (void)
{
  int c;

  for (c=0; c<ICONS; c++) {
    if (icons_bitmap[c]) {
      destroy_bitmap (icons_bitmap[c]);
      icons_bitmap[c] = NULL;
    }
  }
}

static void theme_regen (void)
{
  JTheme theme = ji_get_theme ();
  int c, cmap[8], mask_cmap[2];

  theme->desktop_color = COLOR_DISABLED;
  theme->textbox_fg_color = COLOR_FOREGROUND;
  theme->textbox_bg_color = COLOR_BACKGROUND;

  /* fixup cursors */

  cmap[0] = bitmap_mask_color(ji_screen);
  cmap[1] = COLOR_FOREGROUND;
  cmap[2] = COLOR_DISABLED;
  cmap[3] = COLOR_BACKGROUND;
  cmap[4] = COLOR_FACE;
  cmap[5] = COLOR_HOTFACE;

  mask_cmap[0] = 0;
  mask_cmap[1] = 1;

  for (c=0; c<ICONS; c++) {
    if (icons_bitmap[c])
      destroy_bitmap (icons_bitmap[c]);

    if (icons_data[c].mask)
      icons_bitmap[c] = data2bmp(8, icons_data[c].data, mask_cmap);
    else
      icons_bitmap[c] = data2bmp(bitmap_color_depth (ji_screen),
				 icons_data[c].data, cmap);
  }
}

static BITMAP *theme_set_cursor(int type, int *focus_x, int *focus_y)
{
  BITMAP *sprite = NULL;
  int icon_index = type-1+FIRST_CURSOR;

  *focus_x = 0;
  *focus_y = 0;

  if (icon_index >= FIRST_CURSOR && icon_index <= LAST_CURSOR) {
    if (icons_bitmap[icon_index])
      sprite = icons_bitmap[icon_index];

    switch (type) {
      case JI_CURSOR_NULL:
      case JI_CURSOR_NORMAL:
      case JI_CURSOR_NORMAL_ADD:
	*focus_x = 0;
	*focus_y = 0;
	break;
      case JI_CURSOR_HAND:
	*focus_x = 5;
	*focus_y = 3;
	break;
      case JI_CURSOR_MOVE:
	*focus_x = 8;
	*focus_y = 8;
	break;
      case JI_CURSOR_SIZE_TL:
      case JI_CURSOR_SIZE_T:
      case JI_CURSOR_SIZE_TR:
      case JI_CURSOR_SIZE_L:
      case JI_CURSOR_SIZE_R:
      case JI_CURSOR_SIZE_BL:
      case JI_CURSOR_SIZE_B:
      case JI_CURSOR_SIZE_BR:
	*focus_x = 8;
	*focus_y = 8;
	break;
    }
  }

  return sprite;
}

static void theme_init_widget(JWidget widget)
{
#define BORDER(n)				\
  widget->border_width.l = n;			\
  widget->border_width.t = n;			\
  widget->border_width.r = n;			\
  widget->border_width.b = n;

#define BORDER4(L,T,R,B)			\
  widget->border_width.l = L;			\
  widget->border_width.t = T;			\
  widget->border_width.r = R;			\
  widget->border_width.b = B;

  if ((widget->flags & JI_INITIALIZED) &&
      (widget->type != JI_WINDOW))
    return;

  switch (widget->draw_type) {

    case JI_BOX:
      BORDER(0);
/*       widget->child_spacing = 2; */
      widget->child_spacing = 4;
      break;

    case JI_BUTTON:
      BORDER(4);
      widget->child_spacing = 0;
      break;

    case JI_CHECK:
      BORDER(2);
/*       widget->child_spacing = 2; */
      widget->child_spacing = 4;
      break;

    case JI_ENTRY:
      BORDER(3);
      break;

    case JI_LABEL:
      BORDER(1);
      break;

    case JI_LISTBOX:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case JI_LISTITEM:
      BORDER(1);
      break;

    case JI_MENU:
    case JI_MENUBAR:
    case JI_MENUBOX:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case JI_MENUITEM:
      BORDER(2);
      widget->child_spacing = 18;
      break;

    case JI_PANEL:
      BORDER(0);
      widget->child_spacing = 3;
      break;

    case JI_RADIO:
      BORDER(2);
/*       widget->child_spacing = 2; */
      widget->child_spacing = 4;
      break;

    case JI_SEPARATOR:
      /* frame */
      if ((widget->align & JI_HORIZONTAL) &&
	  (widget->align & JI_VERTICAL)) {
	BORDER (4);
      }
      /* horizontal bar */
      else if (widget->align & JI_HORIZONTAL) {
	BORDER4 (2, 4, 2, 0);
      }
      /* vertical bar */
      else {
	BORDER4 (4, 2, 0, 2);
      }

      if (widget->text) {
	if (widget->align & JI_TOP)
	  widget->border_width.t = jwidget_get_text_height (widget);
	else if (widget->align & JI_BOTTOM)
	  widget->border_width.b = jwidget_get_text_height (widget);
      }
      break;

    case JI_SLIDER:
      BORDER(4);
      widget->child_spacing = jwidget_get_text_height (widget);
      break;

    case JI_TEXTBOX:
      BORDER(2);
      widget->child_spacing = 0;
      break;

    case JI_VIEW:
      BORDER(2);
      widget->child_spacing = 0;
      break;

    case JI_VIEW_SCROLLBAR:
      BORDER(1);
      widget->child_spacing = 0;
      break;

    case JI_VIEW_VIEWPORT:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case JI_WINDOW:
      if (!jwindow_is_desktop (widget)) {
	if (widget->text) {
	  BORDER4(6, 4+jwidget_get_text_height(widget)+6, 6, 6);
#if 1				/* add close button */
	  if (!(widget->flags & JI_INITIALIZED)) {
	    JWidget button = jbutton_new("x");
	    jbutton_set_bevel(button, 0, 0, 0, 0);
	    jwidget_add_hook(button, JI_WIDGET,
			     theme_button_msg_proc, NULL);
	    jwidget_decorative(button, TRUE);
	    jwidget_add_child(widget, button);
	    jwidget_set_name(button, "theme_close_button");
	  }
#endif
	}
	else if (!(widget->flags & JI_INITIALIZED)) {
	  BORDER(3);
	}
      }
      else {
	BORDER (0);
      }
      widget->child_spacing = 4;
      break;

    default:
      break;
  }
}

static JRegion theme_get_window_mask (JWidget widget)
{
  return jregion_new (widget->rc, 1);
}

static void theme_map_decorative_widget (JWidget widget)
{
  if (ustrcmp(widget->name, "theme_close_button") == 0) {
    JWidget window = widget->parent;
    JRect rect = jrect_new(0, 0, 0, 0);

    rect->x2 = jwidget_get_text_height(widget) + 2;
    rect->y2 = jwidget_get_text_height(widget) + 2;

    jrect_displace(rect,
		     window->rc->x2 - 4 - jrect_w(rect),
		     window->rc->y1 + 4);

    jwidget_set_rect(widget, rect);
    jrect_free(rect);
  }
}

static int theme_color_foreground(void)
{
  return COLOR_FOREGROUND;
}

static int theme_color_disabled(void)
{
  return COLOR_DISABLED;
}

static int theme_color_face(void)
{
  return COLOR_FACE;
}

static int theme_color_hotface(void)
{
  return COLOR_HOTFACE;
}

static int theme_color_selected(void)
{
  return COLOR_SELECTED;
}

static int theme_color_background(void)
{
  return COLOR_BACKGROUND;
}

static void theme_draw_box(JWidget widget)
{
  jdraw_rectfill (widget->rc, BGCOLOR);
}

static void theme_draw_button(JWidget widget)
{
  BITMAP *icon_bmp = ji_generic_button_get_icon(widget);
  int icon_align = ji_generic_button_get_icon_align(widget);
  struct jrect box, text, icon;
  int x1, y1, x2, y2;
  int fg, bg, c1, c2;
  int bevel[4];
  JRect crect;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			      icon_align,
			      icon_bmp ? icon_bmp->w : 0,
			      icon_bmp ? icon_bmp->h : 0);

  /* with mouse */
  if (jwidget_is_enabled(widget) && jwidget_has_mouse(widget))
    bg = COLOR_HOTFACE;
  /* without mouse */
  else
    bg = COLOR_FACE;

  /* selected */
  if (jwidget_is_selected(widget)) {
    fg = COLOR_BACKGROUND;
    bg = COLOR_SELECTED;

    c1 = ji_color_faceshadow();
    c2 = ji_color_facelight();
  }
  /* non-selected */
  else {
    fg = COLOR_FOREGROUND;

    c1 = ji_color_facelight();
    c2 = ji_color_faceshadow();
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* extern background */
  rectfill(ji_screen, x1, y1, x2, y2, BGCOLOR);

  /* get bevel info */
  jbutton_get_bevel(widget, bevel);

  /* 1st border */
  if (jwidget_has_focus(widget))
    draw_bevel_box(x1, y1, x2, y2,
		   COLOR_FOREGROUND, COLOR_FOREGROUND, bevel);
  else {
    less_bevel(bevel);
    draw_bevel_box (x1, y1, x2, y2, c1, c2, bevel);
  }

  less_bevel(bevel);

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (jwidget_has_focus(widget))
    draw_bevel_box(x1, y1, x2, y2, c1, c2, bevel);
  else
    draw_bevel_box(x1, y1, x2, y2, bg, bg, bevel);

  less_bevel(bevel);

  /* more borders */
  while ((bevel[0] > 0) || (bevel[1] > 0) ||
	 (bevel[2] > 0) || (bevel[3] > 0)) {
    x1++, y1++, x2--, y2--;
    draw_bevel_box(x1, y1, x2, y2, bg, bg, bevel);
    less_bevel(bevel);
  }

  /* background */
  x1++, y1++, x2--, y2--;
  rectfill(ji_screen, x1, y1, x2, y2, bg);

  /* text */
  crect = jwidget_get_child_rect(widget);
  draw_textstring(NULL, fg, bg, FALSE, widget, crect, 1);
  jrect_free(crect);

  /* icon */
  if (icon_bmp) {
    if (jwidget_is_selected(widget))
      jrect_displace(&icon, 1, 1);

    /* enabled */
    if (jwidget_is_enabled(widget)) {
      /* selected */
      if (jwidget_is_selected(widget)) {
	register int c, mask = bitmap_mask_color(icon_bmp);
	int x, y;

	for (y=0; y<icon_bmp->h; ++y) {
	  for (x=0; x<icon_bmp->w; ++x) {
	    c = getpixel(icon_bmp, x, y);
	    if (c != mask)
	      putpixel(ji_screen, icon.x1+x, icon.y1+y,
		       makecol(255-getr(c),
			       255-getg(c),
			       255-getb(c)));
	  }
	}
      }
      /* non-selected */
      else {
	draw_sprite(ji_screen, icon_bmp, icon.x1, icon.y1);
      }
    }
    /* disabled */
    else {
      _ji_theme_draw_sprite_color(ji_screen, icon_bmp, icon.x1+1, icon.y1+1,
				  COLOR_BACKGROUND);
      _ji_theme_draw_sprite_color(ji_screen, icon_bmp, icon.x1, icon.y1,
				  COLOR_DISABLED);
    }
  }
}

static void theme_draw_check(JWidget widget)
{
  struct jrect box, text, icon;
  int bg;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			      ji_generic_button_get_icon_align(widget),
			      widget->theme->check_icon_size,
			      widget->theme->check_icon_size);

  /* background */
  jdraw_rectfill (widget->rc, bg = BGCOLOR);

  /* mouse */
  if (jwidget_is_enabled(widget) && jwidget_has_mouse(widget))
    jdraw_rectfill (&box, bg = COLOR_HOTFACE);

  /* focus */
  if (jwidget_has_focus(widget)) {
    jrect_stretch(&box, 1);
    jdraw_rect(&box, COLOR_FOREGROUND);
  }

  /* text */
  draw_textstring (NULL, -1, bg, FALSE, widget, &text, 0);

  /* icon */
  draw_icons (icon.x1, icon.y1, widget, ICON_CHECK_EDGE);
}

static void theme_draw_entry(JWidget widget)
{
  bool password = jentry_is_password (widget);
  int scroll, cursor, state, selbeg, selend;
  const char *text = widget->text;
  int c, ch, x, y, w, fg, bg;
  int x1, y1, x2, y2;
  int cursor_x;

  jtheme_entry_info (widget, &scroll, &cursor, &state, &selbeg, &selend);

  /* main pos */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2 - 1;
  y2 = widget->rc->y2 - 1;

  bg = COLOR_BACKGROUND;

  /* 1st border */
  _ji_theme_rectedge(ji_screen, x1, y1, x2, y2,
		     COLOR_DISABLED, COLOR_BACKGROUND);

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (jwidget_has_focus (widget))
    rect (ji_screen, x1, y1, x2, y2, COLOR_FOREGROUND);
  else
    rect (ji_screen, x1, y1, x2, y2, BGCOLOR);

  /* background border */
  x1++, y1++, x2--, y2--;
  rectfill (ji_screen, x1, y1, x2, y2, bg);

  /* draw the text */
  x = widget->rc->x1 + widget->border_width.l;
  y = (widget->rc->y1+widget->rc->y2)/2 - jwidget_get_text_height (widget)/2;

  for (c=scroll; ugetat (text, c); c++) {
    ch = password ? '*': ugetat (text, c);

    /* normal text */
    bg = -1;
    fg = COLOR_FOREGROUND;

    /* selected */
    if ((c >= selbeg) && (c <= selend)) {
      if (jwidget_has_focus(widget))
	bg = COLOR_SELECTED;
      else
	bg = COLOR_DISABLED;
      fg = COLOR_BACKGROUND;
    }

    /* disabled */
    if (jwidget_is_disabled (widget)) {
      bg = -1;
      fg = COLOR_DISABLED;
    }

    w = CHARACTER_LENGTH (widget->text_font, ch);
    if (x+w > widget->rc->x2-3)
      return;

    cursor_x = x;
    ji_font_set_aa_mode (widget->text_font,
			 bg >= 0 ? bg: COLOR_BACKGROUND);
    widget->text_font->vtable->render_char (widget->text_font,
					    ch, fg, bg, ji_screen, x, y);
    x += w;

    /* cursor */
    if ((c == cursor) && (state) && (jwidget_has_focus (widget)))
      draw_entry_cursor (widget, cursor_x, y);
  }

  /* draw the cursor if it is next of the last character */
  if ((c == cursor) && (state) &&
      (jwidget_has_focus (widget)) &&
      (jwidget_is_enabled (widget)))
    draw_entry_cursor (widget, x, y);
}

static void theme_draw_label(JWidget widget)
{
  int bg = BGCOLOR;

  jdraw_rectfill (widget->rc, bg);

  draw_textstring (NULL, -1, bg, FALSE, widget, widget->rc, 0);
}

static void theme_draw_listbox(JWidget widget)
{
  int bg;

  if (jwidget_is_disabled(widget))
    bg = COLOR_FACE;
  else
    bg = COLOR_BACKGROUND;

  jdraw_rectfill(widget->rc, COLOR_BACKGROUND);
}

static void theme_draw_listitem(JWidget widget)
{
  int fg, bg;
  int x, y;

  if (jwidget_is_disabled(widget)) {
    bg = COLOR_FACE;
    fg = COLOR_DISABLED;
  }
  else if (jwidget_is_selected (widget)) {
    bg = COLOR_SELECTED;
    fg = COLOR_BACKGROUND;
  }
  else {
    bg = COLOR_BACKGROUND;
    fg = COLOR_FOREGROUND;
  }

  x = widget->rc->x1+widget->border_width.l;
  y = widget->rc->y1+widget->border_width.t;

  if (widget->text) {
    /* text */
    jdraw_text(widget->text_font, widget->text, x, y, fg, bg, TRUE);

    /* background */
    _ji_theme_rectfill_exclude
      (ji_screen,
       widget->rc->x1, widget->rc->y1,
       widget->rc->x2-1, widget->rc->y2-1,
       x, y,
       x+jwidget_get_text_length(widget)-1,
       y+jwidget_get_text_height(widget)-1, bg);
  }
  /* background */
  else {
    jdraw_rectfill(widget->rc, bg);
  }
}

static void theme_draw_menu(JWidget widget)
{
  jdraw_rectfill (widget->rc, BGCOLOR);
}

static void theme_draw_menuitem(JWidget widget)
{
  int c, bg, fg, bar;
  int x1, y1, x2, y2;
  JRect pos;

  /* TODO assert? */
  if (!widget->parent->parent)
    return;

  bar = (widget->parent->parent->type == JI_MENUBAR);

  /* colors */
  if (jwidget_is_disabled(widget)) {
    bg = BGCOLOR;
    fg = -1;
  }
  else {
    if (jmenuitem_is_highlight(widget)) {
      bg = COLOR_SELECTED;
      fg = COLOR_BACKGROUND;
    }
    else if (jwidget_has_mouse(widget)) {
      bg = COLOR_HOTFACE;
      fg = COLOR_FOREGROUND;
    }
    else {
      bg = BGCOLOR;
      fg = COLOR_FOREGROUND;
    }
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* background */
  rectfill (ji_screen, x1, y1, x2, y2, bg);

  /* draw an indicator for selected items */
  if (jwidget_is_selected (widget)) {
    BITMAP *icon = icons_bitmap[ICON_MENU_MARK];
    int x = widget->rc->x1+4-icon->w/2;
    int y = (widget->rc->y1+widget->rc->y2)/2-icon->h/2;

    if (jwidget_is_enabled(widget))
      draw_character(ji_screen, icon, x, y, fg);
    else {
      draw_character(ji_screen, icon, x+1, y+1, COLOR_BACKGROUND);
      draw_character(ji_screen, icon, x, y, COLOR_DISABLED);
    }
  }

  /* text */
  if (bar)
    widget->align = JI_CENTER | JI_MIDDLE;
  else
    widget->align = JI_LEFT | JI_MIDDLE;

  pos = jwidget_get_rect (widget);
  if (!bar)
    jrect_displace (pos, widget->child_spacing/2, 0);
  draw_textstring (NULL, fg, bg, FALSE, widget, pos, 0);
  jrect_free (pos);

  /* for menu-box */
  if (!bar) {
    /* draw the arrown (to indicate which this menu has a sub-menu) */
    if (jmenuitem_get_submenu (widget)) {
      /* enabled */
      if (jwidget_is_enabled (widget)) {
	for (c=0; c<3; c++)
	  vline (ji_screen,
		 widget->rc->x2-3-c,
		 (widget->rc->y1+widget->rc->y2)/2-c,
		 (widget->rc->y1+widget->rc->y2)/2+c, fg);
      }
      /* disabled */
      else {
	for (c=0; c<3; c++)
	  vline (ji_screen,
		 widget->rc->x2-3-c+1,
		 (widget->rc->y1+widget->rc->y2)/2-c+1,
		 (widget->rc->y1+widget->rc->y2)/2+c+1, COLOR_BACKGROUND);
	for (c=0; c<3; c++)
	  vline (ji_screen,
		 widget->rc->x2-3-c,
		 (widget->rc->y1+widget->rc->y2)/2-c,
		 (widget->rc->y1+widget->rc->y2)/2+c, COLOR_DISABLED);
      }
    }
    /* draw the keyboard shortcut */
    else if (jmenuitem_get_accel (widget)) {
      int old_align = widget->align;
      char buf[256];

      pos = jwidget_get_rect (widget);
      pos->x2 -= widget->child_spacing/4;

      jaccel_to_string (jmenuitem_get_accel (widget), buf);

      widget->align = JI_RIGHT | JI_MIDDLE;
      draw_textstring (buf, fg, bg, FALSE, widget, pos, 0);
      widget->align = old_align;

      jrect_free (pos);
    }
  }
}

static void theme_draw_panel (JWidget widget)
{
  JWidget c1, c2;
  JLink link;
  int c;

/*   jdraw_rectfill(widget->rc, */
/* 		   (jwidget_pick(widget, */
/* 				   ji_mouse_x(0), */
/* 				   ji_mouse_y(0)) == widget) ? */
/* 		   COLOR_HOTFACE: COLOR_FACE); */
  jdraw_rectfill(widget->rc, COLOR_FACE);

  JI_LIST_FOR_EACH(widget->children, link) {
    if (link->next != widget->children->end) {
      c1 = (JWidget)link->data;
      c2 = (JWidget)link->next->data;

      if (widget->align & JI_HORIZONTAL) {
/* 	vline (ji_screen, */
/* 	       (c1->pos->x+c1->pos->w+c2->pos->x-1)/2, */
/* 	       widget->rect->y, */
/* 	       widget->rect->y+widget->rect->h/2-4, COLOR_FOREGROUND); */

/* 	vline (ji_screen, */
/* 	       (c1->pos->x+c1->pos->w+c2->pos->x-1)/2, */
/* 	       widget->rect->y+widget->rect->h/2+4, */
/* 	       widget->rect->y+widget->rect->h-1, COLOR_FOREGROUND); */

	for (c=-4; c<=4; c+=2)
	  hline(ji_screen,
		c1->rc->x2+2,
		(widget->rc->y1+widget->rc->y2)/2+c,
		c2->rc->x1-3, COLOR_FOREGROUND);
      }
      else {
/* 	hline (ji_screen, */
/* 	       widget->rect->x, */
/* 	       (c1->pos->y+c1->pos->h+c2->pos->y-1)/2, */
/* 	       widget->rect->x+widget->rect->w/2-4, COLOR_FOREGROUND); */

/* 	hline (ji_screen, */
/* 	       widget->rect->x+widget->rect->w/2+4, */
/* 	       (c1->pos->y+c1->pos->h+c2->pos->y-1)/2, */
/* 	       widget->rect->x+widget->rect->w-1, COLOR_FOREGROUND); */

	for (c=-4; c<=4; c+=2)
	  vline(ji_screen,
		(widget->rc->x1+widget->rc->x2)/2+c,
		c1->rc->y2+2,
		c2->rc->y1-3, COLOR_FOREGROUND);
      }
    }
  }
}

static void theme_draw_radio (JWidget widget)
{
  struct jrect box, text, icon;
  int bg = BGCOLOR;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			      ji_generic_button_get_icon_align(widget),
			      widget->theme->radio_icon_size,
			      widget->theme->radio_icon_size);

  /* background */
  jdraw_rectfill (widget->rc, bg);

  /* mouse */
  if (jwidget_is_enabled(widget) && jwidget_has_mouse(widget))
    jdraw_rectfill(&box, bg = COLOR_HOTFACE);

  /* focus */
  if (jwidget_has_focus(widget))
    rect(ji_screen, box.x1-1, box.y1-1, box.x2, box.y2,
	 COLOR_FOREGROUND);

  /* text */
  draw_textstring(NULL, -1, bg, FALSE, widget, &text, 0);

  /* icon */
  draw_icons(icon.x1, icon.y1, widget, ICON_RADIO_EDGE);
}

static void theme_draw_separator(JWidget widget)
{
  int x1, y1, x2, y2;

  /* frame position */
  x1 = widget->rc->x1 + widget->border_width.l/2;
  y1 = widget->rc->y1 + widget->border_width.t/2;
  x2 = widget->rc->x2 - 1 - widget->border_width.r/2;
  y2 = widget->rc->y2 - 1 - widget->border_width.b/2;

  /* background */
  jdraw_rectfill (widget->rc, BGCOLOR);

  /* TOP line */
  if (widget->align & JI_HORIZONTAL) {
    hline (ji_screen, x1, y1-1, x2, COLOR_DISABLED);
    hline (ji_screen, x1, y1, x2, COLOR_BACKGROUND);
  }

  /* LEFT line */
  if (widget->align & JI_VERTICAL) {
    vline (ji_screen, x1-1, y1, y2, COLOR_DISABLED);
    vline (ji_screen, x1, y1, y2, COLOR_BACKGROUND);
  }

  /* frame */
  if ((widget->align & JI_HORIZONTAL) &&
      (widget->align & JI_VERTICAL)) {
    /* union between the LEFT and TOP lines */
    putpixel (ji_screen, x1-1, y1-1, COLOR_DISABLED);

    /* BOTTOM line */
    hline (ji_screen, x1, y2, x2, COLOR_DISABLED);
    hline (ji_screen, x1-1, y2+1, x2, COLOR_BACKGROUND);

    /* RIGHT line */
    vline (ji_screen, x2, y1, y2, COLOR_DISABLED);
    vline (ji_screen, x2+1, y1-1, y2, COLOR_BACKGROUND);

    /* union between the RIGHT and BOTTOM lines */
    putpixel (ji_screen, x2+1, y2+1, COLOR_BACKGROUND);
  }

  /* text */
  if (widget->text) {
    int h = jwidget_get_text_height (widget);
    struct jrect r = { x1+h/2, y1-h/2, x2+1-h, y2+1+h };
    draw_textstring (NULL, -1, BGCOLOR, FALSE, widget, &r, 0);
  }
}

#if 1
/* TODO when Allegro 4.1 will be officially released, replace this
   with the get_clip_rect, add_clip_rect, set_clip_rect functions */
static int my_add_clip_rect (BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
  int u1 = MAX (x1, bitmap->cl);
  int v1 = MAX (y1, bitmap->ct);
  int u2 = MIN (x2, bitmap->cr-1);
  int v2 = MIN (y2, bitmap->cb-1);

  if (u1 > u2 || v1 > v2)
    return FALSE;
  else
    set_clip(bitmap, u1, v1, u2, v2);

  return TRUE;
}
#endif

static void theme_draw_slider(JWidget widget)
{
  int x, x1, y1, x2, y2, bg, c1, c2;
  int min, max, value;
  char buf[256];

  jtheme_slider_info (widget, &min, &max, &value);

  /* main pos */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2 - 1;
  y2 = widget->rc->y2 - 1;

  /* with mouse */
  if (jwidget_is_enabled(widget) && jwidget_has_mouse(widget))
    bg = COLOR_HOTFACE;
  /* without mouse */
  else
    bg = COLOR_FACE;

  /* 1st border */
  _ji_theme_rectedge(ji_screen, x1, y1, x2, y2,
		     COLOR_DISABLED, COLOR_BACKGROUND);

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (jwidget_has_focus (widget))
    rect (ji_screen, x1, y1, x2, y2, COLOR_FOREGROUND);
  else
    rect (ji_screen, x1, y1, x2, y2, bg);

  /* 3rd border */
  if (!jwidget_is_selected (widget)) {
    c1 = COLOR_BACKGROUND;
    c2 = COLOR_DISABLED;
  }
  else {
    c1 = COLOR_DISABLED;
    c2 = COLOR_BACKGROUND;
  }

  x1++, y1++, x2--, y2--;
  _ji_theme_rectedge (ji_screen, x1, y1, x2, y2, c1, c2);

  /* progress bar */
  x1++, y1++, x2--, y2--;

  if (min != max)
    x = x1 + (x2-x1) * (value-min) / (max-min);
  else
    x = x1;

  if (value == min) {
    rectfill (ji_screen, x1, y1, x2, y2, bg);
  }
  else {
    rectfill (ji_screen, x1, y1, x, y2,
	      (jwidget_is_disabled (widget)) ?
	      bg: COLOR_SELECTED);

    if (x < x2)
      rectfill (ji_screen, x+1, y1, x2, y2, bg);
  }

  /* text */
  {
    char *old_text = widget->text;
    int cx1, cy1, cx2, cy2;
    JRect r;

    usprintf(buf, "%d", value);

    widget->align = JI_CENTER | JI_MIDDLE;
    widget->text = buf;

    r = jrect_new(x1, y1, x2+1, y2+1);

    /* TODO when Allegro 4.1 will be officially released, replace this
       with the get_clip_rect, add_clip_rect, set_clip_rect
       functions */

    cx1 = ji_screen->cl;
    cy1 = ji_screen->ct;
    cx2 = ji_screen->cr-1;
    cy2 = ji_screen->cb-1;

    if (my_add_clip_rect(ji_screen, x1, y1, x, y2))
      draw_textstring(NULL, COLOR_BACKGROUND,
		      jwidget_is_disabled(widget) ?
		      bg: COLOR_SELECTED, FALSE, widget, r, 0);

    set_clip(ji_screen, cx1, cy1, cx2, cy2);

    if (my_add_clip_rect(ji_screen, x+1, y1, x2, y2))
      draw_textstring(NULL, COLOR_FOREGROUND, bg, FALSE, widget, r, 0);

    set_clip(ji_screen, cx1, cy1, cx2, cy2);

    widget->text = old_text;
    jrect_free(r);
  }
}

static void theme_draw_textbox(JWidget widget)
{
  _ji_theme_textbox_draw(ji_screen, widget, NULL, NULL);
}

static void theme_draw_view(JWidget widget)
{
  JRect pos = jwidget_get_rect (widget);

  if (jwidget_has_focus(widget)) {
    /* 1st border */
    jdraw_rectedge(pos, COLOR_DISABLED, COLOR_BACKGROUND);

    /* 2nd border */
    jrect_shrink(pos, 1);
    jdraw_rect(pos, COLOR_FOREGROUND);
  }
  else {
    /* 1st border */
    jdraw_rectedge(pos, COLOR_DISABLED, COLOR_BACKGROUND);

    /* 2nd border */
    jrect_shrink(pos, 1);
    jdraw_rect(pos, BGCOLOR);
  }

  /* background */
  jrect_shrink(pos, 1);
  jdraw_rectfill(pos, BGCOLOR);

  jrect_free (pos);
}

static void theme_draw_view_scrollbar(JWidget widget)
{
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int pos, len;

  jtheme_scrollbar_info (widget, &pos, &len);

  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* border */
  rect (ji_screen, x1, y1, x2, y2, BGCOLOR);

  /* draw the content */
  x1++, y1++, x2--, y2--;

  /* horizontal bar */
  if (widget->align & JI_HORIZONTAL) {
    u1 = x1+pos;
    v1 = y1;
    u2 = x1+pos+len-1;
    v2 = y2;
  }
  /* vertical bar */
  else {
    u1 = x1;
    v1 = y1+pos;
    u2 = x2;
    v2 = y1+pos+len-1;
  }

  /* background */
  _ji_theme_rectfill_exclude (ji_screen,
			      x1, y1, x2, y2,
			      u1, v1, u2, v2, BGCOLOR);

  /* 1st border */
  if (jwidget_is_selected (widget))
    _ji_theme_rectedge (ji_screen, u1, v1, u2, v2,
			COLOR_DISABLED, COLOR_BACKGROUND);
  else
    _ji_theme_rectedge (ji_screen, u1, v1, u2, v2,
			COLOR_BACKGROUND, COLOR_DISABLED);

  /* bar-block background */
  u1++, v1++, u2--, v2--;
  if (jwidget_is_enabled(widget) && jwidget_has_mouse(widget))
    rectfill(ji_screen, u1, v1, u2, v2, COLOR_HOTFACE);
  else
    rectfill(ji_screen, u1, v1, u2, v2, BGCOLOR);
}

static void theme_draw_view_viewport (JWidget widget)
{
  jdraw_rectfill(widget->rc, BGCOLOR);
}

static void theme_draw_window(JWidget widget)
{
  JRect pos = jwidget_get_rect(widget);
  JRect cpos = jwidget_get_child_rect(widget);

  /* extra lines */
  if (!jwindow_is_desktop(widget)) {
    jdraw_rect(pos, COLOR_FOREGROUND);
    jrect_shrink(pos, 1);
    jdraw_rectedge(pos, COLOR_BACKGROUND, COLOR_DISABLED);
    jrect_shrink(pos, 1);
    jdraw_rectfill(pos, BGCOLOR);

    /* draw title bar */
    if (widget->text) {
      int bg = COLOR_SELECTED;

      jrect_shrink(pos, 1);
/*       pos->y2 = cpos->y1-1; */
      pos->y2 = cpos->y1-3;
      jdraw_rectfill(pos, bg);

      jrect_stretch(pos, 1);
      jdraw_rectedge(cpos, COLOR_DISABLED, COLOR_BACKGROUND);

      jdraw_text(widget->text_font, widget->text,
		   cpos->x1,
		   pos->y1+jrect_h(pos)/2-text_height(widget->text_font)/2,
		   COLOR_BACKGROUND, bg, FALSE);
    }
  }
  /* desktop */
  else {
    jdraw_rectfill(pos, widget->theme->desktop_color);
  }

  jrect_free(pos);
  jrect_free(cpos);
}

static int get_bg_color (JWidget widget)
{
  int c = jwidget_get_bg_color (widget);
  int decorative = jwidget_is_decorative (widget);

  return c >= 0 ? c: (decorative ? COLOR_SELECTED:
				   COLOR_FACE);
}

static void draw_textstring (const char *t, int fg_color, int bg_color,
			     bool fill_bg, JWidget widget, const JRect rect,
			     int selected_offset)
{
  if (t || widget->text) {
    int x, y, w, h;

    if (!t) {
      t = widget->text;
      w = jwidget_get_text_length (widget);
      h = jwidget_get_text_height (widget);
    }
    else {
      w = ji_font_text_len (widget->text_font, t);
      h = text_height (widget->text_font);
    }

    /* horizontally text alignment */

    if (widget->align & JI_RIGHT)
      x = rect->x2 - w;
    else if (widget->align & JI_CENTER)
      x = (rect->x1+rect->x2)/2 - w/2;
    else
      x = rect->x1;

    /* vertically text alignment */

    if (widget->align & JI_BOTTOM)
      y = rect->y2 - h;
    else if (widget->align & JI_MIDDLE)
      y = (rect->y1+rect->y2)/2 - h/2;
    else
      y = rect->y1;

    if (jwidget_is_selected (widget)) {
      x += selected_offset;
      y += selected_offset;
    }

    /* background */
    if (bg_color >= 0) {
      if (jwidget_is_disabled (widget))
	rectfill (ji_screen, x, y, x+w, y+h, bg_color);
      else
	rectfill (ji_screen, x, y, x+w-1, y+h-1, bg_color);
    }

    /* text */
    if (jwidget_is_disabled (widget)) {
      /* TODO avoid this */
      if (fill_bg)		/* only to draw the background */
	jdraw_text(widget->text_font, t, x, y, 0, bg_color, fill_bg);

      /* draw white part */
      jdraw_text(widget->text_font, t, x+1, y+1,
		 COLOR_BACKGROUND, bg_color, fill_bg);

      if (fill_bg)
	fill_bg = FALSE;
    }

    jdraw_text(widget->text_font, t, x, y,
	       jwidget_is_disabled(widget) ?
	       COLOR_DISABLED: (fg_color >= 0 ? fg_color :
						COLOR_FOREGROUND),
	       bg_color, fill_bg);
  }
}

static void draw_entry_cursor(JWidget widget, int x, int y)
{
  int h = jwidget_get_text_height(widget);

  vline(ji_screen, x,   y-1, y+h, COLOR_FOREGROUND);
  vline(ji_screen, x+1, y-1, y+h, COLOR_FOREGROUND);
}

static void draw_icons(int x, int y, JWidget widget, int edge_icon)
{
  draw_sprite(ji_screen, icons_bitmap[edge_icon], x, y);

  if (jwidget_is_selected(widget))
    draw_sprite(ji_screen, icons_bitmap[edge_icon+1], x, y);
}

static void draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel)
{
  hline(ji_screen, x1+bevel[0], y1, x2-bevel[1], c1); /* top */
  hline(ji_screen, x1+bevel[2], y2, x2-bevel[3], c2); /* bottom */

  vline(ji_screen, x1, y1+bevel[0], y2-bevel[2], c1); /* left */
  vline(ji_screen, x2, y1+bevel[1], y2-bevel[3], c2); /* right */

  line(ji_screen, x1, y1+bevel[0], x1+bevel[0], y1, c1); /* top-left */
  line(ji_screen, x1, y2-bevel[2], x1+bevel[2], y2, c2); /* bottom-left */

  line(ji_screen, x2-bevel[1], y1, x2, y1+bevel[1], c2); /* top-right */
  line(ji_screen, x2-bevel[3], y2, x2, y2-bevel[3], c2); /* bottom-right */
}

static void less_bevel(int *bevel)
{
  if (bevel[0] > 0) --bevel[0];
  if (bevel[1] > 0) --bevel[1];
  if (bevel[2] > 0) --bevel[2];
  if (bevel[3] > 0) --bevel[3];
}

/* controls the "X" button in a window to close it */
static bool theme_button_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_KEYPRESSED:
      if (msg->key.scancode == KEY_ESC) {
	jwidget_select(widget);
	return TRUE;
      }
      break;

    case JM_KEYRELEASED:
      if (msg->key.scancode == KEY_ESC) {
	if (jwidget_is_selected(widget)) {
	  jwidget_deselect(widget);
	  jwidget_close_window(widget);
	  return TRUE;
	}
      }
      break;
  }

  return FALSE;
}

