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

#include "config.h"

#include <allegro/gfx.h>
#include <allegro/keyboard.h>
#include <allegro/timer.h>
#include <string.h>

#include "jinete/jbutton.h"
#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

typedef void (*command_t)(JWidget widget);
typedef void (*command_data_t)(JWidget widget, void *data);

typedef struct ButtonCommand
{
  bool use_data;
  void *proc;
  void *data;
} ButtonCommand;

typedef struct Button
{
  /* generic */
  bool pressed_status;
  int draw_type;
  BITMAP *icon;
  int icon_align;
  /* button */
    JList commands;
    int bevel[4];
  /* check */
    /* ...nothing... */
  /* radio */
    int group;
} Button;

static bool button_msg_proc(JWidget widget, JMessage msg);
static void button_request_size(JWidget widget, int *w, int *h);
static void button_selected_signal(JWidget widget);
static void button_deselect_group(JWidget widget, int radio_group);

JWidget ji_generic_button_new(const char *text,
			      int behavior_type,
			      int draw_type)
{
  Widget* widget = new Widget(behavior_type);
  Button *button = jnew(Button, 1);

  button->draw_type = draw_type;
  button->icon = NULL;
  button->icon_align = JI_LEFT | JI_MIDDLE;
  button->commands = jlist_new();
  button->bevel[0] = 2;
  button->bevel[1] = 2;
  button->bevel[2] = 2;
  button->bevel[3] = 2;
  button->group = 0;

  jwidget_add_hook(widget, behavior_type, button_msg_proc, button);
  widget->setAlign(JI_CENTER | JI_MIDDLE);
  widget->setText(text);
  jwidget_focusrest(widget, true);

  // initialize theme
  widget->type = button->draw_type;
  jwidget_init_theme(widget);
  widget->type = behavior_type;

  return widget;
}

void ji_generic_button_set_icon(JWidget widget, struct BITMAP *icon)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  button->icon = icon;

  jwidget_dirty(widget);
}

void ji_generic_button_set_icon_align(JWidget widget, int icon_align)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  button->icon_align = icon_align;

  jwidget_dirty(widget);
}

BITMAP *ji_generic_button_get_icon(JWidget widget)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  return button->icon;
}

int ji_generic_button_get_icon_align(JWidget widget)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  return button->icon_align;
}

/**********************************************************************/
/* button */

JWidget jbutton_new(const char *text)
{
  JWidget widget = ji_generic_button_new(text, JI_BUTTON, JI_BUTTON);
  if (widget)
    widget->setAlign(JI_CENTER | JI_MIDDLE);
  return widget;
}

void jbutton_set_bevel(JWidget widget, int b0, int b1, int b2, int b3)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  button->bevel[0] = b0;
  button->bevel[1] = b1;
  button->bevel[2] = b2;
  button->bevel[3] = b3;

  jwidget_dirty(widget);
}

void jbutton_get_bevel(JWidget widget, int *b4)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  b4[0] = button->bevel[0];
  b4[1] = button->bevel[1];
  b4[2] = button->bevel[2];
  b4[3] = button->bevel[3];
}

void jbutton_add_command(JWidget widget,
			 void (*command_proc)(JWidget widget))
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));
  ButtonCommand* command = (ButtonCommand*)jnew(ButtonCommand, 1);

  command->use_data = false;
  command->proc = (void *)command_proc;
  command->data = NULL;

  jlist_prepend(button->commands, command);
}

void jbutton_add_command_data(JWidget widget,
			      void (*command_proc)(JWidget widget,
						   void *data),
			      void *data)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));
  ButtonCommand* command = jnew(ButtonCommand, 1);

  command->use_data = true;
  command->proc = (void *)command_proc;
  command->data = data;

  jlist_prepend(button->commands, command);
}

/**********************************************************************/
/* check */

JWidget jcheck_new(const char *text)
{
  JWidget widget = ji_generic_button_new(text, JI_CHECK, JI_CHECK);
  if (widget) {
    widget->setAlign(JI_LEFT | JI_MIDDLE);
  }
  return widget;
}

/**********************************************************************/
/* radio */

JWidget jradio_new(const char *text, int radio_group)
{
  JWidget widget = ji_generic_button_new(text, JI_RADIO, JI_RADIO);
  if (widget) {
    widget->setAlign(JI_LEFT | JI_MIDDLE);
    jradio_set_group(widget, radio_group);
  }
  return widget;
}

void jradio_set_group(JWidget widget, int radio_group)
{
  Button* radio = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  radio->group = radio_group;

  /* TODO: update old and new groups */
}

int jradio_get_group(JWidget widget)
{
  Button* radio = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  return radio->group;
}

void jradio_deselect_group(JWidget widget)
{
  Frame* window = static_cast<Frame*>(widget->getRoot());
  if (window)
    button_deselect_group(window, jradio_get_group(widget));
}

/**********************************************************************/
/* procedures */

static bool button_msg_proc(JWidget widget, JMessage msg)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

  switch (msg->type) {

    case JM_DESTROY: {
      JLink link;

      JI_LIST_FOR_EACH(button->commands, link)
	jfree(link->data);

      jlist_free(button->commands);
      jfree(button);
      break;
    }

    case JM_REQSIZE:
      button_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_DRAW: {
      switch (button->draw_type) {
	case JI_BUTTON: widget->theme->draw_button(widget, &msg->draw.rect); break;
	case JI_CHECK:  widget->theme->draw_check(widget, &msg->draw.rect); break;
	case JI_RADIO:  widget->theme->draw_radio(widget, &msg->draw.rect); break;
      }
      return true;
    }

    case JM_SIGNAL:
      if (widget->type == JI_RADIO) {
	if (msg->signal.num == JI_SIGNAL_SELECT) {
	  jradio_deselect_group(widget);

	  jwidget_signal_off(widget);
	  widget->setSelected(true);
	  jwidget_signal_on(widget);
	}
      }
      break;

    case JM_FOCUSENTER:
    case JM_FOCUSLEAVE:
      if (widget->isEnabled()) {
	if (widget->type == JI_BUTTON) {
	  /* deselect the widget (maybe the user press the key, but
	     before release it, changes the focus) */
	  if (widget->isSelected())
	    widget->setSelected(false);
	}

	/* TODO theme specific stuff */
	jwidget_dirty(widget);
      }
      break;

    case JM_KEYPRESSED:
      /* if the button is enabled */
      if (widget->isEnabled()) {
	/* for JI_BUTTON */
	if (widget->type == JI_BUTTON) {
	  /* has focus and press enter/space */
	  if (widget->hasFocus()) {
	    if ((msg->key.scancode == KEY_ENTER) ||
		(msg->key.scancode == KEY_ENTER_PAD) ||
		(msg->key.scancode == KEY_SPACE)) {
	      widget->setSelected(true);
	      return true;
	    }
	  }
	  /* the underscored letter with Alt */
	  if ((msg->any.shifts & KB_ALT_FLAG) &&
	      (jwidget_check_underscored(widget, msg->key.scancode))) {
	    widget->setSelected(true);
	    return true;
	  }
	  /* magnetic */
	  else if (jwidget_is_magnetic(widget) &&
		   ((msg->key.scancode == KEY_ENTER) ||
		    (msg->key.scancode == KEY_ENTER_PAD))) {
	    jmanager_set_focus(widget);

	    /* dispatch focus movement messages (because the buttons
	       process them) */
	    jmanager_dispatch_messages(ji_get_default_manager());

	    widget->setSelected(true);
	    return true;
	  }
	}
	/* for JI_CHECK or JI_RADIO */
	else {
	  /* if the widget has the focus and the user press space or
	     if the user press Alt+the underscored letter of the button */
	  if ((widget->hasFocus() &&
	       (msg->key.scancode == KEY_SPACE)) ||
	      ((msg->any.shifts & KB_ALT_FLAG) &&
	       (jwidget_check_underscored(widget, msg->key.scancode)))) {
	    if (widget->type == JI_CHECK) {
	      // Swap the select status
	      widget->setSelected(!widget->isSelected());
	      
	      // Signal
	      jwidget_emit_signal(widget, JI_SIGNAL_CHECK_CHANGE);
	      jwidget_dirty(widget);
	    }
	    else if (widget->type == JI_RADIO) {
	      if (!widget->isSelected()) {
		widget->setSelected(true);
		jwidget_emit_signal(widget, JI_SIGNAL_RADIO_CHANGE);
	      }
	    }
	    return true;
	  }
	}
      }
      break;

    case JM_KEYRELEASED:
      if (widget->isEnabled()) {
	if (widget->type == JI_BUTTON) {
	  if (widget->isSelected()) {
	    button_selected_signal(widget);
	    return true;
	  }
	}
      }
      break;

    case JM_BUTTONPRESSED:
      switch (widget->type) {

	case JI_BUTTON:
	  if (widget->isEnabled()) {
	    widget->setSelected(true);

	    button->pressed_status = widget->isSelected();
	    widget->captureMouse();
	  }
	  return true;

	case JI_CHECK:
	  if (widget->isEnabled()) {
	    widget->setSelected(!widget->isSelected());

	    button->pressed_status = widget->isSelected();
	    widget->captureMouse();
	  }
	  return true;

	case JI_RADIO:
	  if (widget->isEnabled()) {
	    if (!widget->isSelected()) {
	      jwidget_signal_off(widget);
	      widget->setSelected(true);
	      jwidget_signal_on(widget);

	      button->pressed_status = widget->isSelected();
	      widget->captureMouse();
	    }
	  }
	  return true;
      }
      break;

    case JM_BUTTONRELEASED:
      if (widget->hasCapture()) {
	widget->releaseMouse();

	if (widget->hasMouseOver()) {
	  switch (widget->type) {

	    case JI_BUTTON:
	      button_selected_signal(widget);
	      break;

	    case JI_CHECK:
	      jwidget_emit_signal(widget, JI_SIGNAL_CHECK_CHANGE);
	      jwidget_dirty(widget);
	      break;

	    case JI_RADIO:
	      widget->setSelected(false);
	      widget->setSelected(true);
	      jwidget_emit_signal(widget, JI_SIGNAL_RADIO_CHANGE);
	      break;
	  }
	}
	return true;
      }
      break;

    case JM_MOTION:
      if (widget->isEnabled() && widget->hasCapture()) {
	bool hasMouse = widget->hasMouseOver();

    	// Switch state when the mouse go out
    	if (( hasMouse && widget->isSelected() != button->pressed_status) ||
    	    (!hasMouse && widget->isSelected() == button->pressed_status)) {
    	  jwidget_signal_off(widget);

    	  if (hasMouse) {
    	    widget->setSelected(button->pressed_status);
    	  }
    	  else {
    	    widget->setSelected(!button->pressed_status);
    	  }

    	  jwidget_signal_on(widget);
    	}
      }
      break;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      // TODO theme stuff
      if (widget->isEnabled())
	jwidget_dirty(widget);
      break;
  }

  return false;
}

static void button_request_size(JWidget widget, int *w, int *h)
{
  Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));
  struct jrect box, text, icon;
  int icon_w = 0;
  int icon_h = 0;

  switch (button->draw_type) {

    case JI_BUTTON:
      if (button->icon) {
	icon_w = button->icon->w;
	icon_h = button->icon->h;
      }
      break;

    case JI_CHECK:
      icon_w = widget->theme->check_icon_size;
      icon_h = widget->theme->check_icon_size;
      break;

    case JI_RADIO:
      icon_w = widget->theme->radio_icon_size;
      icon_h = widget->theme->radio_icon_size;
      break;
  }

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			    button->icon_align, icon_w, icon_h);

  *w = widget->border_width.l + jrect_w(&box) + widget->border_width.r;
  *h = widget->border_width.t + jrect_h(&box) + widget->border_width.b;
}

static void button_selected_signal(JWidget widget)
{
  // deselect
  widget->setSelected(false);

  // emit the signal
  bool used = jwidget_emit_signal(widget, JI_SIGNAL_BUTTON_SELECT);

  // not used?
  if (!used) {
    Button* button = reinterpret_cast<Button*>(jwidget_get_data(widget, widget->type));

    /* call commands */
    if (!jlist_empty(button->commands)) {
      ButtonCommand* command;
      JLink link;

      JI_LIST_FOR_EACH(button->commands, link) {
	command = reinterpret_cast<ButtonCommand*>(link->data);

	if (command->proc) {
	  if (command->use_data)
	    (*(command_data_t)command->proc)(widget, command->data);
	  else
	    (*(command_t)command->proc)(widget);
	}
      }
    }
    /* default action: close the window */
    else
      jwidget_close_window(widget);
  }
}

static void button_deselect_group(JWidget widget, int radio_group)
{
  JLink link;

  JI_LIST_FOR_EACH(widget->children, link)
    button_deselect_group(reinterpret_cast<JWidget>(link->data), radio_group);

  if (widget->type == JI_RADIO) {
    if (jradio_get_group(widget) == radio_group)
      widget->setSelected(false);
  }
}
