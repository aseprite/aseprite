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

// #define REPORT_EVENTS
// #define LIMIT_DISPATCH_TIME

#include "config.h"

#ifdef REPORT_EVENTS
#include <stdio.h>
#endif

#include <allegro.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#define TOPWND(manager) reinterpret_cast<Frame*>(jlist_first_data((manager)->children))

#define ACCEPT_FOCUS(widget)				\
  (((widget)->flags & (JI_FOCUSREST |			\
		       JI_DISABLED |			\
		       JI_HIDDEN |			\
		       JI_DECORATIVE)) == JI_FOCUSREST)

#define DOUBLE_CLICK_TIMEOUT_MSECS   400

enum {
  DOUBLE_CLICK_NONE,
  DOUBLE_CLICK_DOWN,
  DOUBLE_CLICK_UP
};

enum {
  STAGE_NORMAL,
  STAGE_WANT_CLOSE,
  STAGE_WAITING_REPLY,
  STAGE_CLOSE_ALL,
};

typedef struct Timer {
  JWidget widget;
  int interval;
  int last_time;
} Timer;

#define NFILTERS	(JM_REGISTERED_MESSAGES+1)

typedef struct Filter {
  int message;
  JWidget widget;
} Filter;

static int double_click_level;
static int double_click_buttons;
static int double_click_ticks;

static int want_close_stage;	   /* variable to handle the external
				      close button in some Windows
				      enviroments */

static JWidget default_manager = NULL;

static Timer **timers;		/* registered timers */
static int n_timers;		/* number of timers */

static JList new_windows;	/* windows that we should show */
static JList proc_windows_list; /* current window's list in process */
static JList msg_queue;		/* messages queue */
static JList msg_filters[NFILTERS]; /* filters for every enqueued message */

static JWidget focus_widget;	/* the widget with the focus */
static JWidget mouse_widget;	/* the widget with the mouse */
static JWidget capture_widget;	/* the widget that captures the
				   mouse */

static bool first_time_poll;    /* true when we don't enter in poll yet */

static char old_readed_key[KEY_MAX]; /* keyboard status of previous
					poll */

static unsigned key_repeated[KEY_MAX];

/* manager widget */
static bool manager_msg_proc(JWidget widget, JMessage msg);
static void manager_request_size(JWidget widget, int *w, int *h);
static void manager_set_position(JWidget widget, JRect rect);
static void manager_pump_queue(JWidget widget);

/* auxiliary */
static void generate_setcursor_message();
static void remove_msgs_for(JWidget widget, JMessage msg);
static void generate_proc_windows_list();
static void generate_proc_windows_list2(JWidget manager);
static int some_parent_is_focusrest(JWidget widget);
static JWidget find_magnetic_widget(JWidget widget);
static JMessage new_mouse_msg(int type, JWidget destination);
static void broadcast_key_msg(JWidget manager, JMessage msg);
static Filter* filter_new(int message, JWidget widget);
static void filter_free(Filter* filter);

/* keyboard focus movement stuff */
static bool move_focus(JWidget manager, JMessage msg);
static int count_widgets_accept_focus(JWidget widget);
static bool childs_accept_focus(JWidget widget, bool first);
static JWidget next_widget(JWidget widget);
static int cmp_left(JWidget widget, int x, int y);
static int cmp_right(JWidget widget, int x, int y);
static int cmp_up(JWidget widget, int x, int y);
static int cmp_down(JWidget widget, int x, int y);

/* hooks the close-button in some platform with window support */
static void allegro_window_close_hook()
{
  if (want_close_stage == STAGE_NORMAL)
    want_close_stage = STAGE_WANT_CLOSE;
}

JWidget ji_get_default_manager()
{
  return default_manager;
}

JWidget jmanager_new()
{
  JWidget widget;
  int c;

  if (!default_manager) {
    if (!ji_screen)
      ji_set_screen(screen, SCREEN_W, SCREEN_H);

    /* hook the window close message */
    want_close_stage = STAGE_NORMAL;
    set_window_close_hook(allegro_window_close_hook);

    /* empty lists */
    msg_queue = jlist_new();
    new_windows = jlist_new();
    proc_windows_list = jlist_new();

    for (c=0; c<NFILTERS; ++c)
      msg_filters[c] = jlist_new();

    /* reset variables */
    focus_widget = NULL;
    mouse_widget = NULL;
    capture_widget = NULL;

    first_time_poll = true;
    double_click_level = DOUBLE_CLICK_NONE;
    double_click_ticks = 0;

    /* reset keyboard */
    for (c=0; c<KEY_MAX; c++) {
      old_readed_key[c] = 0;
      key_repeated[c] = 0;
    }

    /* timers */
    timers = NULL;
    n_timers = 0;
  }

  widget = new Widget(JI_MANAGER);

  jwidget_add_hook(widget, JI_MANAGER, manager_msg_proc, NULL);

  jrect_replace(widget->rc, 0, 0, JI_SCREEN_W, JI_SCREEN_H);
  widget->setVisible(true);

  /* default manager is the first one (and is ever visible) */
  if (!default_manager)
    default_manager = widget;

  return widget;
}

void jmanager_free(JWidget widget)
{
  JLink link;
  int c;

  ASSERT_VALID_WIDGET(widget);

  /* there are some messages in queue? */
  jmanager_dispatch_messages(widget);

  /* finish with main manager */
  if (default_manager == widget) {
    /* no more cursor */
    jmouse_set_cursor(JI_CURSOR_NULL);

    /* destroy this widget */
    jwidget_free(widget);

    /* destroy timers */
    if (timers != NULL) {
      for (c=0; c<n_timers; ++c)
	if (timers[c] != NULL)
	  jfree(timers[c]);
      jfree(timers);
      n_timers = 0;
    }

    /* destroy filters */
    for (c=0; c<NFILTERS; ++c) {
      JI_LIST_FOR_EACH(msg_filters[c], link) {
	filter_free(reinterpret_cast<Filter*>(link->data));
      }
      jlist_free(msg_filters[c]);
      msg_filters[c] = NULL;
    }

    /* no more default manager */
    default_manager = NULL;

    /* shutdown system */
    jlist_free(msg_queue);
    jlist_free(new_windows);
    jlist_free(proc_windows_list);
  }
  else {
    /* destroy this widget */
    jwidget_free(widget);
  }
}

void jmanager_run(JWidget widget)
{
  while (!jlist_empty(widget->children)) {
    if (jmanager_generate_messages(widget))
      jmanager_dispatch_messages(widget);
  }
}

/**
 * @return true if there are messages in the queue to be distpatched
 *         through jmanager_dispatch_messages().
 */
bool jmanager_generate_messages(JWidget manager)
{
  JWidget widget;
  JWidget window;
  int mousemove;
  JMessage msg;
  JLink link;
  int c;

  /* poll keyboard */
  poll_keyboard();

  if (first_time_poll) {
    first_time_poll = false;
    
    jmanager_refresh_screen();
    jmouse_set_cursor(JI_CURSOR_NORMAL);
  }

  /* first check: there are windows to manage? */
  if (jlist_empty(manager->children))
    return false;

  /* new windows to show? */
  if (!jlist_empty(new_windows)) {
    JWidget magnet;

    JI_LIST_FOR_EACH(new_windows, link) {
      window = reinterpret_cast<JWidget>(link->data);

      /* dirty the entire window and show it */
      jwidget_dirty(window);
      window->setVisible(true);

      /* attract the focus to the magnetic widget... */
      /* 1) get the magnetic widget */
      magnet = find_magnetic_widget(window->getRoot());
      /* 2) if magnetic widget exists and it doesn't have the focus */
      if (magnet && !magnet->hasFocus())
	jmanager_set_focus(magnet);
      /* 3) if not, put the focus in the first child */
      else
	jmanager_focus_first_child(window);
    }

    jlist_clear(new_windows);

    generate_proc_windows_list();
  }

/*   generate_proc_windows_list(manager); */
  if (jlist_empty(proc_windows_list))
    generate_proc_windows_list();

  /* update mouse status */
  mousemove = jmouse_poll();
  if (mousemove || !mouse_widget) {
    /* get the widget under the mouse */
    widget = NULL;

    JI_LIST_FOR_EACH(proc_windows_list, link) {
      window = reinterpret_cast<JWidget>(link->data);
      widget = window->pick(jmouse_x(0), jmouse_y(0));
      if (widget)
	break;
    }

    /* fixup "mouse" flag */
    if (widget != mouse_widget) {
      if (!widget)
	jmanager_free_mouse();
      else
	jmanager_set_mouse(widget);
    }

    /* mouse movement */
    if (mousemove) {
      JWidget dst;

      /* reset double click status */
      double_click_level = DOUBLE_CLICK_NONE;

      if (capture_widget)
	dst = capture_widget;
      else
	dst = mouse_widget;

      /* send the mouse movement message */
      msg = new_mouse_msg(JM_MOTION, dst);
      jmanager_enqueue_message(msg);

      generate_setcursor_message();
    }
  }

  /* mouse wheel */
  if (jmouse_z(0) != jmouse_z(1)) {
    msg = new_mouse_msg(JM_WHEEL,
			capture_widget ? capture_widget:
					 mouse_widget);
    jmanager_enqueue_message(msg);
  }

  // mouse clicks
  if (jmouse_b(0) != jmouse_b(1)) {
    int current_ticks = ji_clock;
    bool pressed =
      ((jmouse_b(1) & 1) == 0 && (jmouse_b(0) & 1) == 1) ||
      ((jmouse_b(1) & 2) == 0 && (jmouse_b(0) & 2) == 2) ||
      ((jmouse_b(1) & 4) == 0 && (jmouse_b(0) & 4) == 4);

    msg = new_mouse_msg(pressed ? JM_BUTTONPRESSED:
				  JM_BUTTONRELEASED,
			capture_widget ? capture_widget:
					 mouse_widget);

    /**********************************************************************/
    /* Double Click */
    if (msg->type == JM_BUTTONPRESSED) {
      if (double_click_level != DOUBLE_CLICK_NONE) {
	/* time out, back to NONE */
	if (current_ticks - double_click_ticks > DOUBLE_CLICK_TIMEOUT_MSECS) {
	  double_click_level = DOUBLE_CLICK_NONE;
	}
	else if (double_click_buttons == msg->mouse.flags) {
	  if (double_click_level == DOUBLE_CLICK_UP) {
	    msg->type = JM_DOUBLECLICK;
	  }
	  else {
	    double_click_level = DOUBLE_CLICK_NONE;
	  }
	}
	/* press other button, back to NONE */
	else {
	  double_click_level = DOUBLE_CLICK_NONE;
	}
      }

      /* this could be the beginning of the state */
      if (double_click_level == DOUBLE_CLICK_NONE) {
	double_click_level = DOUBLE_CLICK_DOWN;
	double_click_buttons = msg->mouse.flags;
	double_click_ticks = current_ticks;
      }
    }
    else if (msg->type == JM_BUTTONRELEASED) {
      if (double_click_level != DOUBLE_CLICK_NONE) {
	/* time out, back to NONE */
	if (current_ticks - double_click_ticks > DOUBLE_CLICK_TIMEOUT_MSECS) {
	  double_click_level = DOUBLE_CLICK_NONE;
	}
	else if (double_click_buttons == msg->mouse.flags) {
	  if (double_click_level == DOUBLE_CLICK_DOWN) {
	    double_click_level = DOUBLE_CLICK_UP;
	    double_click_ticks = current_ticks;
	  }
	}
	/* press other button, back to NONE */
	else {
	  double_click_level = DOUBLE_CLICK_NONE;
	}
      }
    }

    /* Z-Order:
       Send the window to top (only when you click in a window
       that aren't the desktop) */
    if (msg->type == JM_BUTTONPRESSED &&
	!capture_widget && mouse_widget) {
      Frame* window = static_cast<Frame*>(mouse_widget->getRoot());
      JWidget win_manager = window ? window->getManager(): NULL;

      if ((window) &&
	  (!window->is_desktop()) &&
	  (window != TOPWND(win_manager))) {
	/* put it in the top of the list */
	jlist_remove(win_manager->children, window);

	if (window->is_ontop())
	  jlist_prepend(win_manager->children, window);
	else {
	  int pos = jlist_length(win_manager->children);
	  JI_LIST_FOR_EACH_BACK(win_manager->children, link) {
	    if (((Frame*)link->data)->is_ontop())
	      break;
	    pos--;
	  }
	  jlist_insert(win_manager->children, window, pos);
	}

	generate_proc_windows_list();
	jwidget_dirty(window);
      }

      /* put the focus */
      jmanager_set_focus(mouse_widget);
    }

    jmanager_enqueue_message(msg);
  }

  /* generate ESC key when the user press close button in the system window */
  if (want_close_stage == STAGE_WANT_CLOSE) {
    want_close_stage = STAGE_NORMAL;

    msg = jmessage_new_key_related(JM_KEYPRESSED, (KEY_ESC << 8) | 27);
    broadcast_key_msg(manager, msg);
    jmanager_enqueue_message(msg);

    msg = jmessage_new_key_related(JM_KEYRELEASED, (KEY_ESC << 8) | 27);
    broadcast_key_msg(manager, msg);
    jmanager_enqueue_message(msg);
  }

  /* generate JM_CHAR/JM_KEYPRESSED messages */
  while (keypressed()) {
    int readkey_value = readkey();

    /* char message first */
    msg = jmessage_new_key_related(JM_KEYPRESSED, readkey_value);

    c = readkey_value >> 8;
    old_readed_key[c] = key[c];
    msg->key.repeat = key_repeated[c]++;

    broadcast_key_msg(manager, msg);
    jmanager_enqueue_message(msg);
  }

  for (c=0; c<KEY_MAX; c++) {
    if (old_readed_key[c] != key[c]) {
      /* generate JM_KEYRELEASED messages (old key state is activated,
	 the new one is deactivated) */
      if (old_readed_key[c]) {
	/* press/release key interface */
	msg = jmessage_new_key_related(JM_KEYRELEASED,
				       (c << 8) | scancode_to_ascii(c));
	old_readed_key[c] = key[c];
	key_repeated[c] = 0;

	broadcast_key_msg(manager, msg);
	jmanager_enqueue_message(msg);
      }
      /* generate JM_KEYPRESSED messages for modifiers */
      else if (c >= KEY_MODIFIERS) {
	/* press/release key interface */
	msg = jmessage_new_key_related(JM_KEYPRESSED,
				       (c << 8) | scancode_to_ascii(c));
	old_readed_key[c] = key[c];
	msg->key.repeat = key_repeated[c]++;

	broadcast_key_msg(manager, msg);
	jmanager_enqueue_message(msg);
      }
    }
  }

  /* timers */
  if (n_timers > 0) {
    int t = ji_clock;
    int count;

    for (c=0; c<n_timers; ++c) {
      if (timers[c] && timers[c]->last_time >= 0) {
	count = 0;
	while (t - timers[c]->last_time > timers[c]->interval) {
	  timers[c]->last_time += timers[c]->interval;
	  ++count;

	  /* we spend too much time here */
	  if (ji_clock - t > timers[c]->interval) {
	    timers[c]->last_time = ji_clock;
	    break;
	  }
	}

	if (count > 0) {
	  msg = jmessage_new(JM_TIMER);
	  msg->timer.count = count;
	  msg->timer.timer_id = c;
	  jmessage_add_dest(msg, timers[c]->widget);
	  jmanager_enqueue_message(msg);
	}
      }
    }
  }

  /* generate redraw events */
  jwidget_flush_redraw(manager);

  if (!jlist_empty(msg_queue))
    return true;
  else {
    /* make some OSes happy */
    yield_timeslice();
    rest(1);
    return false;
  }
}

void jmanager_dispatch_messages(JWidget manager)
{
  JMessage msg;

  ASSERT(manager != NULL);
  
  /* add the "Queue Processing" message for the manager */
  msg = new_mouse_msg(JM_QUEUEPROCESSING, manager);
  jmanager_enqueue_message(msg);

  manager_pump_queue(manager);
}

/**
 * Adds a timer event for the specified widget.
 *
 * @return A timer ID that can be used with @ref jmanager_remove_timer
 */
int jmanager_add_timer(JWidget widget, int interval)
{
  Timer *timer;
  int c, new_id = -1;

  ASSERT_VALID_WIDGET(widget);

  for (c=0; c<n_timers; ++c) {
    /* there are an empty slot */
    if (timers[c] == NULL) {
      new_id = c;
      break;
    }
  }

  if (new_id < 0) {
    new_id = n_timers++;
    timers = jrenew(Timer *, timers, n_timers);
  }

  timer = jnew(Timer, 1);
  timer->widget = widget;
  timer->interval = interval;
  timer->last_time = -1;
  timers[new_id] = timer;

  return new_id;
}

void jmanager_remove_timer(int timer_id)
{
  JMessage message;
  JLink link, next;

  ASSERT(timer_id >= 0 && timer_id < n_timers);
  ASSERT(timers[timer_id] != NULL);

  jfree(timers[timer_id]);
  timers[timer_id] = NULL;

  /* remove messages of this timer in the queue */
  JI_LIST_FOR_EACH_SAFE(msg_queue, link, next) {
    message = reinterpret_cast<JMessage>(link->data);
    if (!message->any.used &&
	message->any.type == JM_TIMER &&
	message->timer.timer_id == timer_id) {
      printf("REMOVING A TIMER MESSAGE FROM THE QUEUE!!\n"); fflush(stdout);
      jmessage_free(reinterpret_cast<JMessage>(link->data));
      jlist_delete_link(msg_queue, link);
    }
  }
}

void jmanager_start_timer(int timer_id)
{
  ASSERT(timer_id >= 0 && timer_id < n_timers);
  ASSERT(timers[timer_id] != NULL);

  timers[timer_id]->last_time = ji_clock;
}

void jmanager_stop_timer(int timer_id)
{
  ASSERT(timer_id >= 0 && timer_id < n_timers);
  ASSERT(timers[timer_id] != NULL);

  timers[timer_id]->last_time = -1;
}

void jmanager_set_timer_interval(int timer_id, int interval)
{
  ASSERT(timer_id >= 0 && timer_id < n_timers);
  ASSERT(timers[timer_id] != NULL);

  timers[timer_id]->interval = interval;
}

bool jmanager_timer_is_running(int timer_id)
{
  ASSERT(timer_id >= 0 && timer_id < n_timers);
  ASSERT(timers[timer_id] != NULL);

  return (timers[timer_id]->last_time >= 0);
}

/**
 * @param msg You can't use the this message after calling this
 *            routine. The message will be automatically freed through
 *            @ref jmessage_free
 */
void jmanager_enqueue_message(JMessage msg)
{
  int c;

  ASSERT(msg_queue != NULL);
  ASSERT(msg != NULL);

  /* check if this message must be filtered by some widget before */
  c = msg->type;
  if (c >= JM_REGISTERED_MESSAGES)
    c = JM_REGISTERED_MESSAGES;

  if (!jlist_empty(msg_filters[c])) { /* ok, so are filters to add ... */
    JLink link;

    /* add all the filters in the destination list of the message */
    JI_LIST_FOR_EACH_BACK(msg_filters[c], link) {
      Filter* filter = reinterpret_cast<Filter*>(link->data);
      if (msg->type == filter->message)
	jmessage_add_pre_dest(msg, filter->widget);
    }
  }

  /* there are a destination widget at least? */
  if (!jlist_empty(msg->any.widgets))
    jlist_append(msg_queue, msg);
  else
    jmessage_free(msg);
}

JWidget jmanager_get_focus()
{
  return focus_widget;
}

JWidget jmanager_get_mouse()
{
  return mouse_widget;
}

JWidget jmanager_get_capture()
{
  return capture_widget;
}

void jmanager_set_focus(JWidget widget)
{
  if ((focus_widget != widget)
      && (!(widget)
	  || (!(widget->flags & JI_DISABLED)
	      && !(widget->flags & JI_HIDDEN)
	      && !(widget->flags & JI_DECORATIVE)
	      && some_parent_is_focusrest(widget)))) {
    JList widget_parents = NULL;
    JWidget common_parent = NULL;
    JLink link, link2;
    JMessage msg;

    if (widget)
      widget_parents = jwidget_get_parents(widget, false);
    else
      widget_parents = jlist_new();

    /* fetch the focus */

    if (focus_widget) {
      JList focus_parents = jwidget_get_parents(focus_widget, true);
      msg = jmessage_new(JM_FOCUSLEAVE);

      JI_LIST_FOR_EACH(focus_parents, link) {
	if (widget) {
	  JI_LIST_FOR_EACH(widget_parents, link2) {
	    if (link->data == link2->data) {
	      common_parent = reinterpret_cast<JWidget>(link->data);
	      break;
	    }
	  }
	  if (common_parent)
	    break;
	}

	if (reinterpret_cast<JWidget>(link->data)->hasFocus()) {
	  ((JWidget)link->data)->flags &= ~JI_HASFOCUS;
	  jmessage_add_dest(msg, reinterpret_cast<JWidget>(link->data));
	}
      }

      jmanager_enqueue_message(msg);
      jlist_free(focus_parents);
    }

    /* put the focus */

    focus_widget = widget;

    if (widget) {
      if (common_parent)
	link = jlist_find(widget_parents, common_parent)->next;
      else
	link = jlist_first(widget_parents);

      msg = jmessage_new(JM_FOCUSENTER);

      for (; link != widget_parents->end; link=link->next) {
	JWidget w = (JWidget)link->data;

	if (w->flags & JI_FOCUSREST) {
	  w->flags |= JI_HASFOCUS;

	  jmessage_add_dest(msg, w);
	}
      }

      jmanager_enqueue_message(msg);
    }

    jlist_free(widget_parents);
  }
}

void jmanager_set_mouse(JWidget widget)
{
  if ((mouse_widget != widget) && (!capture_widget)) {
    JList widget_parents = NULL;
    JWidget common_parent = NULL;
    JLink link, link2;
    JMessage msg;

    if (widget)
      widget_parents = jwidget_get_parents(widget, false);
    else
      widget_parents = jlist_new();

    /* fetch the mouse */

    if (mouse_widget) {
      JList mouse_parents = jwidget_get_parents(mouse_widget, true);
      msg = jmessage_new(JM_MOUSELEAVE);

      JI_LIST_FOR_EACH(mouse_parents, link) {
	if (widget) {
	  JI_LIST_FOR_EACH(widget_parents, link2) {
	    if (link->data == link2->data) {
	      common_parent = reinterpret_cast<JWidget>(link->data);
	      break;
	    }
	  }
	  if (common_parent)
	    break;
	}

	if (reinterpret_cast<JWidget>(link->data)->hasMouse()) {
	  ((JWidget)link->data)->flags &= ~JI_HASMOUSE;
	  jmessage_add_dest(msg, reinterpret_cast<JWidget>(link->data));
	}
      }

      jmanager_enqueue_message(msg);
      jlist_free(mouse_parents);
    }

    /* put the mouse */

    mouse_widget = widget;

    if (widget) {
      if (common_parent)
	link = jlist_find(widget_parents, common_parent)->next;
      else
	link = jlist_first(widget_parents);

      msg = jmessage_new(JM_MOUSEENTER);

      for (; link != widget_parents->end; link=link->next) {
	reinterpret_cast<JWidget>(link->data)->flags |= JI_HASMOUSE;
	jmessage_add_dest(msg, reinterpret_cast<JWidget>(link->data));
      }

      jmanager_enqueue_message(msg);
      generate_setcursor_message();
    }

    jlist_free(widget_parents);
  }
}

void jmanager_set_capture(JWidget widget)
{
  widget->flags |= JI_HASCAPTURE;
  capture_widget = widget;
}

/* sets the focus to the "magnetic" widget inside the window */
void jmanager_attract_focus(JWidget widget)
{
  /* get the magnetic widget */
  JWidget magnet = find_magnetic_widget(widget->getRoot());

  /* if magnetic widget exists and it doesn't have the focus */
  if (magnet && !magnet->hasFocus())
    jmanager_set_focus(magnet);
}

void jmanager_focus_first_child(JWidget widget)
{
  for (Widget* it=widget->getRoot(); it; it=next_widget(it)) {
    if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true))) {
      jmanager_set_focus(it);
      break;
    }
  }
}

void jmanager_free_focus()
{
  jmanager_set_focus(NULL);
}

void jmanager_free_mouse()
{
  jmanager_set_mouse(NULL);
}

void jmanager_free_capture()
{
  if (capture_widget) {
    capture_widget->flags &= ~JI_HASCAPTURE;
    capture_widget = NULL;
  }
}

void jmanager_free_widget(JWidget widget)
{
  // Break any relationship with the GUI manager

  if (widget->hasCapture() || (widget == capture_widget))
    jmanager_free_capture();

  if (widget->hasMouse() || (widget == mouse_widget))
    jmanager_free_mouse();

  if (widget->hasFocus() || (widget == focus_widget))
    jmanager_free_focus();
}

void jmanager_remove_message(JMessage msg)
{
  jlist_remove(msg_queue, msg);
}

void jmanager_remove_messages_for(JWidget widget)
{
  JLink link;
  JI_LIST_FOR_EACH(msg_queue, link)
    remove_msgs_for(widget, reinterpret_cast<JMessage>(link->data));
}

void jmanager_refresh_screen()
{
  if (default_manager)
    jwidget_invalidate(default_manager);
}

void jmanager_add_msg_filter(int message, JWidget widget)
{
  int c = message;
  if (c >= JM_REGISTERED_MESSAGES)
    c = JM_REGISTERED_MESSAGES;
  
  jlist_append(msg_filters[c], filter_new(message, widget));
}

void jmanager_remove_msg_filter(int message, JWidget widget)
{
  JLink link, next;
  int c = message;
  if (c >= JM_REGISTERED_MESSAGES)
    c = JM_REGISTERED_MESSAGES;

  JI_LIST_FOR_EACH_SAFE(msg_filters[c], link, next) {
    Filter* filter = reinterpret_cast<Filter*>(link->data);
    if (filter->widget == widget) {
      filter_free(filter);
      jlist_delete_link(msg_filters[c], link);
    }
  }
}

void jmanager_remove_msg_filter_for(JWidget widget)
{
  JLink link, next;
  int c;

  for (c=0; c<NFILTERS; ++c) {
    JI_LIST_FOR_EACH_SAFE(msg_filters[c], link, next) {
      Filter* filter = reinterpret_cast<Filter*>(link->data);
      if (filter->widget == widget) {
	filter_free(filter);
	jlist_delete_link(msg_filters[c], link);
      }
    }
  }
}

/* configures the window for begin the loop */
void _jmanager_open_window(JWidget manager, Frame* window)
{
  JMessage msg;

  // free all widgets of special states
  if (window->is_wantfocus()) {
    jmanager_free_capture();
    jmanager_free_mouse();
    jmanager_free_focus();
  }

  /* add the window to manager */
  jlist_prepend(manager->children, window);
  window->parent = manager;

  /* append signal */
  jwidget_emit_signal(manager, JI_SIGNAL_MANAGER_ADD_WINDOW);

  /* broadcast the open message */
  msg = jmessage_new(JM_OPEN);
  jmessage_add_dest(msg, window);
  jmanager_enqueue_message(msg);

  /* update the new windows list to show */
  jlist_append(new_windows, window);
}

void _jmanager_close_window(JWidget manager, Frame* window, bool redraw_background)
{
  JMessage msg;
  JRegion reg1;

  if (!jwidget_has_child(manager, window))
    return;

  if (redraw_background)
    reg1 = jwidget_get_region(window);
  else
    reg1 = NULL;

  /* close all windows to this desktop */
  if (window->is_desktop()) {
    JLink link, next;

    JI_LIST_FOR_EACH_SAFE(manager->children, link, next) {
      if (link->data == window)
	break;
      else {
	JRegion reg2 = jwidget_get_region(window);
	jregion_union(reg1, reg1, reg2);
	jregion_free(reg2);

	_jmanager_close_window(manager, reinterpret_cast<Frame*>(link->data), false);
      }
    }
  }

  /* free all widgets of special states */
  if (capture_widget != NULL && capture_widget->getRoot() == window)
    jmanager_free_capture();

  if (mouse_widget != NULL && mouse_widget->getRoot() == window)
    jmanager_free_mouse();

  if (focus_widget != NULL && focus_widget->getRoot() == window)
    jmanager_free_focus();

  /* hide window */
  window->setVisible(false);

  /* close message */
  msg = jmessage_new(JM_CLOSE);
  jmessage_add_dest(msg, window);
  jmanager_enqueue_message(msg);

  /* update manager list stuff */
  jlist_remove(manager->children, window);
  window->parent = NULL;
  generate_proc_windows_list();

  /* remove signal */
  jwidget_emit_signal(manager, JI_SIGNAL_MANAGER_REMOVE_WINDOW);

  /* redraw background */
  if (reg1) {
    jwidget_invalidate_region(manager, reg1);
    jregion_free(reg1);
  }

  /* maybe the window is in the "new_windows" list */
  jlist_remove(new_windows, window);
}

/**********************************************************************
				Manager
 **********************************************************************/

static bool manager_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DEFERREDFREE:
      ASSERT_VALID_WIDGET(msg->deffree.widget_to_free);
      ASSERT(msg->deffree.widget_to_free != widget);

      jwidget_free(msg->deffree.widget_to_free);
      return true;

    case JM_REQSIZE:
      manager_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      manager_set_position(widget, &msg->setpos.rect);
      return true;

    case JM_DRAW:
      jdraw_rectfill(&msg->draw.rect, widget->theme->desktop_color);
      return true;

    case JM_KEYPRESSED:
    case JM_KEYRELEASED: {
      JLink link, link2;

      msg->key.propagate_to_children = true;
      msg->key.propagate_to_parent = false;

      /* continue sending the message to the children of all windows
	 (until a desktop or foreground window) */
      JI_LIST_FOR_EACH(widget->children, link) {
	Frame* w = (Frame*)link->data;

	/* send to the window */
	JI_LIST_FOR_EACH(w->children, link2)
	  if (jwidget_send_message(reinterpret_cast<JWidget>(link2->data), msg))
	    return true;

	if (w->is_foreground() ||
	    w->is_desktop())
	  break;
      }

      /* check the focus movement */
      if (msg->type == JM_KEYPRESSED)
	move_focus(widget, msg);

      return true;
    }
      
  }

  return false;
}

static void manager_request_size(JWidget widget, int *w, int *h)
{
  if (!widget->parent) {	/* hasn't parent? */
    *w = jrect_w(widget->rc);
    *h = jrect_h(widget->rc);
  }
  else {
    JRect cpos, pos = jwidget_get_child_rect(widget->parent);
    JLink link;

    JI_LIST_FOR_EACH(widget->children, link) {
      cpos = jwidget_get_rect(reinterpret_cast<JWidget>(link->data));
      jrect_union(pos, cpos);
      jrect_free(cpos);
    }

    *w = jrect_w(pos);
    *h = jrect_h(pos);
    jrect_free(pos);
  }
}

static void manager_set_position(JWidget widget, JRect rect)
{
  JRect cpos, old_pos;
  JWidget child;
  JLink link;
  int x, y;

  old_pos = jrect_new_copy(widget->rc);
  jrect_copy(widget->rc, rect);

  /* offset for all windows */
  x = widget->rc->x1 - old_pos->x1;
  y = widget->rc->y1 - old_pos->y1;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    cpos = jwidget_get_rect(child);
    jrect_displace(cpos, x, y);
    jwidget_set_rect(child, cpos);
    jrect_free(cpos);
  }

  jrect_free(old_pos);
}

static void manager_pump_queue(JWidget widget_manager)
{
  JMessage msg, first_msg;
  JLink link, link2, next;
  JWidget widget;
  bool done;
#ifdef LIMIT_DISPATCH_TIME
  int t = ji_clock;
#endif

  /* TODO get the msg_queue from 'widget_manager' */

  ASSERT(msg_queue != NULL);

  link = jlist_first(msg_queue);
  while (link != msg_queue->end) {
#ifdef LIMIT_DISPATCH_TIME
    if (ji_clock-t > 250)
      break;
#endif

    /* the message to process */
    msg = reinterpret_cast<JMessage>(link->data);

    /* go to next message */
    if (msg->any.used) {
      link = link->next;
      continue;
    }

    /* this message is in use */
    msg->any.used = true;
    first_msg = msg;

    done = false;
    JI_LIST_FOR_EACH(msg->any.widgets, link2) {
      widget = reinterpret_cast<JWidget>(link2->data);

#ifdef REPORT_EVENTS
      {
	static char *msg_name[] = {
	  "JM_OPEN",
	  "JM_CLOSE",
	  "JM_DESTROY",
	  "JM_DRAW",
	  "JM_SIGNAL",
	  "JM_TIMER",
	  "JM_REQSIZE",
	  "JM_SETPOS",
	  "JM_WINMOVE",
	  "JM_DEFERREDFREE",
	  "JM_DIRTYCHILDREN",
	  "JM_QUEUEPROCESSING",
	  "JM_KEYPRESSED",
	  "JM_KEYRELEASED",
	  "JM_FOCUSENTER",
	  "JM_FOCUSLEAVE",
	  "JM_BUTTONPRESSED",
	  "JM_BUTTONRELEASED",
	  "JM_DOUBLECLICK",
	  "JM_MOUSEENTER",
	  "JM_MOUSELEAVE",
	  "JM_MOTION",
	  "JM_SETCURSOR",
	  "JM_WHEEL",
	};
	const char *string =
	  (msg->type >= JM_OPEN &&
	   msg->type <= JM_WHEEL) ? msg_name[msg->type]:
				    "Unknown";

	printf("Event #%d: %s (%d)\n", msg->type, string, widget->id);
	fflush(stdout);
      }
#endif

      /* draw message? */
      if (msg->type == JM_DRAW) {
	/* hidden? */
	if (widget->flags & JI_HIDDEN)
	  continue;

	jmouse_hide();
	acquire_bitmap(ji_screen);

	/* set clip */
	ASSERT(get_clip_state(ji_screen));
	set_clip_rect(ji_screen,
		      msg->draw.rect.x1, msg->draw.rect.y1,
		      msg->draw.rect.x2-1, msg->draw.rect.y2-1);
#ifdef REPORT_EVENTS
	printf(" - clip(%d, %d, %d, %d)\n",
	       msg->draw.rect.x1, msg->draw.rect.y1,
	       msg->draw.rect.x2-1, msg->draw.rect.y2-1);
	fflush(stdout);
#endif
	/* rectfill(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1, makecol(255, 0, 0)); */
	/* vsync(); vsync(); vsync(); vsync(); */
      }

      /* call message handler */
      done = jwidget_send_message(widget, msg);

      /* restore clip */
      if (msg->type == JM_DRAW) {
	set_clip_rect(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);

	/* dirty rectangles */
	if (ji_dirty_region)
	  ji_add_dirty_rect(&msg->draw.rect);

	release_bitmap(ji_screen);
	jmouse_show();
      }

      if (done)
	break;
    }

    /* remove the message from the msg_queue */
    next = link->next;
    jlist_delete_link(msg_queue, link);
    link = next;

    /* destroy the message */
    jmessage_free(first_msg);
  }
}

void jmanager_invalidate_region(JWidget widget, JRegion region)
{
  Frame* window;
  JRegion reg1 = jregion_new(NULL, 0);
  JRegion reg2 = jregion_new(widget->rc, 0);
  JRegion reg3;
  JLink link;

  /* TODO intersect with jwidget_get_drawable_region()??? */
  jregion_intersect(reg1, region, reg2);

  /* redraw windows from top to background */
  JI_LIST_FOR_EACH(widget->children, link) {
    window = (Frame*)link->data;

    // invalidate regions of this window
    jwidget_invalidate_region(window, reg1);

    /* there is desktop? */
    if (window->is_desktop())
      break;					/* work done */

    /* clip this window area for the next window */
    reg3 = jwidget_get_region(window);
    jregion_copy(reg2, reg1);
    jregion_subtract(reg1, reg2, reg3);
    jregion_free(reg3);
  }

  // invalidate areas outside windows (only when there are not a desktop window)
  if (link == widget->children->end)
    jwidget_invalidate_region(widget, reg1);

  jregion_free(reg1);
  jregion_free(reg2);
}

/**********************************************************************
			Internal routines
 **********************************************************************/

static void generate_setcursor_message()
{
  JWidget dst;
  JMessage msg;
  
  if (capture_widget)
    dst = capture_widget;
  else
    dst = mouse_widget;

  if (dst) {
    msg = new_mouse_msg(JM_SETCURSOR, dst);
    jmanager_enqueue_message(msg);
  }
  else
    jmouse_set_cursor(JI_CURSOR_NORMAL);
}

static void remove_msgs_for(JWidget widget, JMessage msg)
{
  JLink link, next;

  JI_LIST_FOR_EACH_SAFE(msg->any.widgets, link, next) {
    if (link->data == widget) 
      jlist_delete_link(msg->any.widgets, link);
  }
}

static void generate_proc_windows_list()
{
  jlist_clear(proc_windows_list);
  generate_proc_windows_list2(default_manager);
}

static void generate_proc_windows_list2(JWidget widget)
{
  Frame* window;
  JLink link;

  if (widget->type == JI_MANAGER) {
    JI_LIST_FOR_EACH(widget->children, link) {
      window = reinterpret_cast<Frame*>(link->data);
      jlist_append(proc_windows_list, window);
      if (window->is_foreground() ||
	  window->is_desktop())
	break;
    }
  }

  JI_LIST_FOR_EACH(widget->children, link)
    generate_proc_windows_list2(reinterpret_cast<JWidget>(link->data));
}

static int some_parent_is_focusrest(JWidget widget)
{
  if (jwidget_is_focusrest(widget))
    return true;

  if (widget->parent)
    return some_parent_is_focusrest(widget->parent);
  else
    return false;
}

static JWidget find_magnetic_widget(JWidget widget)
{
  JWidget found;
  JLink link;

  JI_LIST_FOR_EACH(widget->children, link) {
    found = find_magnetic_widget(reinterpret_cast<JWidget>(link->data));
    if (found)
      return found;
  }

  if (jwidget_is_magnetic(widget))
    return widget;
  else
    return NULL;
}

static JMessage new_mouse_msg(int type, JWidget widget)
{
  JMessage msg = jmessage_new(type);
  if (!msg)
    return NULL;

  msg->mouse.x = jmouse_x(0);
  msg->mouse.y = jmouse_y(0);
  msg->mouse.flags =
    type == JM_BUTTONRELEASED ? jmouse_b(1):
				jmouse_b(0);
  msg->mouse.left = msg->mouse.flags & 1 ? true: false;
  msg->mouse.right = msg->mouse.flags & 2 ? true: false;
  msg->mouse.middle = msg->mouse.flags & 4 ? true: false;

  if (widget != NULL)
    jmessage_add_dest(msg, widget);

  return msg;
}

static void broadcast_key_msg(JWidget manager, JMessage msg)
{
  /* send the message to the widget with capture */
  if (capture_widget) {
    jmessage_add_dest(msg, capture_widget);
  }
  /* send the msg to the focused widget */
  else if (focus_widget) {
    jmessage_add_dest(msg, focus_widget);
  }
  /* finally, send the message to the manager, it'll know what to do */
  else {
    jmessage_add_dest(msg, manager);
  }
}

static Filter* filter_new(int message, JWidget widget)
{
  Filter* filter = (Filter*)jnew(Filter, 1);
  if (!filter)
    return NULL;

  filter->message = message;
  filter->widget = widget;

  return filter;
}

static void filter_free(Filter* filter)
{
  jfree(filter);
}

/***********************************************************************
			    Focus Movement
 ***********************************************************************/

static bool move_focus(JWidget manager, JMessage msg)
{
  int (*cmp)(JWidget, int, int) = NULL;
  Widget* focus = NULL;
  Widget* it, **list;
  bool ret = false;
  Frame* window;
  int c, count;

  /* who have the focus */
  if (focus_widget)
    window = static_cast<Frame*>(focus_widget->getRoot());
  else if (!jlist_empty(manager->children))
    window = TOPWND(manager);
  else
    return ret;

  /* how many child-widget want the focus in this widget? */
  count = count_widgets_accept_focus(window);

  /* one at least */
  if (count > 0) {
    /* list of widgets */
    list = (JWidget*)jmalloc(sizeof(JWidget) * count);
    if (!list)
      return ret;

    c = 0;

    /* list's 1st element is the focused widget */
    for (it=focus_widget; it; it=next_widget(it)) {
      if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true)))
	list[c++] = it;
    }

    for (it=window; it != focus_widget; it=next_widget(it)) {
      if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true)))
	list[c++] = it;
    }

    /* depend of the pressed key */
    switch (msg->key.scancode) {

      case KEY_TAB:
        /* reverse tab */
        if (msg->any.shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG)) {
          focus = list[count-1];
        }
        /* normal tab */
        else if (count > 1) {
          focus = list[1];
        }
        ret = true;
        break;

      /* arrow keys */
      case KEY_LEFT:  if (!cmp) cmp = cmp_left;
      case KEY_RIGHT: if (!cmp) cmp = cmp_right;
      case KEY_UP:    if (!cmp) cmp = cmp_up;
      case KEY_DOWN:  if (!cmp) cmp = cmp_down;

        /* more than one widget */
        if (count > 1) {
          int i, j, x, y;

          /* position where the focus come */
          x = ((focus_widget) ? focus_widget->rc->x1+focus_widget->rc->x2:
				window->rc->x1+window->rc->x2)
	    / 2;
          y = ((focus_widget) ? focus_widget->rc->y1+focus_widget->rc->y2:
				window->rc->y1+window->rc->y2)
	    / 2;

          c = focus_widget ? 1: 0;

          /* rearrange the list */
          for (i=c; i<count-1; i++) {
            for (j=i+1; j<count; j++) {
              /* sort the list in ascending order */
              if ((*cmp) (list[i], x, y) > (*cmp) (list[j], x, y)) {
		JWidget tmp = list[i];
                list[i] = list[j];
                list[j] = tmp;
              }
            }
          }

          /* check if the new widget to put the focus is not
             in the wrong way */
          if ((*cmp) (list[c], x, y) < INT_MAX)
            focus = list[c];
        }
        /* if only there are one widget, put the focus in this */
        else
          focus = list[0];

        ret = true;
        break;
    }

    jfree(list);

    if ((focus) && (focus != focus_widget))
      jmanager_set_focus(focus);
  }

  return ret;
}

static int count_widgets_accept_focus(JWidget widget)
{
  int count = 0;
  JLink link;

  JI_LIST_FOR_EACH(widget->children, link)
    count += count_widgets_accept_focus(reinterpret_cast<JWidget>(link->data));

  if ((count == 0) && (ACCEPT_FOCUS(widget)))
    count++;

  return count;
}

static bool childs_accept_focus(JWidget widget, bool first)
{
  JLink link;

  JI_LIST_FOR_EACH(widget->children, link)
    if (childs_accept_focus(reinterpret_cast<JWidget>(link->data), false))
      return true;

  return first ? false: ACCEPT_FOCUS(widget);
}

static JWidget next_widget(JWidget widget)
{
  if (!jlist_empty(widget->children))
    return reinterpret_cast<JWidget>(jlist_first(widget->children)->data);

  while (widget->parent->type != JI_MANAGER) {
    JLink link = jlist_find(widget->parent->children, widget);
    if (link->next != widget->parent->children->end)
      return reinterpret_cast<JWidget>(link->next->data);
    else
      widget = widget->parent;
  }

  return NULL;
}

static int cmp_left(JWidget widget, int x, int y)
{
  int z = x - (widget->rc->x1+widget->rc->x2)/2;
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->rc->y1+widget->rc->y2)/2 - y) * 8;
}

static int cmp_right(JWidget widget, int x, int y)
{
  int z = (widget->rc->x1+widget->rc->x2)/2 - x;
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->rc->y1+widget->rc->y2)/2 - y) * 8;
}

static int cmp_up(JWidget widget, int x, int y)
{
  int z = y - (widget->rc->y1+widget->rc->y2)/2;
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->rc->x1+widget->rc->x2)/2 - x) * 8;
}

static int cmp_down(JWidget widget, int x, int y)
{
  int z = (widget->rc->y1+widget->rc->y2)/2 - y;
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->rc->x1+widget->rc->x2)/2 - x) * 8;
}

