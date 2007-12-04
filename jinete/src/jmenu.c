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

#include <allegro/gfx.h>
#include <allegro/keyboard.h>
#include <ctype.h>
#include <stdio.h>

#include "jinete.h"

/* internal messages: to move between menus */
#define DO_GOTOBAR		1
#define DO_GOTODEST		2

#define MOUSE_IN(pos)						\
  ((jmouse_x(0) >= pos->x1) && (jmouse_x(0) < pos->x2) &&	\
   (jmouse_y(0) >= pos->y1) && (jmouse_y(0) < pos->y2))

#define MBOX(widget)						\
  ((MenuBox *)jwidget_get_data(((JWidget)widget), JI_MENUBOX))

#define MITEM(widget)						\
  ((MenuItem *)jwidget_get_data(((JWidget)widget), JI_MENUITEM))

/* JWidget *menuitem */
#define HAS_SUBMENU(menuitem)					\
  ((MITEM(menuitem)->submenu) &&				\
   (!jlist_empty(MITEM(menuitem)->submenu->children)))

/* MenuBox *menubox_data */
#define HAS_MENU(menubox_data)					\
  ((menubox_data->menu) && (!jlist_empty(menubox_data->menu->children)))

typedef struct MenuBox
{
  JWidget menu;
  JWidget parent_menuitem;
} MenuBox;

typedef struct MenuItem
{
  JAccel accel;			/* hot-key */
  bool highlight : 1;		/* is highlighted? */
  bool opened : 1;		/* is sub-menu of this menu-item opened? */
  JWidget submenu;		/* the sub-menu */
} MenuItem;

static int internal_msg = 0;
static JWidget dest_menubox = NULL;
static JWidget dest_menuitem = NULL;
static JWidget emitter_menuitem = NULL;
static bool dest_selectfirst = FALSE;
static bool was_clicked = FALSE;
static int current_level = 0;

static bool menu_msg_proc(JWidget widget, JMessage msg);
static void menu_request_size(JWidget widget, int *w, int *h);
static void menu_set_position(JWidget widget, JRect rect);

static bool menubox_msg_proc(JWidget widget, JMessage msg);
static void menubox_request_size(JWidget widget, int *w, int *h);
static void menubox_set_position(JWidget widget, JRect rect);

static bool menuitem_msg_proc(JWidget widget, JMessage msg);
static void menuitem_request_size(JWidget widget, int *w, int *h);

static JWidget get_highlight(JWidget menu);
static void set_highlight(JWidget menu, JWidget menuitem);
static void unhighlight(JWidget menu);
static void open_menuitem(JWidget menuitem, bool select_first);
static JWidget check_for_letter(JWidget menu, int ascii);
static JWidget check_for_accel(JWidget menu, JMessage msg);
static JWidget pick_menuitem(JWidget *menubox, JWidget *open_menubox);
static void emit_signal(JWidget menuitem);

static JWidget find_nextitem (JWidget menu, JWidget menuitem);
static JWidget find_previtem (JWidget menu, JWidget menuitem);

JWidget jmenu_new(void)
{
  JWidget widget = jwidget_new(JI_MENU);

  jwidget_add_hook(widget, JI_MENU, menu_msg_proc, NULL);
  jwidget_init_theme(widget);

  return widget;
}

JWidget jmenubar_new(void)
{
  JWidget widget = jmenubox_new();

  widget->type = JI_MENUBAR;

  jwidget_init_theme(widget);

  return widget;
}

JWidget jmenubox_new(void)
{
  JWidget widget = jwidget_new(JI_MENUBOX);
  MenuBox *menubox = jnew(MenuBox, 1);

  menubox->menu = NULL;
  menubox->parent_menuitem = NULL;

  jwidget_add_hook(widget, JI_MENUBOX, menubox_msg_proc, menubox);
  jwidget_focusrest(widget, TRUE);
  jwidget_init_theme(widget);

  return widget;
}

JWidget jmenuitem_new(const char *text)
{
  JWidget widget = jwidget_new(JI_MENUITEM);
  MenuItem *menuitem = jnew(MenuItem, 1);

  menuitem->accel = NULL;
  menuitem->highlight = FALSE;
  menuitem->opened = FALSE;
  menuitem->submenu = NULL;

  jwidget_add_hook(widget, JI_MENUITEM, menuitem_msg_proc, menuitem);
  jwidget_set_text(widget, text);
  jwidget_init_theme(widget);

  return widget;
}

JWidget jmenubox_get_menu(JWidget widget)
{
  MenuBox *menubox = MBOX(widget);

  return menubox->menu ? menubox->menu: NULL;
}

JWidget jmenubar_get_menu(JWidget widget)
{
  return jmenubox_get_menu(widget);
}

JWidget jmenuitem_get_submenu(JWidget widget)
{
  MenuItem *menuitem = MITEM(widget);

  return menuitem->submenu ? menuitem->submenu: NULL;
}

JAccel jmenuitem_get_accel(JWidget widget)
{
  MenuItem *menuitem = MITEM(widget);

  return menuitem->accel;
}

void jmenubox_set_menu(JWidget widget, JWidget widget_menu)
{
  MenuBox *menubox = MBOX(widget);

  if (menubox->menu)
    jwidget_remove_child(widget, menubox->menu);

  menubox->menu = widget_menu;

  if (menubox->menu)
    jwidget_add_child(widget, menubox->menu);
}

void jmenubar_set_menu(JWidget widget, JWidget widget_menu)
{
  jmenubox_set_menu(widget, widget_menu);
}

void jmenuitem_set_submenu(JWidget widget, JWidget widget_menu)
{
  MenuItem *menuitem = MITEM (widget);

  menuitem->submenu = widget_menu;
}

/**
 * Changes the keyboard shortcuts (accelerators) for the specified
 * widget (a menu-item).
 *
 * @warning The specified @a accel will be freed automatically when
 *          the menu-item'll receive JM_DESTROY message.
 */
void jmenuitem_set_accel(JWidget widget, JAccel accel)
{
  MenuItem *menuitem = MITEM (widget);

  if (menuitem->accel)
    jaccel_free(menuitem->accel);

  menuitem->accel = accel;
}

int jmenuitem_is_highlight(JWidget widget)
{
  MenuItem *menuitem = MITEM (widget);

  return menuitem->highlight;
}

void jmenu_popup(JWidget menu, int x, int y)
{
  JWidget window, menubox;

  do {
    jmouse_poll();
  } while (jmouse_b(0));

  was_clicked = TRUE;
  current_level = 1;
  internal_msg = 0;
  emitter_menuitem = NULL;

  /* new window and new menu-box */
  window = jwindow_new(NULL);
  menubox = jmenubox_new();

  jwindow_moveable(window, FALSE);	 /* can't move the window */
  MBOX(menubox)->parent_menuitem = NULL; /* without parent */

  /* set children */
  jmenubox_set_menu(menubox, menu);
  jwidget_add_child(window, menubox);

  jwindow_remap(window);

  /* menubox position */
  jwindow_position(window,
		   MID(0, x, JI_SCREEN_W-jrect_w(window->rc)),
		   MID(0, y, JI_SCREEN_H-jrect_h(window->rc)));

  /* set the focus to the new menubox */
/*   jmanager_set_focus(menubox); */
  jwidget_magnetic(menubox, TRUE);

  /* open the window */
  jwindow_open_fg(window);

  jmanager_free_focus();

  /* fetch the "menu" */
  jmenubox_set_menu(menubox, NULL);

  /* destroy the window */
  jwidget_free(window);

  was_clicked = FALSE;
  current_level = 0;
  internal_msg = 0;

  /* emit signal */
  if(emitter_menuitem)
    emit_signal(emitter_menuitem);
}

static bool menu_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      menu_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_SETPOS:
      menu_set_position(widget, &msg->setpos.rect);
      return TRUE;
  }

  return FALSE;
}

static void menu_request_size(JWidget widget, int *w, int *h)
{
  int req_w, req_h;
  JLink link;

  *w = *h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    jwidget_request_size((JWidget)link->data, &req_w, &req_h);

    if (widget->parent->type == JI_MENUBAR) {
      *w += req_w + ((link->next != widget->children->end) ?
		     widget->child_spacing: 0);
      *h = MAX (*h, req_h);
    }
    else {
      *w = MAX (*w, req_w);
      *h += req_h + ((link->next != widget->children->end) ?
		     widget->child_spacing: 0);
    }
  }

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void menu_set_position(JWidget widget, JRect rect)
{
  int req_w, req_h;
  JWidget child;
  JRect cpos;
  JLink link;

  jrect_copy(widget->rc, rect);
  cpos = jwidget_get_child_rect(widget);

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    jwidget_request_size(child, &req_w, &req_h);

    if (widget->parent->type == JI_MENUBAR)
      cpos->x2 = cpos->x1+req_w;
    else
      cpos->y2 = cpos->y1+req_h;

    jwidget_set_rect(child, cpos);

    if (widget->parent->type == JI_MENUBAR)
      cpos->x1 += jrect_w(cpos);
    else
      cpos->y1 += jrect_h(cpos);
  }

  jrect_free(cpos);
}

static bool menubox_msg_proc(JWidget widget, JMessage msg)
{
  MenuBox *menubox = MBOX(widget);
  int open_immediatly = FALSE;

  switch (msg->type) {

    case JM_DESTROY:
      jfree(menubox);
      break;

    case JM_REQSIZE:
      menubox_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_SETPOS:
      menubox_set_position(widget, &msg->setpos.rect);
      return TRUE;

    case JM_MOTION:
      /* isn't pressing a button? */
      if (!msg->mouse.flags && !was_clicked)
	break;

    case JM_BUTTONPRESSED:
      if (!(internal_msg & (DO_GOTOBAR | DO_GOTODEST)))
	open_immediatly = TRUE;

    case JM_FOCUSENTER:
      if (jmanager_get_capture() &&
	  jmanager_get_capture() != widget)
	break;

      if (HAS_MENU(menubox)) {
	JWidget picked;

	/* in mouse-motion or button-pressed */
	if (msg->type != JM_FOCUSENTER) {
	  picked = jwidget_pick(menubox->menu,
				  msg->mouse.x, msg->mouse.y);
	}
	/* when focus enter */
	else {
	  /* in menu-bar */
	  if (widget->type == JI_MENUBAR) {
	    /* there isn't highlight? */
	    if (!get_highlight(menubox->menu)) {
	      /* highlight the first menu-item */
	      picked = (JWidget)jlist_first(menubox->menu->children)->data;
	    }
	    else
	      return FALSE; /* we don't need to change the highlight */
	  }
	  else
	    return FALSE;
	}

	if (picked) {
	  if (!(picked->flags & JI_DISABLED) &&
	      picked->type == JI_MENUITEM) {
	    set_highlight(menubox->menu, picked);

	    if (open_immediatly) {
	      was_clicked = TRUE;
	      open_menuitem(picked, FALSE);
	    }
	  }
	  else {
	    unhighlight(menubox->menu);
	  }
	}
      }
      break;

    case JM_FOCUSLEAVE:
      if (jmanager_get_capture() &&
	  jmanager_get_capture() != widget)
	break;

      if (HAS_MENU(menubox)) {
	JWidget highlight = get_highlight(menubox->menu);

	if ((highlight) && (!MITEM(highlight)->opened))
	  unhighlight(menubox->menu);
      }
      break;

    case JM_MOUSELEAVE:
      if (jmanager_get_capture() &&
	  jmanager_get_capture() != widget)
	break;

      if (HAS_MENU(menubox)) {
	JWidget highlight = get_highlight(menubox->menu);

	if ((highlight) && (!MITEM(highlight)->opened))
	  unhighlight(menubox->menu);
      }
      break;

    case JM_BUTTONRELEASED:
      if (jmanager_get_capture() &&
	  jmanager_get_capture() != widget)
	break;

      if (HAS_MENU(menubox)) {
	JWidget highlight = get_highlight(menubox->menu);

	if ((highlight) && (!MITEM(highlight)->opened)) {
	  unhighlight(menubox->menu);
	  emit_signal(highlight);
	  was_clicked = FALSE;

	  /* if (menubox->parent_menuitem) */
	    internal_msg |= DO_GOTOBAR;
	}
      }
      break;

    case JM_CHAR:
      if (jmanager_get_capture() &&
	  jmanager_get_capture() != widget)
	break;

      /* there is an internal message waiting? */
      if (internal_msg)
	break;

      was_clicked = FALSE;

      if (HAS_MENU(menubox)) {
	JWidget selected;

	/* check for ALT+some letter in menubar and some letter in menuboxes */
	if (((widget->type == JI_MENUBOX) && (!msg->any.shifts)) ||
	    ((widget->type == JI_MENUBAR) && (msg->any.shifts & KB_ALT_FLAG))) {
	  selected = check_for_letter(menubox->menu,
				      scancode_to_ascii(msg->key.scancode));
	  if (selected) {
	    set_highlight(menubox->menu, selected);

	    if (HAS_SUBMENU(selected))
	      open_menuitem(selected, TRUE);
	    else {
	      emit_signal(selected);

	      /* if (menubox->parent_menuitem) */
		internal_msg |= DO_GOTOBAR;
	    }
	    return TRUE;
	  }
	}

	/* only in menu-bars */
	if (widget->type == JI_MENUBAR) {
	  /* check for accelerators */
	  selected = check_for_accel (menubox->menu, msg);
	  if (selected) {
	    emit_signal (selected);

	    /* if this is not the main menu, go to bar (this happend if
	       we press a keyboard shortcut when we have open a
	       sub-menu) */
	    /* if (menubox->parent_menuitem) */
	      internal_msg |= DO_GOTOBAR;

	    return TRUE;
	  }
	}

	/* highlight movement with keyboard */
	if (jwidget_has_focus (widget)) {
	  JWidget highlight = get_highlight (menubox->menu);
	  int open_item = FALSE;

	  switch (msg->key.scancode) {

	    case KEY_ESC:
              /* in menu-bar */
	      if (widget->type == JI_MENUBAR) {
		/* fetch the focus */
		jmanager_free_focus ();
		return TRUE;
              }
              /* in menu-boxes */
              else {
                /* go to parent */
                if (menubox->parent_menuitem) {
		  /* just retrogress one parent-level */
		  jwidget_close_window (widget);
		}
              }
	      break;

	    case KEY_UP:
              /* in menu-bar */
	      if (widget->type == JI_MENUBAR) {
                /* do nothing */
              }
              /* in menu-boxes */
              else {
                /* go to previous */
                highlight = find_previtem (menubox->menu, highlight);
              }
	      break;

            case KEY_DOWN:
              /* in menu-bar */
	      if (widget->type == JI_MENUBAR) {
                /* select the active menu */
		open_item = TRUE;
              }
              /* in menu-boxes */
              else {
                /* go to next */
                highlight = find_nextitem (menubox->menu, highlight);
              }
	      break;

	    case KEY_LEFT:
              /* in menu-bar */
	      if (widget->type == JI_MENUBAR) {
                /* go to previous */
                highlight = find_previtem (menubox->menu, highlight);
              }
              /* in menu-boxes */
	      else {
                /* go to parent */
                if (menubox->parent_menuitem) {
                  JWidget menuitem;
                  JWidget parent = menubox->parent_menuitem->parent->parent;

                  /* go to the previous item in the parent */

                  /* if the parent is the menu-bar */
                  if (parent->type == JI_MENUBAR) {
                    menuitem = find_previtem
		      (MBOX (parent)->menu,
		       get_highlight (MBOX (parent)->menu));

                    /* go to previous item in the parent */
		    dest_menubox = parent;
		    dest_menuitem = menuitem;
		    dest_selectfirst = TRUE;

		    internal_msg |= DO_GOTODEST;
                  }
                  /* if the parent isn't the menu-bar */
                  else {
                    /* just retrogress one parent-level */
		    jwidget_close_window (widget);
		  }
                }
	      }
	      break;

	    case KEY_RIGHT:
              /* in menu-bar */
	      if (widget->type == JI_MENUBAR) {
                /* go to next */
                highlight = find_nextitem (menubox->menu, highlight);
              }
              /* in menu-boxes */
              else {
                /* enter in sub-menu */
                if ((highlight) && HAS_SUBMENU (highlight)) {
                  open_item = TRUE;
                }
                /* go to parent */
                else if (menubox->parent_menuitem) {
                  JWidget root, menuitem;

                  /* get the root menu */
                  for (root=widget;
		       MBOX (root)->parent_menuitem;
		       root=MBOX (root)->parent_menuitem->parent->parent);

                  /* go to the next item in the root */
                  menuitem = find_nextitem
		    (MBOX (root)->menu,
		     get_highlight (MBOX (root)->menu));

                  /* open the sub-menu */
		  dest_menubox = root;
		  dest_menuitem = menuitem;
		  dest_selectfirst = TRUE;

		  internal_msg |= DO_GOTODEST;
                }
              }
	      break;

	    case KEY_ENTER:
	    case KEY_ENTER_PAD:
	      open_item = TRUE;
	      break;

	    default:
	      return FALSE;
	  }

	  set_highlight(menubox->menu, highlight);

	  if (open_item && highlight) {
	    if (HAS_SUBMENU (highlight))
	      open_menuitem (highlight, TRUE);
	    else {
	      emit_signal (highlight);

	      /* if (menubox->parent_menuitem) */
		internal_msg |= DO_GOTOBAR;
	    }
	  }

	  return TRUE;
	}
      }
      break;

    case JM_IDLE:
      if (jmanager_get_capture () &&
	  jmanager_get_capture () != widget)
	break;

      /* goto destination */
      if (internal_msg & DO_GOTODEST) {
	if (dest_menubox == widget) {
	  dest_menubox = NULL;
	  internal_msg ^= DO_GOTODEST;

	  /* printf ("	ENTRY IN (%d)\n", widget->id); */

	  if (dest_menuitem) {
	    /* printf ("	OPEN MENU (%d)\n", dest_menuitem->id); */

	    set_highlight(menubox->menu, dest_menuitem);
	    open_menuitem(dest_menuitem, dest_selectfirst);
	  }
	  /********************************************************************************/
	}
	else {
	  /* printf ("	CLOSING (%d)\n", widget->id); */
	  jwidget_close_window (widget);
	}
	return TRUE;
      }

      /* a menubox created by open_menuitem() */
/*       if (menubox->parent_menuitem) { */
      if (widget->type == JI_MENUBAR) {
	/* goto bar */
	if (internal_msg & DO_GOTOBAR) {
	  internal_msg ^= DO_GOTOBAR;
	}
      }
      else if (widget->type == JI_MENUBOX) {
	/* goto bar */
	if (internal_msg & DO_GOTOBAR) {
	  jwidget_close_window (widget);
	  break;
	}

	/* mouse outside the box? */
	if (!MOUSE_IN (widget->rc)) {
	  /* control button press outside the menubox */
	  if (jmouse_b(0) || was_clicked) {
	    JWidget picked_menubox = widget;
	    JWidget open_menubox = NULL;
	    JWidget picked = pick_menuitem (&picked_menubox, &open_menubox);

/* 	    printf ("CLICK OUTSIDE (%d): %d %d %d\n", */
/* 		    widget->id, */
/* 		    picked ? picked->id: 0, */
/* 		    picked_menubox ? picked_menubox->id: 0, */
/* 		    open_menubox ? open_menubox->id: 0); */

	    if (picked && jwidget_is_enabled (picked)) {
/* 	      if (open_menubox != widget) { */
/* 		dest_menubox = open_menubox; */
/* 		dest_menuitem = NULL; */
/* 		dest_selectfirst = FALSE; */
/* 		was_clicked = TRUE; */

/* 		internal_msg |= DO_GOTODEST; */
/* 		printf ("GO TO MENUBOX: %d\n", dest_menubox->id); */
/* 	      } */

	      if (MBOX (open_menubox)->parent_menuitem != picked) {
		dest_menubox = picked_menubox;
		dest_menuitem = picked;
		dest_selectfirst = FALSE;
		was_clicked = TRUE;

		internal_msg |= DO_GOTODEST;
		/* printf ("GO TO MENUITEM: %d %d\n", dest_menubox->id, dest_menuitem->id); */
	      }
	    }
	  }

	  /* control button-released outside menubox */
	  if ((!jmouse_b(0)) && (jmouse_b(1))) {
	    JWidget picked =
	      jwidget_pick
		   (menubox->parent_menuitem ?
		    menubox->parent_menuitem->parent: menubox->menu,
		    msg->mouse.x, msg->mouse.y);

	    if ((!picked) || (menubox->parent_menuitem &&
			      menubox->parent_menuitem != picked))
	      internal_msg |= DO_GOTOBAR;
	  }
	}
      }
      break;
  }

  return FALSE;
}

static void menubox_request_size(JWidget widget, int *w, int *h)
{
  MenuBox *menubox = MBOX (widget);

  if (menubox->menu)
    jwidget_request_size (menubox->menu, w, h);
  else
    *w = *h = 0;

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void menubox_set_position(JWidget widget, JRect rect)
{
  MenuBox *menubox = MBOX (widget);

  jrect_copy(widget->rc, rect);

  if (menubox->menu) {
    JRect cpos = jwidget_get_child_rect(widget);
    jwidget_set_rect(menubox->menu, cpos);
    jrect_free(cpos);
  }
}

static bool menuitem_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY: {
      MenuItem *menuitem = MITEM(widget);

      if (menuitem->accel)
	jaccel_free(menuitem->accel);

      if (menuitem->submenu)
	jwidget_free(menuitem->submenu);

      jfree(menuitem);
      break;
    }

    case JM_REQSIZE:
      menuitem_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      /* TODO theme specific!! */
      jwidget_dirty(widget);
      break;
  }

  return FALSE;
}

static void menuitem_request_size(JWidget widget, int *w, int *h)
{
  MenuItem *menuitem = MITEM (widget);
  int bar = widget->parent->parent->type == JI_MENUBAR;

  if (widget->text) {
    *w =
      + widget->border_width.l
      + jwidget_get_text_length (widget)
      + ((bar) ? widget->child_spacing/4: widget->child_spacing)
      + widget->border_width.r;

    *h =
      + widget->border_width.t
      + jwidget_get_text_height (widget)
      + widget->border_width.b;

    if (menuitem->accel) {
      char buf[256];
      jaccel_to_string(menuitem->accel, buf);
      *w += ji_font_text_len(widget->text_font, buf);
    }
  }
  else {
    *w = *h = 0;
  }
}

static JWidget get_highlight(JWidget menu)
{
  JWidget child;
  JLink link;

  JI_LIST_FOR_EACH(menu->children, link) {
    child = (JWidget)link->data;

    if (child->type != JI_MENUITEM)
      continue;

    if (MITEM(child)->highlight)
      return child;
  }

  return NULL;
}

static void set_highlight(JWidget menu, JWidget menuitem)
{
  JWidget child;
  JLink link;
  
  JI_LIST_FOR_EACH(menu->children, link) {
    child = (JWidget)link->data;

    if (child->type != JI_MENUITEM)
      continue;

    if (MITEM(child)->highlight) {
      if (child == menuitem) {
	menuitem = NULL;
	continue;
      }

      MITEM(child)->highlight = FALSE;
      jwidget_dirty(child);
    }
  }

  if (menuitem) {
    MITEM(menuitem)->highlight = TRUE;
    jwidget_dirty(menuitem);
  }
}

static void unhighlight(JWidget menu)
{
  set_highlight(menu, NULL);
}

static void open_menuitem(JWidget menuitem, bool select_first)
{
  static struct jrect menubox_pos;
  bool first_one = current_level == 0 ? TRUE: FALSE;
  JRect pos, old_pos;

  if (first_one)
    old_pos = jwidget_get_rect(menuitem->parent->parent);
  else
    old_pos = jrect_new_copy(&menubox_pos);

/*   printf ("ENTER current_level=%d\n", current_level); */
  current_level++;

  if (HAS_SUBMENU(menuitem)) {
    JWidget window, menubox;

    MITEM(menuitem)->opened = TRUE; /* activate "opened" flag in the
				       menuitem */
    /* TODO */
/*     jwidget_flush_redraw (menuitem); /\* redraw the menuitem (the */
/* 					  highlight could be change) *\/ */

    /* new window and new menu-box */
    window = jwindow_new(NULL);
    menubox = jmenubox_new();

    /* created_windows = jlist_prepend (created_windows, window); */

    jwindow_moveable(window, FALSE); /* can't move the window */
    MBOX(menubox)->parent_menuitem = menuitem; /* menuitem parent */

    /* set children */
    jmenubox_set_menu(menubox, MITEM(menuitem)->submenu);
    jwidget_add_child(window, menubox);

    jwindow_remap(window);

    /* menubox position */
    pos = jwidget_get_rect (window);

    if (menuitem->parent->parent->type == JI_MENUBAR) {
      jrect_moveto(pos,
		     MID(0, menuitem->rc->x1, JI_SCREEN_W-jrect_w(pos)),
		     MID(0, menuitem->rc->y2, JI_SCREEN_H-jrect_h(pos)));
    }
    else {
      int x_left = menuitem->rc->x1-jrect_w(pos);
      int x_right = menuitem->rc->x2;
      int x, y = menuitem->rc->y1;
      struct jrect r1, r2;
      int s1, s2;

      r1.x1 = x_left = MID (0, x_left, JI_SCREEN_W-jrect_w(pos));
      r2.x1 = x_right = MID (0, x_right, JI_SCREEN_W-jrect_w(pos));

      r1.y1 = r2.y1 = y = MID (0, y, JI_SCREEN_H-jrect_h(pos));

      r1.x2 = r1.x1+jrect_w(pos);
      r1.y2 = r1.y1+jrect_h(pos);
      r2.x2 = r2.x1+jrect_w(pos);
      r2.y2 = r2.y1+jrect_h(pos);

      /* calculate both intersections */
      s1 = jrect_intersect (&r1, old_pos);
      s2 = jrect_intersect (&r2, old_pos);

      if (!s2)
	x = x_right;		/* use the right because there aren't
				   intersection with it */
      else if (!s1)
	x = x_left;		/* use the left because there are not
				   intersection */
      else if (jrect_w(&r2)*jrect_h(&r2) <= jrect_w(&r1)*jrect_h(&r1))
	x = x_right;	        /* use the right because there are less
				   intersection area */
      else
	x = x_left;		/* use the left because there are less
				   intersection area */

      jrect_moveto (pos, x, y);
    }

    jrect_copy (&menubox_pos, pos);
    jwindow_position (window, pos->x1, pos->y1);
    jrect_free (pos);

    /* set the focus to the new menubox */
    /* jmanager_set_focus (menubox); */
    jwidget_magnetic (menubox, TRUE);

    /* setup the highlight of the new menubox */
    if (select_first)
      /* TODO */
/*       set_highlight(menubox, */
      set_highlight(MBOX(menubox)->menu,
		    jlist_first(MBOX(menubox)->menu->children)->data);
    else
      unhighlight(MBOX(menubox)->menu);

    /* run in foreground */
/*     { */
/*       static int tab = 0; */
/*       int c; */
/*       for (c=0; c<tab; c++) printf ("  "); printf ("new %d\n", window->id); */
/*       tab++; */
    jwindow_open_fg(window);
/*       tab--; */
/*       for (c=0; c<tab; c++) printf ("  "); printf ("free %d\n", window->id); */
/*     } */

    /* menuitem isn't "opened" anymore */
    MITEM(menuitem)->opened = FALSE;

    if (internal_msg & DO_GOTOBAR) {
      /* final */
      if (first_one) {
	/* TODO this isn't necessary */
/* 	internal_msg ^= DO_GOTOBAR; */
	was_clicked = FALSE;
      }

      jmanager_free_focus();
      unhighlight(menuitem->parent);
    }
    else {
      /* set the focus to the old menubox */
      jmanager_set_focus(menuitem->parent->parent);
      set_highlight(menuitem->parent, menuitem);
    }

    /* fetch the "menu" */
    jmenubox_set_menu(menubox, NULL);

    /* destroy the window */
    jwidget_free (window);
  }

  jrect_copy (&menubox_pos, old_pos);
  jrect_free (old_pos);

  current_level--;
/*   printf ("EXIT current_level=%d\n", current_level); */

  /* emit signal */
  if (emitter_menuitem) {
    emit_signal (emitter_menuitem);
    was_clicked = FALSE;
  }
}

static void emit_signal(JWidget menuitem)
{
  emitter_menuitem = menuitem;

  if (!current_level) {
    jwidget_emit_signal (emitter_menuitem, JI_SIGNAL_MENUITEM_SELECT);
    emitter_menuitem = NULL;
  }
}

static JWidget check_for_letter(JWidget menu, int ascii)
{
  JWidget menuitem;
  JLink link;
  int c;

  JI_LIST_FOR_EACH(menu->children, link) {
    menuitem = (JWidget)link->data;
    if (menuitem->text)
      for (c=0; menuitem->text[c]; c++)
        if ((menuitem->text[c] == '&') && (menuitem->text[c+1] != '&'))
          if (tolower(ascii) == tolower(menuitem->text[c+1]))
	    return menuitem;
  }

  return NULL;
}

static JWidget check_for_accel(JWidget menu, JMessage msg)
{
  JWidget menuitem;
  JLink link;

  JI_LIST_FOR_EACH(menu->children, link) {
    menuitem = (JWidget)link->data;

    if (menuitem->type != JI_MENUITEM)
      continue;

    if (MITEM(menuitem)->submenu) {
      if ((menuitem = check_for_accel(MITEM(menuitem)->submenu, msg)))
        return menuitem;
    }
    else if (MITEM(menuitem)->accel) {
      if ((jwidget_is_enabled(menuitem)) &&
	  (jaccel_check(MITEM(menuitem)->accel,
			  msg->any.shifts,
			  msg->key.ascii,
			  msg->key.scancode)))
	return menuitem;
    }
  }

  return NULL;
}

/* returns the widget (of JI_MENUITEM type) in the mouse position, or
   NULL if there aren't nothing */
static JWidget pick_menuitem(JWidget *_menubox, JWidget *open_menubox)
{
  JWidget menubox = *_menubox;
  JWidget picked = NULL;

  for (;;) {
    if (MOUSE_IN(menubox->rc)) {
      *_menubox = menubox;
      picked = jwidget_pick(MBOX(menubox)->menu,
			    jmouse_x(0), jmouse_y(0));
      if (picked->type != JI_MENUITEM)
	picked = NULL;
      break;
    }
    else if (MBOX(menubox)->parent_menuitem) {
      *open_menubox = menubox;
      menubox = MBOX(menubox)->parent_menuitem->parent->parent;
    }
    else
      break;
  }

  return picked;
}

/* finds the next item of `menuitem', if `menuitem' is NULL searchs
   from the first item in `menu' */
static JWidget find_nextitem(JWidget menu, JWidget menuitem)
{
  JWidget nextitem;
  JLink link;

  if (menuitem)
    link = jlist_find(menu->children, menuitem)->next;
  else
    link = jlist_first(menu->children);

  for (; link != menu->children->end; link=link->next) {
    nextitem = (JWidget)link->data;
    if ((nextitem->type == JI_MENUITEM) && jwidget_is_enabled(nextitem))
      return nextitem;
  }

  if (menuitem)
    return find_nextitem(menu, NULL);
  else
   return NULL;
}

static JWidget find_previtem(JWidget menu, JWidget menuitem)
{
  JWidget nextitem;
  JLink link;

  if (menuitem)
    link = jlist_find(menu->children, menuitem)->prev;
  else
    link = jlist_last(menu->children);

  for (; link != menu->children->end; link=link->prev) {
    nextitem = (JWidget)link->data;
    if ((nextitem->type == JI_MENUITEM) && jwidget_is_enabled(nextitem))
      return nextitem;
  }

  if (menuitem)
    return find_previtem(menu, NULL);
  else
    return NULL;
}
