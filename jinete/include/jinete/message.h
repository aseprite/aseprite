/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_MSG_H
#define JINETE_MSG_H

#include "jinete/base.h"
#include "jinete/rect.h"

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

/* XXX */
/* void jmessage_set_marshal(JMessage msg, JMarshal marshal); */

JI_END_DECLS

#endif /* JINETE_MSG_H */
