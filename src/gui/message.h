// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_MESSAGE_H_INCLUDED
#define GUI_MESSAGE_H_INCLUDED

#include "gui/base.h"
#include "gui/rect.h"

/* TODO add mutexes */
#define JM_MESSAGE(name)				\
  static int _jm_##name = 0;				\
  int jm_##name()					\
  {							\
    if (!_jm_##name)					\
      _jm_##name = ji_register_message_type();		\
    return _jm_##name;					\
  }							\

struct jmessage_any
{
  int type;		/* type of message */
  JList widgets;	/* destination widgets */
  bool used : 1;	/* was used */
  int shifts;		/* key shifts pressed when message was created */
};

struct jmessage_deffree		/* deferred jwidget_free call */
{
  struct jmessage_any any;
  JWidget widget_to_free;
};

struct jmessage_key
{
  struct jmessage_any any;
  unsigned scancode : 8;	/* Allegro scancode */
  unsigned ascii : 8;		/* ASCII code */
  unsigned repeat;		/* repeat=0 means the first time the key is pressed */
  bool propagate_to_children : 1;
  bool propagate_to_parent : 1;
};

struct jmessage_draw
{
  struct jmessage_any any;
  int count;			/* cound=0 if it's last msg of draw-chain */
  struct jrect rect;		/* area to draw */
};

struct jmessage_mouse
{
  struct jmessage_any any;
  int x, y;			/* mouse position */
  unsigned flags : 4;		/* all buttons */
  bool left : 1;		/* left button */
  bool right : 1;		/* right button */
  bool middle : 1;		/* middle button */
};

struct jmessage_signal
{
  struct jmessage_any any;
  int num;			/* signal number */
  JWidget from;			/* signal generator */
};

struct jmessage_timer
{
  struct jmessage_any any;
  int count;			/* accumulated calls */
  int timer_id;			/* number of timer */
};

struct jmessage_setpos
{
  struct jmessage_any any;
  struct jrect rect;		/* set position */
};

struct jmessage_reqsize
{
  struct jmessage_any any;
  int w, h;			/* requested size */
};

struct jmessage_drawrgn
{
  struct jmessage_any any;
  JRegion region;		/* region to redraw */
};

struct jmessage_user
{
  struct jmessage_any any;
  int a, b, c;
  void *dp;
};

union jmessage
{
  int type;
  struct jmessage_any any;
  struct jmessage_deffree deffree;
  struct jmessage_key key;
  struct jmessage_draw draw;
  struct jmessage_mouse mouse;
  struct jmessage_signal signal;
  struct jmessage_timer timer;
  struct jmessage_setpos setpos;
  struct jmessage_reqsize reqsize;
  struct jmessage_drawrgn drawrgn;
  struct jmessage_user user;
};

int ji_register_message_type();

JMessage jmessage_new(int type);
JMessage jmessage_new_key_related(int type, int readkey_value);
JMessage jmessage_new_copy(const JMessage msg);
JMessage jmessage_new_copy_without_dests(const JMessage msg);
void jmessage_free(JMessage msg);

void jmessage_add_dest(JMessage msg, JWidget widget);
void jmessage_add_pre_dest(JMessage msg, JWidget widget);

void jmessage_broadcast_to_children(JMessage msg, JWidget widget);
void jmessage_broadcast_to_parents(JMessage msg, JWidget widget);

#endif
