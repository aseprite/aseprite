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

#ifndef JINETE_MSG_H
#define JINETE_MSG_H

#include "jinete/jbase.h"
#include "jinete/jrect.h"

JI_BEGIN_DECLS

struct jmessage_any
{
  int type;		/* type of message */
  JList widgets;	/* destination widgets */
  JMessage sub_msg;	/* sub-message if this msg isn't used */
  bool used : 1;	/* was used */
  int shifts;		/* key shifts pressed when message was created */
};

struct jmessage_key
{
  struct jmessage_any any;
  unsigned scancode : 8;	/* Allegro scancode */
  unsigned ascii : 8;		/* ASCII code */
};

struct jmessage_draw
{
  struct jmessage_any any;
  int count;			/* cound=0 if it's last msg of draw-chains */
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

union jmessage
{
  int type;
  struct jmessage_any any;
  struct jmessage_key key;
  struct jmessage_draw draw;
  struct jmessage_mouse mouse;
  struct jmessage_signal signal;
  struct jmessage_setpos setpos;
  struct jmessage_reqsize reqsize;
  struct jmessage_drawrgn drawrgn;
};

JMessage jmessage_new(int type);
JMessage jmessage_new_copy(const JMessage msg);
void jmessage_free(JMessage msg);

void jmessage_add_dest(JMessage msg, JWidget widget);

void jmessage_broadcast_to_children(JMessage msg, JWidget widget);
void jmessage_broadcast_to_parents(JMessage msg, JWidget widget);

void jmessage_set_sub_msg(JMessage msg, JMessage sub_msg);

/* TODO */
/* void jmessage_set_marshal(JMessage msg, JMarshal marshal); */

JI_END_DECLS

#endif /* JINETE_MSG_H */
