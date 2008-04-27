/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
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

#ifndef JINETE_WIDGET_H
#define JINETE_WIDGET_H

#include "jinete/jbase.h"

#ifndef NDEBUG
#include "jinete/jintern.h"
#define assert_valid_widget(widget) assert((widget) != NULL &&		\
					   _ji_is_valid_widget((widget)))
#else
#define assert_valid_widget(widget) ((void)0)
#endif

JI_BEGIN_DECLS

struct FONT;
struct BITMAP;

struct jwidget
{
  JID id;			/* identify code */
  int type;			/* widget's type */

  char *name;			/* widget's name */
  JRect rc;			/* position rectangle */
  struct {
    int l, t, r, b;
  } border_width;		/* border separation with the parent */
  int child_spacing;		/* separation between children */

  /* flags */
  int flags;
  int emit_signals;		/* emit signal counter */

  /* widget size limits */
  int min_w, min_h;
  int max_w, max_h;

  /* structures */
  JList children;		 /* sub-objects */
  JWidget parent;		 /* who is the parent? */
  JTheme theme;			 /* widget's theme */

  /* virtual properties */
  JList hooks;			/* hooks with msg_proc and specific data */
  int draw_type;
  JDrawFunc draw_method;	/* virtual method to draw the widget
				   (the default msg_proc uses it) */

  /* common widget properties */
  int align;			/* widget alignment */
  int text_size;		/* text size (in characters) */
  char *text;			/* widget text */
  int text_size_pix;		/* cached text size in pixels */
  struct FONT *text_font;	/* text font type */
  int bg_color;			/* background color */

  /* drawable cycle */
  JRegion update_region;	/* region to be redrawed */

  /* more properties... */

  /* for JTheme */
  void *theme_data[4];

  /* for user */
  void *user_data[4];
};

int ji_register_widget_type(void);

JWidget jwidget_new(int type);
void jwidget_free(JWidget widget);
void jwidget_free_deferred(JWidget widget);

void jwidget_init_theme(JWidget widget);

/* hooks */

void jwidget_add_hook(JWidget widget, int type,
		      JMessageFunc msg_proc, void *data);
JHook jwidget_get_hook(JWidget widget, int type);
void *jwidget_get_data(JWidget widget, int type);

/* main properties */

int jwidget_get_type(JWidget widget);
const char *jwidget_get_name(JWidget widget);
const char *jwidget_get_text(JWidget widget);
int jwidget_get_align(JWidget widget);
struct FONT *jwidget_get_font(JWidget widget);

void jwidget_set_name(JWidget widget, const char *name);
void jwidget_set_text(JWidget widget, const char *text);
void jwidget_set_text_soft(JWidget widget, const char *text);
void jwidget_set_align(JWidget widget, int align);
void jwidget_set_font(JWidget widget, struct FONT *font);
 
/* behavior properties */

void jwidget_magnetic(JWidget widget, bool state);
void jwidget_expansive(JWidget widget, bool state);
void jwidget_decorative(JWidget widget, bool state);
void jwidget_focusrest(JWidget widget, bool state);

bool jwidget_is_magnetic(JWidget widget);
bool jwidget_is_expansive(JWidget widget);
bool jwidget_is_decorative(JWidget widget);
bool jwidget_is_focusrest(JWidget widget);

/* status properties */

void jwidget_dirty(JWidget widget);
void jwidget_show(JWidget widget);
void jwidget_hide(JWidget widget);
void jwidget_enable(JWidget widget);
void jwidget_disable(JWidget widget);
void jwidget_select(JWidget widget);
void jwidget_deselect(JWidget widget);

bool jwidget_is_visible(JWidget widget);
bool jwidget_is_hidden(JWidget widget);
bool jwidget_is_enabled(JWidget widget);
bool jwidget_is_disabled(JWidget widget);
bool jwidget_is_selected(JWidget widget);
bool jwidget_is_deselected(JWidget widget);

/* properties with manager */

bool jwidget_has_focus(JWidget widget);
bool jwidget_has_mouse(JWidget widget);
bool jwidget_has_capture(JWidget widget);

/* children handle */

void jwidget_add_child(JWidget widget, JWidget child);
void jwidget_add_children(JWidget widget, ...);
void jwidget_remove_child(JWidget widget, JWidget child);
void jwidget_replace_child(JWidget widget, JWidget old_child,
			   JWidget new_child);

/* parents and children */

JWidget jwidget_get_parent(JWidget widget);
JWidget jwidget_get_window(JWidget widget);
JWidget jwidget_get_manager(JWidget window);
JList jwidget_get_parents(JWidget widget, bool ascendant);
JList jwidget_get_children(JWidget widget);
JWidget jwidget_pick(JWidget widget, int x, int y);
bool jwidget_has_child(JWidget widget, JWidget child);

/* position and geometry */

void jwidget_request_size(JWidget widget, int *w, int *h);
void jwidget_relayout(JWidget widget);
JRect jwidget_get_rect(JWidget widget);
JRect jwidget_get_child_rect(JWidget widget);
JRegion jwidget_get_region(JWidget widget);
JRegion jwidget_get_drawable_region(JWidget widget, int flags);
int jwidget_get_bg_color(JWidget widget);
JTheme jwidget_get_theme(JWidget widget);
int jwidget_get_text_length(JWidget widget);
int jwidget_get_text_height(JWidget widget);
void jwidget_get_texticon_info(JWidget widget,
			       JRect box, JRect text, JRect icon,
			       int icon_align, int icon_w, int icon_h);

void jwidget_noborders(JWidget widget);
void jwidget_set_border(JWidget widget, int l, int t, int r, int b);
void jwidget_set_rect(JWidget widget, JRect rect);
void jwidget_set_min_size(JWidget widget, int w, int h);
void jwidget_set_max_size(JWidget widget, int w, int h);
void jwidget_set_bg_color(JWidget widget, int color);
void jwidget_set_theme(JWidget widget, JTheme theme);

/* drawing methods */

void jwidget_flush_redraw(JWidget widget);
void jwidget_redraw_region(JWidget widget, const JRegion region);
void jwidget_invalidate(JWidget widget);
void jwidget_invalidate_rect(JWidget widget, const JRect rect);
void jwidget_invalidate_region(JWidget widget, const JRegion region);
void jwidget_scroll(JWidget widget, JRegion region, int dx, int dy);

/* signal handle */

void jwidget_signal_on(JWidget widget);
void jwidget_signal_off(JWidget widget);

int jwidget_emit_signal(JWidget widget, int signal_num);

/* manager handler */

bool jwidget_send_message(JWidget widget, JMessage msg);
bool jwidget_send_message_after_type(JWidget widget, JMessage msg, int type);
void jwidget_close_window(JWidget widget);
void jwidget_capture_mouse(JWidget widget);
void jwidget_hard_capture_mouse(JWidget widget);
void jwidget_release_mouse(JWidget widget);

/* miscellaneous */

JWidget jwidget_find_name(JWidget widget, const char *name);
bool jwidget_check_underscored(JWidget widget, int scancode);

JI_END_DECLS

#endif /* JINETE_WIDGET_H */
