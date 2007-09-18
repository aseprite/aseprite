/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete.h"
#include "jinete/intern.h"

#define CHARACTER_LENGTH(f, c) ((f)->vtable->char_length ((f), (c)))

static void theme_destroy (void);
static void theme_regen (void);
static void theme_set_cursor (int type);
static void theme_init_widget (JWidget widget);
static JRegion theme_get_window_mask (JWidget widget);
static void theme_map_decorative_widget (JWidget widget);
static void theme_draw_box (JWidget widget);
static void theme_draw_button (JWidget widget);
static void theme_draw_check (JWidget widget);
static void theme_draw_entry (JWidget widget);
static void theme_draw_label (JWidget widget);
static void theme_draw_listbox (JWidget widget);
static void theme_draw_listitem (JWidget widget);
static void theme_draw_menu (JWidget widget);
static void theme_draw_menuitem (JWidget widget);
static void theme_draw_panel (JWidget widget);
static void theme_draw_radio (JWidget widget);
static void theme_draw_separator (JWidget widget);
static void theme_draw_slider (JWidget widget);
static void theme_draw_textbox (JWidget widget);
static void theme_draw_view (JWidget widget);
static void theme_draw_view_scrollbar (JWidget widget);
static void theme_draw_view_viewport (JWidget widget);
static void theme_draw_window (JWidget widget);

static void draw_entry_cursor (JWidget widget, int x, int y);
static void get_colors (JWidget widget, int *fg, int *bg);

static bool theme_button_msg_proc (JWidget widget, JMessage msg);

JTheme jtheme_new_simple (void)
{
  JTheme theme;

  theme = jtheme_new ();
  if (!theme)
    return NULL;

  theme->name = "Simple Theme";
  theme->check_icon_size = 8;
  theme->radio_icon_size = 8;
  theme->scrollbar_size = 10;

  theme->destroy = theme_destroy;
  theme->regen = theme_regen;
  theme->set_cursor = theme_set_cursor;
  theme->init_widget = theme_init_widget;
  theme->get_window_mask = theme_get_window_mask;
  theme->map_decorative_widget = theme_map_decorative_widget;

  jtheme_set_method (theme, JI_BOX, theme_draw_box);
  jtheme_set_method (theme, JI_BUTTON, theme_draw_button);
  jtheme_set_method (theme, JI_CHECK, theme_draw_check);
  jtheme_set_method (theme, JI_ENTRY, theme_draw_entry);
  jtheme_set_method (theme, JI_LABEL, theme_draw_label);
  jtheme_set_method (theme, JI_LISTBOX, theme_draw_listbox);
  jtheme_set_method (theme, JI_LISTITEM, theme_draw_listitem);
  jtheme_set_method (theme, JI_MENU, theme_draw_menu);
  jtheme_set_method (theme, JI_MENUITEM, theme_draw_menuitem);
  jtheme_set_method (theme, JI_PANEL, theme_draw_panel);
  jtheme_set_method (theme, JI_RADIO, theme_draw_radio);
  jtheme_set_method (theme, JI_SEPARATOR, theme_draw_separator);
  jtheme_set_method (theme, JI_SLIDER, theme_draw_slider);
  jtheme_set_method (theme, JI_TEXTBOX, theme_draw_textbox);
  jtheme_set_method (theme, JI_VIEW, theme_draw_view);
  jtheme_set_method (theme, JI_VIEW_SCROLLBAR, theme_draw_view_scrollbar);
  jtheme_set_method (theme, JI_VIEW_VIEWPORT, theme_draw_view_viewport);
  jtheme_set_method (theme, JI_WINDOW, theme_draw_window);

  return theme;
}

static void theme_destroy (void)
{
}

static void theme_regen (void)
{
  JTheme theme = ji_get_theme ();

  theme->desktop_color = makecol(255,255,255);
  theme->textbox_fg_color = makecol(0,0,0);
  theme->textbox_bg_color = makecol(255,255,255);
}

static void theme_set_cursor (int type)
{
  set_mouse_sprite (NULL);
}

static void theme_init_widget (JWidget widget)
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
      widget->child_spacing = -1;
      break;

    case JI_BUTTON:
      BORDER4(2, 2, 2, 3);
      widget->child_spacing = 0;
      break;

    case JI_CHECK:
      BORDER(2);
      widget->child_spacing = 1;
      break;

    case JI_ENTRY:
      BORDER(2);
      break;

    case JI_LABEL:
      BORDER(2);
      break;

    case JI_LISTBOX:
      BORDER (0);
      widget->child_spacing = 0;
      break;

    case JI_LISTITEM:
      BORDER (0);
      break;

    case JI_MENU:
    case JI_MENUBAR:
    case JI_MENUBOX:
      BORDER (0);
      widget->child_spacing = 0;
      break;

    case JI_MENUITEM:
      BORDER (0);
      widget->child_spacing = 18;
      break;

    case JI_PANEL:
      BORDER (0);
      widget->child_spacing = 3;
      break;

    case JI_RADIO:
      BORDER (2);
      widget->child_spacing = 1;
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
      BORDER (4);
      widget->child_spacing = jwidget_get_text_height (widget);
      break;

    case JI_TEXTBOX:
      BORDER (2);
      widget->child_spacing = 0;
      break;

    case JI_VIEW:
      BORDER (1);
      widget->child_spacing = 0;
      break;

    case JI_VIEW_SCROLLBAR:
      if (widget->align & JI_HORIZONTAL) {
	BORDER4 (0, 1, 0, 0);
      }
      else {
	BORDER4 (1, 0, 0, 0);
      }
      widget->child_spacing = 0;
      break;

    case JI_VIEW_VIEWPORT:
      BORDER (0);
      widget->child_spacing = 0;
      break;

    case JI_WINDOW:
      if (!jwindow_is_desktop (widget)) {
	if (widget->text) {
	  BORDER4 (3, 2+jwidget_get_text_height (widget)+1, 3, 3);
#if 1				/* add close button */
	  if (!(widget->flags & JI_INITIALIZED)) {
	    JWidget button = jbutton_new ("x");
	    jwidget_add_hook (button, JI_WIDGET,
				theme_button_msg_proc, NULL);
	    jwidget_decorative (button, TRUE);
	    jwidget_add_child (widget, button);
	    jwidget_set_name (button, "theme_close_button");
	  }
#endif
	}
	else if (!(widget->flags & JI_INITIALIZED)) {
	  BORDER (3);
	}
      }
      else {
	BORDER (0);
      }
      widget->child_spacing = 2;
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
  if (ustrcmp (widget->name, "theme_close_button") == 0) {
    JWidget window = widget->parent;
    JRect rect = jrect_new (0, 0, 0, 0);

    rect->x2 = 2 + jwidget_get_text_height (widget) + 2;
    rect->y2 = 2 + jwidget_get_text_height (widget) + 2;

    jrect_displace (rect,
		      window->rc->x2 - jrect_w(rect),
		      window->rc->y1);

    jwidget_set_rect (widget, rect);
    jrect_free (rect);
  }
}

static void theme_draw_box (JWidget widget)
{
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jdraw_rectfill(widget->rc, bg);
}

static void theme_draw_button (JWidget widget)
{
  BITMAP *icon_bmp = ji_generic_button_get_icon(widget);
  int icon_align = ji_generic_button_get_icon_align(widget);
  JRect pos = jwidget_get_rect(widget);
  struct jrect box, text, icon;
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jwidget_get_texticon_info(widget, &box, &text, &icon,
			      icon_align,
			      icon_bmp ? icon_bmp->w : 0,
			      icon_bmp ? icon_bmp->h : 0);

  /* border */
  jdraw_rect(pos, makecol(0,0,0));
  jrect_shrink(pos, 1);

  /* background */
  jdraw_rectfill(pos, bg);

  /* text */
  if (widget->text)
    jdraw_text(widget->text_font, widget->text,
	       text.x1, text.y1, fg, bg, FALSE);

  /* icon */
  if (icon_bmp) {
    if (jwidget_is_enabled(widget))
      draw_sprite(ji_screen, icon_bmp, icon.x1, icon.y1);
    else
      _ji_theme_draw_sprite_color(ji_screen, icon_bmp, icon.x1, icon.y1, fg);
  }

  jrect_free(pos);
}

static void theme_draw_check (JWidget widget)
{
  struct jrect box, text, icon;
  int fg, bg;

  get_colors (widget, &fg, &bg);
  jwidget_get_texticon_info(widget, &box, &text, &icon,
			      ji_generic_button_get_icon_align(widget),
			      widget->theme->check_icon_size,
			      widget->theme->check_icon_size);

  /* background */
  jdraw_rectfill (widget->rc, bg);

  /* focus */
  /* XXX */

  /* text */
  jdraw_text(widget->text_font, widget->text,
	       text.x1, text.y1, fg, bg, FALSE);

  /* icon */
  jdraw_rect(&icon, fg);
  jrect_shrink(&icon, 1);
  if (jwidget_is_selected(widget)) {
    line(ji_screen, icon.x1, icon.y1, icon.x2-1, icon.y2-1, fg);
    line(ji_screen, icon.x1, icon.y2-1, icon.x2-1, icon.y1, fg);
  }
}

static void theme_draw_entry (JWidget widget)
{
  JRect rect = jwidget_get_rect (widget);
  bool password = jentry_is_password (widget);
  int scroll, cursor, state, selbeg, selend;
  int c, ch, x, y, w, fg, bg, cfg, cbg;
  const char *text = widget->text;
  int cursor_x;

  get_colors (widget, &fg, &bg);
  jtheme_entry_info (widget, &scroll, &cursor, &state, &selbeg, &selend);

  /* border */
  jdraw_rect (rect, fg);
  jrect_shrink (rect, 1);

  /* background */
  jdraw_rectfill (rect, bg);

  /* draw the text */
  x = rect->x1;
  y = rect->y1;

  for (c=scroll; ugetat (text, c); c++) {
    ch = password ? '*': ugetat (text, c);

    if ((c >= selbeg) && (c <= selend)) {
      cbg = fg;
      cfg = bg;
    }
    else {
      cbg = bg;
      cfg = fg;
    }

    w = CHARACTER_LENGTH (widget->text_font, ch);
    if (x+w >= rect->x2)
      return;

    cursor_x = x;
    ji_font_set_aa_mode (widget->text_font, cbg);
    widget->text_font->vtable->render_char (widget->text_font,
					    ch, cfg, cbg, ji_screen, x, y);
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

  jrect_free (rect);
}

static void theme_draw_label (JWidget widget)
{
  JRect rect = jwidget_get_rect (widget);
  struct jrect text;
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jwidget_get_texticon_info(widget, NULL, &text, NULL, 0, 0, 0);

  jdraw_rect(rect, makecol(0,0,0));
  jrect_shrink(rect, 1);

  jdraw_rectfill(rect, bg);
  jdraw_text(widget->text_font, widget->text,
	       text.x1, text.y1, fg, bg, FALSE);

  jrect_free(rect);
}

static void theme_draw_listbox(JWidget widget)
{
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jdraw_rectfill(widget->rc, bg);
}

static void theme_draw_listitem (JWidget widget)
{
  JRect rect = jwidget_get_rect (widget);
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jdraw_rectfill(rect, bg);

  if (widget->text)
    jdraw_text(widget->text_font, widget->text,
		 rect->x1, rect->y1, fg, bg, TRUE);

  jrect_free(rect);
}

static void theme_draw_menu (JWidget widget)
{
  int fg, bg;

  get_colors (widget, &fg, &bg);
  jdraw_rectfill (widget->rc, bg);
}

static void theme_draw_menuitem (JWidget widget)
{
#if 0
  int c, bg, fg, bar;
  int x1, y1, x2, y2;
  JRect pos;

  /* XXXX assert? */
  if (!widget->parent->parent)
    return;

  bar = (widget->parent->parent->type == JI_MENUBAR);

  /* colors */
  if (jwidget_is_disabled (widget)) {
    bg = BGCOLOR;
    fg = -1;
  }
  else {
    if (jmenuitem_is_highlight (widget)) {
      bg = makecol(44,76,145);
      fg = makecol(255,255,255);
    }
    else if (jwidget_has_mouse (widget)) {
      bg = makecol(224,224,224);
      fg = makecol(0,0,0);
    }
    else {
      bg = BGCOLOR;
      fg = makecol(0,0,0);
    }
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* border */
  if ((jwidget_is_enabled (widget)) &&
      (jmenuitem_is_highlight (widget))) {
    rect (ji_screen, x1, y1, x2, y2, makecol (0, 0, 0));
    x1++, y1++, x2--, y2--;
  }

  /* background */
  rectfill (ji_screen, x1, y1, x2, y2, bg);

  /* draw an indicator for selected items */
  if (jwidget_is_selected (widget)) {
    BITMAP *icon = icons_bitmap[ICON_MENU_MARK];
    int x = widget->rc->x+4-icon->w/2;
    int y = widget->rc->y+widget->rc->h/2-icon->h/2;

    if (jwidget_is_enabled (widget))
      draw_character (ji_screen, icon, x, y, fg);
    else {
      draw_character (ji_screen, icon, x+1, y+1, makecol (255, 255, 255));
      draw_character (ji_screen, icon, x, y, makecol (128, 128, 128));
    }
  }

  /* text */
  if (bar)
    widget->align = JI_CENTER | JI_MIDDLE;
  else
    widget->align = JI_LEFT | JI_MIDDLE;

  pos = jwidget_get_rect (widget);
  if (!bar)
    pos->x += widget->child_spacing/2;
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
		 widget->rect->x+widget->rect->w-3-c,
		 widget->rect->y+widget->rect->h/2-c,
		 widget->rect->y+widget->rect->h/2+c, fg);
      }
      /* disabled */
      else {
	for (c=0; c<3; c++)
	  vline (ji_screen,
		 widget->rect->x+widget->rect->w-3-c+1,
		 widget->rect->y+widget->rect->h/2-c+1,
		 widget->rect->y+widget->rect->h/2+c+1, makecol (255, 255, 255));
	for (c=0; c<3; c++)
	  vline (ji_screen,
		 widget->rect->x+widget->rect->w-3-c,
		 widget->rect->y+widget->rect->h/2-c,
		 widget->rect->y+widget->rect->h/2+c, makecol (128, 128, 128));
      }
    }
    /* draw the keyboard shortcut */
    else if (jmenuitem_get_accel (widget)) {
      int old_align = widget->align;
      char buf[256];

      pos = jwidget_get_rect (widget);
      pos->w -= widget->child_spacing/4;

      jaccel_to_string (jmenuitem_get_accel (widget), buf);

      widget->align = JI_RIGHT | JI_MIDDLE;
      draw_textstring (buf, fg, bg, FALSE, widget, pos, 0);
      widget->align = old_align;

      jrect_free (pos);
    }
  }
#endif
}

static void theme_draw_panel (JWidget widget)
{
  JRect rect = jwidget_get_rect (widget);
  int fg, bg;

  get_colors (widget, &fg, &bg);
  jdraw_rect (rect, fg);
  jrect_shrink (rect, 1);
  jdraw_rectfill (rect, bg);

  jrect_free (rect);
}

static void theme_draw_radio (JWidget widget)
{
  struct jrect box, text, icon;
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jwidget_get_texticon_info(widget, &box, &text, &icon,
			      ji_generic_button_get_icon_align(widget),
			      widget->theme->check_icon_size,
			      widget->theme->check_icon_size);

  /* background */
  jdraw_rectfill(widget->rc, bg);

  /* focus */
  /* XXX */

  /* text */
  jdraw_text(widget->text_font, widget->text,
	       text.x1, text.y1, fg, bg, FALSE);

  /* icon */
  circle(ji_screen, (icon.x1+icon.x2)/2, (icon.y1+icon.y2)/2, 4, fg);
  if (jwidget_is_selected(widget))
    circlefill(ji_screen, (icon.x1+icon.x2)/2, (icon.y1+icon.y2)/2, 2, fg);
}

static void theme_draw_separator (JWidget widget)
{
#if 0
  int x1, y1, x2, y2;

  /* frame position */
  x1 = widget->rc->x1 + widget->border_width.l/2;
  y1 = widget->rc->y1 + widget->border_width.t/2;
  x2 = widget->rc->x2 - 1 - widget->border_width.r/2;
  y2 = widget->rc->y2 - 1 - widget->border_width.b/2;

  /* background */
  rectfill (ji_screen, widget->rect->x, widget->rect->y,
	    widget->rect->x + widget->rect->w - 1,
	    widget->rect->y + widget->rect->h - 1, BGCOLOR);

  /* TOP line */
  if (widget->align & JI_HORIZONTAL) {
    hline (ji_screen, x1, y1-1, x2, makecol (128, 128, 128));
    hline (ji_screen, x1, y1, x2, makecol (255, 255, 255));
  }

  /* LEFT line */
  if (widget->align & JI_VERTICAL) {
    vline (ji_screen, x1-1, y1, y2, makecol (128, 128, 128));
    vline (ji_screen, x1, y1, y2, makecol (255, 255, 255));
  }

  /* frame */
  if ((widget->align & JI_HORIZONTAL) &&
      (widget->align & JI_VERTICAL)) {
    /* union between the LEFT and TOP lines */
    putpixel (ji_screen, x1-1, y1-1, makecol (128, 128, 128));

    /* BOTTOM line */
    hline (ji_screen, x1, y2, x2, makecol (128, 128, 128));
    hline (ji_screen, x1-1, y2+1, x2, makecol (255, 255, 255));

    /* RIGHT line */
    vline (ji_screen, x2, y1, y2, makecol (128, 128, 128));
    vline (ji_screen, x2+1, y1-1, y2, makecol (255, 255, 255));

    /* union between the RIGHT and BOTTOM lines */
    putpixel (ji_screen, x2+1, y2+1, makecol (255, 255, 255));
  }

  /* text */
  if (widget->text) {
    int h = jwidget_get_text_height (widget);
    struct jrect r = { x1+h/2, y1-h/2, x2+1-h, y2+1+h };
    draw_textstring (NULL, -1, BGCOLOR, FALSE, widget, &r, 0);
  }
#endif
}

static void theme_draw_slider (JWidget widget)
{
#if 0
  int x, x1, y1, x2, y2, bg, c1, c2;
  int min, max, value;
  char buf[256];

  jtheme_slider_info (widget, &min, &max, &value);

  /* main pos */
  x1 = widget->rect->x;
  y1 = widget->rect->y;
  x2 = widget->rect->x + widget->rect->w - 1;
  y2 = widget->rect->y + widget->rect->h - 1;

  /* with mouse */
  if (jwidget_has_mouse (widget))
    bg = makecol (224, 224, 224);
  /* without mouse */
  else
    bg = makecol (192, 192, 192);

  /* 1st border */
  _ji_theme_rectedge (ji_screen, x1, y1, x2, y2,
		      makecol (128, 128, 128), makecol (255, 255, 255));

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (jwidget_has_focus (widget))
    rect (ji_screen, x1, y1, x2, y2, makecol (0, 0, 0));
  else
    rect (ji_screen, x1, y1, x2, y2, bg);

  /* 3rd border */
  if (!jwidget_is_selected (widget)) {
    c1 = makecol (255, 255, 255);
    c2 = makecol (128, 128, 128);
  }
  else {
    c1 = makecol (128, 128, 128);
    c2 = makecol (255, 255, 255);
  }

  x1++, y1++, x2--, y2--;
  _ji_theme_rectedge (ji_screen, x1, y1, x2, y2, c1, c2);

  /* progress bar */
  x1++, y1++, x2--, y2--;

  x = x1 + (x2-x1) * (value-min) / (max-min);

  if (value == min) {
    rectfill (ji_screen, x1, y1, x2, y2, bg);
  }
  else {
    rectfill (ji_screen, x1, y1, x, y2,
	      (jwidget_is_disabled (widget)) ?
	      bg: makecol (44, 76, 145));

    if (x < x2)
      rectfill (ji_screen, x+1, y1, x2, y2, bg);
  }

  /* text */
  {
    char *old_text = widget->text;
    int cx1, cy1, cx2, cy2;
    JRect r;

    usprintf (buf, "%d", value);

    widget->align = JI_CENTER | JI_MIDDLE;
    widget->text = buf;

    r = jrect_new (x1, y1, x2-x1+1, y2-y1+1);

    /* XXXX when Allegro 4.1 will be officially released, replace this
       with the get_clip_rect, add_clip_rect, set_clip_rect functions */

    cx1 = ji_screen->cl;
    cy1 = ji_screen->ct;
    cx2 = ji_screen->cr-1;
    cy2 = ji_screen->cb-1;

    if (my_add_clip_rect (ji_screen, x1, y1, x, y2))
      draw_textstring (NULL, makecol (255, 255, 255),
		       jwidget_is_disabled (widget) ?
		       bg: makecol (44, 76, 145), FALSE, widget, r, 0);

    set_clip (ji_screen, cx1, cy1, cx2, cy2);

    if (my_add_clip_rect (ji_screen, x+1, y1, x2, y2))
      draw_textstring (NULL, makecol (0, 0, 0), bg, FALSE, widget, r, 0);

    set_clip (ji_screen, cx1, cy1, cx2, cy2);

    widget->text = old_text;
    jrect_free (r);
  }
#endif
}

static void theme_draw_textbox (JWidget widget)
{
  _ji_theme_textbox_draw (ji_screen, widget, NULL, NULL);
}

static void theme_draw_view (JWidget widget)
{
  JRect pos = jwidget_get_rect(widget);
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jdraw_rect(pos, fg);
  jrect_shrink(pos, 1);
  jdraw_rectfill(pos, bg);
  jrect_replace(pos,
		  widget->rc->x2-widget->theme->scrollbar_size-1,
		  widget->rc->y2-widget->theme->scrollbar_size-1,
		  widget->rc->x2, widget->rc->y2);
  jdraw_rect(pos, fg);
  jrect_free(pos);
}

static void theme_draw_view_scrollbar (JWidget widget)
{
  JRect rect = jwidget_get_rect(widget);
  JRect cpos = jwidget_get_child_rect(widget);
  int pos, len;
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jtheme_scrollbar_info(widget, &pos, &len);

  jdraw_rect(rect, makecol (0, 0, 0));
  jdraw_rectfill(cpos, bg);

  /* horizontal bar */
  if (widget->align & JI_HORIZONTAL) {
    cpos->x1 += pos;
    cpos->x2 = cpos->x1+len;
  }
  /* vertical bar */
  else {
    cpos->y1 += pos;
    cpos->y2 = cpos->y1+len;
  }
  jdraw_rectfill(cpos, fg);

  jrect_free(rect);
  jrect_free(cpos);
}

static void theme_draw_view_viewport (JWidget widget)
{
  int fg, bg;

  get_colors(widget, &fg, &bg);
  jdraw_rectfill(widget->rc, bg);
}

static void theme_draw_window (JWidget widget)
{
  JRect pos = jwidget_get_rect (widget);
  int fg, bg;

  get_colors(widget, &fg, &bg);

  /* extra lines */
  if (!jwindow_is_desktop (widget)) {
    jdraw_rect(pos, fg);
    jrect_shrink(pos, 1);
    jdraw_rectfill(pos, bg);
    hline(ji_screen, pos->x1, pos->y1+1+text_height(font)+1, pos->x2-1, fg);
    jrect_shrink(pos, 1);

    /* draw title bar */
    if (widget->text)
      jdraw_text(widget->text_font, widget->text,
		   pos->x1, pos->y1, fg, bg, FALSE);
  }
  /* desktop */
  else {
    jdraw_rectfill(pos, widget->theme->desktop_color);
  }

  jrect_free(pos);
}

static void draw_entry_cursor(JWidget widget, int x, int y)
{
  int h = jwidget_get_text_height(widget);

  vline(ji_screen, x, y-1, y+h, makecol(0, 0, 0));
}

static void get_colors(JWidget widget, int *fg, int *bg)
{
  if (jwidget_is_disabled(widget)) {
    *bg = makecol(255, 255, 255);
    *fg = makecol(128, 128, 128);
  }
  else if (jwidget_is_selected(widget)) {
    *bg = makecol(0, 0, 0);
    *fg = makecol(255, 255, 255);
  }
  else {
    *bg = makecol(255, 255, 255);
    *fg = makecol(0, 0, 0);
  }
}

static bool theme_button_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_KEYPRESSED:
      if (msg->key.scancode == KEY_ESC) {
	jwidget_select (widget);
	return TRUE;
      }
      break;

    case JM_KEYRELEASED:
      if (msg->key.scancode == KEY_ESC) {
	if (jwidget_is_selected (widget)) {
	  jwidget_deselect (widget);
	  jwidget_close_window (widget);
	  return TRUE;
	}
      }
      break;
  }

  return FALSE;
}
