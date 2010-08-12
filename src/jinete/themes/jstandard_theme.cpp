/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
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
 *   * Neither the name of the author nor the names of its contributors may
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

#include "jinete/jinete.h"
#include "jinete/jintern.h"

/* data related */
#include "jstandard_theme_icons.h"
#include "pcx2data.cpp"

#define CHARACTER_LENGTH(f, c)	((f)->vtable->char_length((f), (c)))

#define COLOR_FOREGROUND	makecol(0, 0, 0)
#define COLOR_DISABLED		makecol(128, 128, 128)
#define COLOR_FACE		makecol(210, 200, 190)
#define COLOR_HOTFACE		makecol(250, 240, 230)
#define COLOR_SELECTED		makecol(44, 76, 145)
#define COLOR_BACKGROUND	makecol(255, 255, 255)

/* "icons_data" indexes */
enum {
  FIRST_CURSOR = 0,
  LAST_CURSOR = 14,
  ICON_CHECK_EDGE = 15,
  ICON_CHECK_MARK,
  ICON_CLOSE,
  ICON_MENU_MARK,
  ICON_RADIO_EDGE,
  ICON_RADIO_MARK,
  ICON_COMBOBOX,
  ICONS,
};

static struct {
  bool mask : 1;
  unsigned char *data;
} icons_data[ICONS] = {
  { false, default_theme_cnormal },
  { false, default_theme_cnoradd },
  { false, default_theme_cforbidden },
  { false, default_theme_chand },
  { false, default_theme_cscroll },
  { false, default_theme_cmove },
  { false, default_theme_csizetl },
  { false, default_theme_csizet },
  { false, default_theme_csizetr },
  { false, default_theme_csizel },
  { false, default_theme_csizer },
  { false, default_theme_csizebl },
  { false, default_theme_csizeb },
  { false, default_theme_csizebr },
  { false, default_theme_ceyedropper },
  { false, default_theme_ichecke },
  { false, default_theme_icheckm },
  { false, default_theme_iclose },
  {  true, default_theme_imenum },
  { false, default_theme_iradioe },
  { false, default_theme_iradiom },
  { false, default_theme_icombobox },
};

class jstandard_theme : public jtheme
{
  BITMAP *icons_bitmap[ICONS];

public:
  jstandard_theme();
  ~jstandard_theme();

  void regen();
  BITMAP *set_cursor(int type, int *focus_x, int *focus_y);
  void init_widget(JWidget widget);
  JRegion get_window_mask(JWidget widget);
  void map_decorative_widget(JWidget widget);

  int color_foreground();
  int color_disabled();
  int color_face();
  int color_hotface();
  int color_selected();
  int color_background();

  void draw_box(JWidget widget, JRect clip);
  void draw_button(JWidget widget, JRect clip);
  void draw_check(JWidget widget, JRect clip);
  void draw_entry(JWidget widget, JRect clip);
  void draw_grid(JWidget widget, JRect clip);
  void draw_label(JWidget widget, JRect clip);
  void draw_link_label(JWidget widget, JRect clip);
  void draw_listbox(JWidget widget, JRect clip);
  void draw_listitem(JWidget widget, JRect clip);
  void draw_menu(JWidget widget, JRect clip);
  void draw_menuitem(JWidget widget, JRect clip);
  void draw_panel(JWidget widget, JRect clip);
  void draw_radio(JWidget widget, JRect clip);
  void draw_separator(JWidget widget, JRect clip);
  void draw_slider(JWidget widget, JRect clip);
  void draw_combobox_entry(JWidget widget, JRect clip);
  void draw_combobox_button(JWidget widget, JRect clip);
  void draw_textbox(JWidget widget, JRect clip);
  void draw_view(JWidget widget, JRect clip);
  void draw_view_scrollbar(JWidget widget, JRect clip);
  void draw_view_viewport(JWidget widget, JRect clip);
  void draw_frame(Frame* frame, JRect clip);

private:

  int get_bg_color(JWidget widget);
  void draw_textstring(const char *t, int fg_color, int bg_color,
		       bool fill_bg, JWidget widget, const JRect rect,
		       int selected_offset);
  void draw_entry_cursor(JWidget widget, int x, int y);
  void draw_icons(int x, int y, JWidget widget, int edge_icon);
  void draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel);
  void less_bevel(int *bevel);

};

static bool theme_button_msg_proc(JWidget widget, JMessage msg);

JTheme jtheme_new_standard()
{
  return new jstandard_theme();
}

jstandard_theme::jstandard_theme()
{
  for (int c=0; c<ICONS; c++)
    icons_bitmap[c] = NULL;

  this->name = "Standard Theme";
  this->check_icon_size = 8;
  this->radio_icon_size = 8;
  this->scrollbar_size = 12;
}

jstandard_theme::~jstandard_theme()
{
  for (int c=0; c<ICONS; c++) {
    if (icons_bitmap[c]) {
      destroy_bitmap(icons_bitmap[c]);
      icons_bitmap[c] = NULL;
    }
  }
}

void jstandard_theme::regen()
{
  int c, cmap[8], mask_cmap[2];

  this->desktop_color = COLOR_DISABLED;
  this->textbox_fg_color = COLOR_FOREGROUND;
  this->textbox_bg_color = COLOR_BACKGROUND;

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
      destroy_bitmap(icons_bitmap[c]);

    if (icons_data[c].mask)
      icons_bitmap[c] = data2bmp(8, icons_data[c].data, mask_cmap);
    else
      icons_bitmap[c] = data2bmp(bitmap_color_depth(ji_screen),
				 icons_data[c].data, cmap);
  }
}

BITMAP *jstandard_theme::set_cursor(int type, int *focus_x, int *focus_y)
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
      case JI_CURSOR_FORBIDDEN:
      case JI_CURSOR_MOVE:
	*focus_x = 0;
	*focus_y = 0;
	break;
      case JI_CURSOR_HAND:
	*focus_x = 5;
	*focus_y = 3;
	break;
      case JI_CURSOR_SCROLL:
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
      case JI_CURSOR_EYEDROPPER:
	*focus_x = 0;
	*focus_y = 15;
	break;
    }
  }

  return sprite;
}

void jstandard_theme::init_widget(JWidget widget)
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
      (widget->type != JI_FRAME) &&
      (widget->type != JI_SEPARATOR))
    return;

  switch (widget->type) {

    case JI_BOX:
      BORDER(0);
      widget->child_spacing = 4;
      break;

    case JI_BUTTON:
      BORDER(4);
      widget->child_spacing = 0;
      break;

    case JI_CHECK:
      BORDER(2);
      widget->child_spacing = 4;
      break;

    case JI_ENTRY:
      BORDER(3);
      break;

    case JI_GRID:
      BORDER(0);
      widget->child_spacing = 4;
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

    case JI_COMBOBOX: {
      ComboBox* combobox = dynamic_cast<ComboBox*>(widget);
      if (combobox != NULL) {
	Widget* button = combobox->getButtonWidget();
	ji_generic_button_set_icon(button, icons_bitmap[ICON_COMBOBOX]);
      }
      break;
    }

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
      if ((widget->getAlign() & JI_HORIZONTAL) &&
	  (widget->getAlign() & JI_VERTICAL)) {
	BORDER(4);
      }
      /* horizontal bar */
      else if (widget->getAlign() & JI_HORIZONTAL) {
	BORDER4(2, 4, 2, 0);
      }
      /* vertical bar */
      else {
	BORDER4(4, 2, 0, 2);
      }

      if (widget->hasText()) {
	if (widget->getAlign() & JI_TOP)
	  widget->border_width.t = jwidget_get_text_height(widget);
	else if (widget->getAlign() & JI_BOTTOM)
	  widget->border_width.b = jwidget_get_text_height(widget);
      }
      break;

    case JI_SLIDER:
      BORDER(4);
      widget->child_spacing = jwidget_get_text_height(widget);
      widget->setAlign(JI_CENTER | JI_MIDDLE);
      break;

    case JI_TEXTBOX:
      BORDER(0);
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

    case JI_FRAME:
      if (!static_cast<Frame*>(widget)->is_desktop()) {
	if (widget->hasText()) {
	  BORDER4(6, 4+jwidget_get_text_height(widget)+6, 6, 6);
#if 1				/* add close button */
	  if (!(widget->flags & JI_INITIALIZED)) {
	    JWidget button = jbutton_new("x");
	    jbutton_set_bevel(button, 0, 0, 0, 0);
	    jwidget_add_hook(button, JI_WIDGET,
			     theme_button_msg_proc, NULL);
	    jwidget_decorative(button, true);
	    jwidget_add_child(widget, button);
	    button->setName("theme_close_button");
	  }
#endif
	}
	else if (!(widget->flags & JI_INITIALIZED)) {
	  BORDER(3);
	}
      }
      else {
	BORDER(0);
      }
      widget->child_spacing = 4;
      break;

    default:
      break;
  }
}

JRegion jstandard_theme::get_window_mask(JWidget widget)
{
  return jregion_new(widget->rc, 1);
}

void jstandard_theme::map_decorative_widget(JWidget widget)
{
  if (widget->name != NULL &&
      strcmp(widget->name, "theme_close_button") == 0) {
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

int jstandard_theme::color_foreground()
{
  return COLOR_FOREGROUND;
}

int jstandard_theme::color_disabled()
{
  return COLOR_DISABLED;
}

int jstandard_theme::color_face()
{
  return COLOR_FACE;
}

int jstandard_theme::color_hotface()
{
  return COLOR_HOTFACE;
}

int jstandard_theme::color_selected()
{
  return COLOR_SELECTED;
}

int jstandard_theme::color_background()
{
  return COLOR_BACKGROUND;
}

void jstandard_theme::draw_box(JWidget widget, JRect clip)
{
  jdraw_rectfill(clip, get_bg_color(widget));
}

void jstandard_theme::draw_button(JWidget widget, JRect clip)
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
  if (widget->isEnabled() && widget->hasMouseOver())
    bg = COLOR_HOTFACE;
  /* without mouse */
  else
    bg = COLOR_FACE;

  /* selected */
  if (widget->isSelected()) {
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
  rectfill(ji_screen, x1, y1, x2, y2, get_bg_color(widget));

  /* get bevel info */
  jbutton_get_bevel(widget, bevel);

  /* 1st border */
  if (widget->hasFocus())
    draw_bevel_box(x1, y1, x2, y2,
		   COLOR_FOREGROUND, COLOR_FOREGROUND, bevel);
  else {
    less_bevel(bevel);
    draw_bevel_box(x1, y1, x2, y2, c1, c2, bevel);
  }

  less_bevel(bevel);

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (widget->hasFocus())
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
  draw_textstring(NULL, fg, bg, false, widget, crect, 1);
  jrect_free(crect);

  /* icon */
  if (icon_bmp) {
    if (widget->isSelected())
      jrect_displace(&icon, 1, 1);

    /* enabled */
    if (widget->isEnabled()) {
      /* selected */
      if (widget->isSelected()) {
	jdraw_inverted_sprite(ji_screen, icon_bmp, icon.x1, icon.y1);
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

void jstandard_theme::draw_check(JWidget widget, JRect clip)
{
  struct jrect box, text, icon;
  int bg;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			      ji_generic_button_get_icon_align(widget),
			      widget->theme->check_icon_size,
			      widget->theme->check_icon_size);

  /* background */
  jdraw_rectfill(widget->rc, bg = get_bg_color(widget));

  /* mouse */
  if (widget->isEnabled() && widget->hasMouseOver())
    jdraw_rectfill(&box, bg = COLOR_HOTFACE);

  /* focus */
  if (widget->hasFocus()) {
    jrect_stretch(&box, 1);
    jdraw_rect(&box, COLOR_FOREGROUND);
  }

  /* text */
  draw_textstring(NULL, -1, bg, false, widget, &text, 0);

  /* icon */
  draw_icons(icon.x1, icon.y1, widget, ICON_CHECK_EDGE);
}

void jstandard_theme::draw_grid(JWidget widget, JRect clip)
{
  jdraw_rectfill(clip, get_bg_color(widget));
}

void jstandard_theme::draw_entry(JWidget widget, JRect clip)
{
  bool password = jentry_is_password(widget);
  int scroll, cursor, state, selbeg, selend;
  const char *text = widget->getText();
  int c, ch, x, y, w, fg, bg;
  int x1, y1, x2, y2;
  int cursor_x;

  jtheme_entry_info(widget, &scroll, &cursor, &state, &selbeg, &selend);

  /* main pos */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2 - 1;
  y2 = widget->rc->y2 - 1;

  bg = COLOR_BACKGROUND;

  /* 1st border */
  jrectedge(ji_screen, x1, y1, x2, y2,
	    COLOR_DISABLED, COLOR_BACKGROUND);

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (widget->hasFocus())
    rect(ji_screen, x1, y1, x2, y2, COLOR_FOREGROUND);
  else
    rect(ji_screen, x1, y1, x2, y2, get_bg_color(widget));

  /* background border */
  x1++, y1++, x2--, y2--;
  rectfill(ji_screen, x1, y1, x2, y2, bg);

  /* draw the text */
  x = widget->rc->x1 + widget->border_width.l;
  y = (widget->rc->y1+widget->rc->y2)/2 - jwidget_get_text_height(widget)/2;

  for (c=scroll; ugetat(text, c); c++) {
    ch = password ? '*': ugetat(text, c);

    /* normal text */
    bg = -1;
    fg = COLOR_FOREGROUND;

    /* selected */
    if ((c >= selbeg) && (c <= selend)) {
      if (widget->hasFocus())
	bg = COLOR_SELECTED;
      else
	bg = COLOR_DISABLED;
      fg = COLOR_BACKGROUND;
    }

    /* disabled */
    if (!widget->isEnabled()) {
      bg = -1;
      fg = COLOR_DISABLED;
    }

    w = CHARACTER_LENGTH(widget->getFont(), ch);
    if (x+w > widget->rc->x2-3)
      return;

    cursor_x = x;
    ji_font_set_aa_mode(widget->getFont(), bg >= 0 ? bg: COLOR_BACKGROUND);
    widget->getFont()->vtable->render_char(widget->getFont(),
					   ch, fg, bg, ji_screen, x, y);
    x += w;

    /* cursor */
    if ((c == cursor) && (state) && (widget->hasFocus()))
      draw_entry_cursor(widget, cursor_x, y);
  }

  /* draw the cursor if it is next of the last character */
  if ((c == cursor) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled()))
    draw_entry_cursor(widget, x, y);
}

void jstandard_theme::draw_label(JWidget widget, JRect clip)
{
  int bg = get_bg_color(widget);

  jdraw_rectfill(widget->rc, bg);

  draw_textstring(NULL, -1, bg, false, widget, widget->rc, 0);
}

void jstandard_theme::draw_link_label(JWidget widget, JRect clip)
{
  int bg = get_bg_color(widget);

  jdraw_rectfill(widget->rc, bg);

  draw_textstring(NULL, makecol(0, 0, 255), bg, false, widget, widget->rc, 0);

  if (widget->hasMouseOver()) {
    int w = jwidget_get_text_length(widget);
    //int h = jwidget_get_text_height(widget);

    hline(ji_screen,
	  widget->rc->x1, widget->rc->y2-1, widget->rc->x1+w-1, makecol(0, 0, 255));
  }
}

void jstandard_theme::draw_listbox(JWidget widget, JRect clip)
{
  int bg;

  if (!widget->isEnabled())
    bg = COLOR_FACE;
  else
    bg = COLOR_BACKGROUND;

  jdraw_rectfill(widget->rc, COLOR_BACKGROUND);
}

void jstandard_theme::draw_listitem(JWidget widget, JRect clip)
{
  int fg, bg;
  int x, y;

  if (!widget->isEnabled()) {
    bg = COLOR_FACE;
    fg = COLOR_DISABLED;
  }
  else if (widget->isSelected()) {
    bg = COLOR_SELECTED;
    fg = COLOR_BACKGROUND;
  }
  else {
    bg = COLOR_BACKGROUND;
    fg = COLOR_FOREGROUND;
  }

  x = widget->rc->x1+widget->border_width.l;
  y = widget->rc->y1+widget->border_width.t;

  if (widget->hasText()) {
    /* text */
    jdraw_text(ji_screen, widget->getFont(), widget->getText(), x, y, fg, bg, true);

    /* background */
    jrectexclude
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

void jstandard_theme::draw_menu(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, get_bg_color(widget));
}

void jstandard_theme::draw_menuitem(JWidget widget, JRect clip)
{
  int c, bg, fg, bar;
  int x1, y1, x2, y2;
  JRect pos;

  /* TODO ASSERT? */
  if (!widget->parent->parent)
    return;

  bar = (widget->parent->parent->type == JI_MENUBAR);

  /* colors */
  if (!widget->isEnabled()) {
    bg = get_bg_color(widget);
    fg = -1;
  }
  else {
    if (jmenuitem_is_highlight(widget)) {
      bg = COLOR_SELECTED;
      fg = COLOR_BACKGROUND;
    }
    else if (widget->hasMouse()) {
      bg = COLOR_HOTFACE;
      fg = COLOR_FOREGROUND;
    }
    else {
      bg = get_bg_color(widget);
      fg = COLOR_FOREGROUND;
    }
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* background */
  rectfill(ji_screen, x1, y1, x2, y2, bg);

  // Draw an indicator for selected items
  if (widget->isSelected()) {
    BITMAP *icon = icons_bitmap[ICON_MENU_MARK];
    int x = widget->rc->x1+4-icon->w/2;
    int y = (widget->rc->y1+widget->rc->y2)/2-icon->h/2;

    if (widget->isEnabled())
      draw_character(ji_screen, icon, x, y, fg);
    else {
      draw_character(ji_screen, icon, x+1, y+1, COLOR_BACKGROUND);
      draw_character(ji_screen, icon, x, y, COLOR_DISABLED);
    }
  }

  /* text */
  if (bar)
    widget->setAlign(JI_CENTER | JI_MIDDLE);
  else
    widget->setAlign(JI_LEFT | JI_MIDDLE);

  pos = jwidget_get_rect(widget);
  if (!bar)
    jrect_displace(pos, widget->child_spacing/2, 0);
  draw_textstring(NULL, fg, bg, false, widget, pos, 0);
  jrect_free(pos);

  /* for menu-box */
  if (!bar) {
    /* draw the arrown (to indicate which this menu has a sub-menu) */
    if (jmenuitem_get_submenu(widget)) {
      /* enabled */
      if (widget->isEnabled()) {
	for (c=0; c<3; c++)
	  vline(ji_screen,
		widget->rc->x2-3-c,
		(widget->rc->y1+widget->rc->y2)/2-c,
		(widget->rc->y1+widget->rc->y2)/2+c, fg);
      }
      /* disabled */
      else {
	for (c=0; c<3; c++)
	  vline(ji_screen,
		widget->rc->x2-3-c+1,
		(widget->rc->y1+widget->rc->y2)/2-c+1,
		(widget->rc->y1+widget->rc->y2)/2+c+1, COLOR_BACKGROUND);
	for (c=0; c<3; c++)
	  vline(ji_screen,
		widget->rc->x2-3-c,
		(widget->rc->y1+widget->rc->y2)/2-c,
		(widget->rc->y1+widget->rc->y2)/2+c, COLOR_DISABLED);
      }
    }
    /* draw the keyboard shortcut */
    else if (jmenuitem_get_accel(widget)) {
      int old_align = widget->getAlign();
      char buf[256];

      pos = jwidget_get_rect(widget);
      pos->x2 -= widget->child_spacing/4;

      jaccel_to_string(jmenuitem_get_accel(widget), buf);

      widget->setAlign(JI_RIGHT | JI_MIDDLE);
      draw_textstring(buf, fg, bg, false, widget, pos, 0);
      widget->setAlign(old_align);

      jrect_free(pos);
    }
  }
}

void jstandard_theme::draw_panel(JWidget widget, JRect clip)
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

      if (widget->getAlign() & JI_HORIZONTAL) {
/* 	vline(ji_screen, */
/* 	       (c1->pos->x+c1->pos->w+c2->pos->x-1)/2, */
/* 	       widget->rect->y, */
/* 	       widget->rect->y+widget->rect->h/2-4, COLOR_FOREGROUND); */

/* 	vline(ji_screen, */
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
/* 	hline(ji_screen, */
/* 	       widget->rect->x, */
/* 	       (c1->pos->y+c1->pos->h+c2->pos->y-1)/2, */
/* 	       widget->rect->x+widget->rect->w/2-4, COLOR_FOREGROUND); */

/* 	hline(ji_screen, */
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

void jstandard_theme::draw_radio(JWidget widget, JRect clip)
{
  struct jrect box, text, icon;
  int bg = get_bg_color(widget);

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			    ji_generic_button_get_icon_align(widget),
			    widget->theme->radio_icon_size,
			    widget->theme->radio_icon_size);

  /* background */
  jdraw_rectfill(widget->rc, bg);

  /* mouse */
  if (widget->isEnabled() && widget->hasMouse())
    jdraw_rectfill(&box, bg = COLOR_HOTFACE);

  /* focus */
  if (widget->hasFocus())
    rect(ji_screen, box.x1-1, box.y1-1, box.x2, box.y2,
	 COLOR_FOREGROUND);

  /* text */
  draw_textstring(NULL, -1, bg, false, widget, &text, 0);

  /* icon */
  draw_icons(icon.x1, icon.y1, widget, ICON_RADIO_EDGE);
}

void jstandard_theme::draw_separator(JWidget widget, JRect clip)
{
  int x1, y1, x2, y2;

  /* frame position */
  x1 = widget->rc->x1 + widget->border_width.l/2;
  y1 = widget->rc->y1 + widget->border_width.t/2;
  x2 = widget->rc->x2 - 1 - widget->border_width.r/2;
  y2 = widget->rc->y2 - 1 - widget->border_width.b/2;

  /* background */
  jdraw_rectfill(widget->rc, get_bg_color(widget));

  /* TOP line */
  if (widget->getAlign() & JI_HORIZONTAL) {
    hline(ji_screen, x1, y1-1, x2, COLOR_DISABLED);
    hline(ji_screen, x1, y1, x2, COLOR_BACKGROUND);
  }

  /* LEFT line */
  if (widget->getAlign() & JI_VERTICAL) {
    vline(ji_screen, x1-1, y1, y2, COLOR_DISABLED);
    vline(ji_screen, x1, y1, y2, COLOR_BACKGROUND);
  }

  /* frame */
  if ((widget->getAlign() & JI_HORIZONTAL) &&
      (widget->getAlign() & JI_VERTICAL)) {
    /* union between the LEFT and TOP lines */
    putpixel(ji_screen, x1-1, y1-1, COLOR_DISABLED);

    /* BOTTOM line */
    hline(ji_screen, x1, y2, x2, COLOR_DISABLED);
    hline(ji_screen, x1-1, y2+1, x2, COLOR_BACKGROUND);

    /* RIGHT line */
    vline(ji_screen, x2, y1, y2, COLOR_DISABLED);
    vline(ji_screen, x2+1, y1-1, y2, COLOR_BACKGROUND);

    /* union between the RIGHT and BOTTOM lines */
    putpixel(ji_screen, x2+1, y2+1, COLOR_BACKGROUND);
  }

  /* text */
  if (widget->hasText()) {
    int h = jwidget_get_text_height(widget);
    struct jrect r = { x1+h/2, y1-h/2, x2+1-h, y2+1+h };
    draw_textstring(NULL, -1, get_bg_color(widget), false, widget, &r, 0);
  }
}

static bool my_add_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
  int u1 = MAX(x1, bitmap->cl);
  int v1 = MAX(y1, bitmap->ct);
  int u2 = MIN(x2, bitmap->cr-1);
  int v2 = MIN(y2, bitmap->cb-1);

  if (u1 > u2 || v1 > v2)
    return false;
  else
    set_clip_rect(bitmap, u1, v1, u2, v2);

  return true;
}

void jstandard_theme::draw_slider(JWidget widget, JRect clip)
{
  int x, x1, y1, x2, y2, bg, c1, c2;
  int min, max, value;
  char buf[256];

  jtheme_slider_info(widget, &min, &max, &value);

  /* main pos */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2 - 1;
  y2 = widget->rc->y2 - 1;

  /* with mouse */
  if (widget->isEnabled() && widget->hasMouse())
    bg = COLOR_HOTFACE;
  /* without mouse */
  else
    bg = COLOR_FACE;

  /* 1st border */
  jrectedge(ji_screen, x1, y1, x2, y2,
	    COLOR_DISABLED, COLOR_BACKGROUND);

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (widget->hasFocus())
    rect(ji_screen, x1, y1, x2, y2, COLOR_FOREGROUND);
  else
    rect(ji_screen, x1, y1, x2, y2, bg);

  /* 3rd border */
  if (!widget->isSelected()) {
    c1 = COLOR_BACKGROUND;
    c2 = COLOR_DISABLED;
  }
  else {
    c1 = COLOR_DISABLED;
    c2 = COLOR_BACKGROUND;
  }

  x1++, y1++, x2--, y2--;
  jrectedge(ji_screen, x1, y1, x2, y2, c1, c2);

  /* progress bar */
  x1++, y1++, x2--, y2--;

  if (min != max)
    x = x1 + (x2-x1) * (value-min) / (max-min);
  else
    x = x1;

  if (value == min) {
    rectfill(ji_screen, x1, y1, x2, y2, bg);
  }
  else {
    rectfill(ji_screen, x1, y1, x, y2,
	     (!widget->isEnabled()) ? bg: COLOR_SELECTED);

    if (x < x2)
      rectfill(ji_screen, x+1, y1, x2, y2, bg);
  }

  /* text */
  {
    std::string old_text = widget->getText();
    JRect r;
    int cx1, cy1, cx2, cy2;
    get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

    usprintf(buf, "%d", value);

    widget->setTextQuiet(buf);

    r = jrect_new(x1, y1, x2+1, y2+1);

    if (my_add_clip_rect(ji_screen, x1, y1, x, y2))
      draw_textstring(NULL, COLOR_BACKGROUND,
		      !widget->isEnabled() ?
		      bg: COLOR_SELECTED, false, widget, r, 0);

    set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);

    if (my_add_clip_rect(ji_screen, x+1, y1, x2, y2))
      draw_textstring(NULL, COLOR_FOREGROUND, bg, false, widget, r, 0);

    set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);

    widget->setTextQuiet(old_text.c_str());
    jrect_free(r);
  }
}

void jstandard_theme::draw_combobox_entry(JWidget widget, JRect clip)
{
  draw_entry(widget, clip);
}

void jstandard_theme::draw_combobox_button(JWidget widget, JRect clip)
{
  draw_button(widget, clip);
}

void jstandard_theme::draw_textbox(JWidget widget, JRect clip)
{
  _ji_theme_textbox_draw(ji_screen, widget, NULL, NULL,
			 widget->theme->textbox_bg_color,
			 widget->theme->textbox_fg_color);
}

void jstandard_theme::draw_view(JWidget widget, JRect clip)
{
  JRect pos = jwidget_get_rect(widget);

  if (widget->hasFocus()) {
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
    jdraw_rect(pos, get_bg_color(widget));
  }

  /* background */
  jrect_shrink(pos, 1);
  jdraw_rectfill(pos, get_bg_color(widget));

  jrect_free(pos);
}

void jstandard_theme::draw_view_scrollbar(JWidget widget, JRect clip)
{
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int pos, len;

  jtheme_scrollbar_info(widget, &pos, &len);

  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* border */
  rect(ji_screen, x1, y1, x2, y2, get_bg_color(widget));

  /* draw the content */
  x1++, y1++, x2--, y2--;

  /* horizontal bar */
  if (widget->getAlign() & JI_HORIZONTAL) {
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
  jrectexclude(ji_screen,
	       x1, y1, x2, y2,
	       u1, v1, u2, v2, get_bg_color(widget));

  /* 1st border */
  if (widget->isSelected())
    jrectedge(ji_screen, u1, v1, u2, v2,
	      COLOR_DISABLED, COLOR_BACKGROUND);
  else
    jrectedge(ji_screen, u1, v1, u2, v2,
	      COLOR_BACKGROUND, COLOR_DISABLED);

  /* bar-block background */
  u1++, v1++, u2--, v2--;
  if (widget->isEnabled() && widget->hasMouse())
    rectfill(ji_screen, u1, v1, u2, v2, COLOR_HOTFACE);
  else
    rectfill(ji_screen, u1, v1, u2, v2, get_bg_color(widget));
}

void jstandard_theme::draw_view_viewport(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, get_bg_color(widget));
}

void jstandard_theme::draw_frame(Frame* frame, JRect clip)
{
  JRect pos = jwidget_get_rect(frame);
  JRect cpos = jwidget_get_child_rect(frame);

  /* extra lines */
  if (!frame->is_desktop()) {
    // draw frame borders
    jdraw_rect(pos, COLOR_FOREGROUND);
    jrect_shrink(pos, 1);
    jdraw_rectedge(pos, COLOR_BACKGROUND, COLOR_DISABLED);
    jrect_shrink(pos, 1);
    jdraw_rectfill(pos, get_bg_color(frame));

    // draw title bar
    if (frame->hasText()) {
      int bg = COLOR_SELECTED;

      jrect_shrink(pos, 1);
/*       pos->y2 = cpos->y1-1; */
      pos->y2 = cpos->y1-3;
      jdraw_rectfill(pos, bg);

      jrect_stretch(pos, 1);
      jdraw_rectedge(cpos, COLOR_DISABLED, COLOR_BACKGROUND);

      jdraw_text(ji_screen, frame->getFont(), frame->getText(),
		 cpos->x1,
		 pos->y1+jrect_h(pos)/2-text_height(frame->getFont())/2,
		 COLOR_BACKGROUND, bg, false);
    }
  }
  /* desktop */
  else {
    jdraw_rectfill(pos, frame->theme->desktop_color);
  }

  jrect_free(pos);
  jrect_free(cpos);
}

int jstandard_theme::get_bg_color(JWidget widget)
{
  int c = jwidget_get_bg_color(widget);
  int decorative = jwidget_is_decorative(widget);

  return c >= 0 ? c: (decorative ? COLOR_SELECTED:
				   COLOR_FACE);
}

void jstandard_theme::draw_textstring(const char *t, int fg_color, int bg_color,
				      bool fill_bg, JWidget widget, const JRect rect,
				      int selected_offset)
{
  if (t || widget->hasText()) {
    int x, y, w, h;

    if (!t) {
      t = widget->getText();
      w = jwidget_get_text_length(widget);
      h = jwidget_get_text_height(widget);
    }
    else {
      w = ji_font_text_len(widget->getFont(), t);
      h = text_height(widget->getFont());
    }

    /* horizontally text alignment */

    if (widget->getAlign() & JI_RIGHT)
      x = rect->x2 - w;
    else if (widget->getAlign() & JI_CENTER)
      x = (rect->x1+rect->x2)/2 - w/2;
    else
      x = rect->x1;

    /* vertically text alignment */

    if (widget->getAlign() & JI_BOTTOM)
      y = rect->y2 - h;
    else if (widget->getAlign() & JI_MIDDLE)
      y = (rect->y1+rect->y2)/2 - h/2;
    else
      y = rect->y1;

    if (widget->isSelected()) {
      x += selected_offset;
      y += selected_offset;
    }

    /* background */
    if (bg_color >= 0) {
      if (!widget->isEnabled())
	rectfill(ji_screen, x, y, x+w, y+h, bg_color);
      else
	rectfill(ji_screen, x, y, x+w-1, y+h-1, bg_color);
    }

    /* text */
    if (!widget->isEnabled()) {
      /* TODO avoid this */
      if (fill_bg)		/* only to draw the background */
	jdraw_text(ji_screen, widget->getFont(), t, x, y, 0, bg_color, fill_bg);

      /* draw white part */
      jdraw_text(ji_screen, widget->getFont(), t, x+1, y+1,
		 COLOR_BACKGROUND, bg_color, fill_bg);

      if (fill_bg)
	fill_bg = false;
    }

    jdraw_text(ji_screen, widget->getFont(), t, x, y,
	       !widget->isEnabled() ?
	       COLOR_DISABLED: (fg_color >= 0 ? fg_color :
						COLOR_FOREGROUND),
	       bg_color, fill_bg);
  }
}

void jstandard_theme::draw_entry_cursor(JWidget widget, int x, int y)
{
  int h = jwidget_get_text_height(widget);

  vline(ji_screen, x,   y-1, y+h, COLOR_FOREGROUND);
  vline(ji_screen, x+1, y-1, y+h, COLOR_FOREGROUND);
}

void jstandard_theme::draw_icons(int x, int y, JWidget widget, int edge_icon)
{
  draw_sprite(ji_screen, icons_bitmap[edge_icon], x, y);

  if (widget->isSelected())
    draw_sprite(ji_screen, icons_bitmap[edge_icon+1], x, y);
}

void jstandard_theme::draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel)
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

void jstandard_theme::less_bevel(int *bevel)
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
	widget->setSelected(true);
	return true;
      }
      break;

    case JM_KEYRELEASED:
      if (msg->key.scancode == KEY_ESC) {
	if (widget->isSelected()) {
	  widget->setSelected(false);
	  jwidget_close_window(widget);
	  return true;
	}
      }
      break;
  }

  return false;
}

