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

#ifndef JINETE_JMESSAGE_H_INCLUDED
#define JINETE_JMESSAGE_H_INCLUDED

#include "jinete/jbase.h"
#include "jinete/jrect.h"

/* TODO add mutexes */
#define JM_MESSAGE(name)				\
  static int _jm_##name = 0;				\
  static int jm_##name()				\
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
