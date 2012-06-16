// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_MESSAGE_H_INCLUDED
#define GUI_MESSAGE_H_INCLUDED

#include "gui/base.h"
#include "gui/rect.h"

namespace gui { class Timer; }

/* TODO add mutexes */
#define JM_MESSAGE(name)                                \
  static int _jm_##name = 0;                            \
  int jm_##name()                                       \
  {                                                     \
    if (!_jm_##name)                                    \
      _jm_##name = ji_register_message_type();          \
    return _jm_##name;                                  \
  }                                                     \

struct MessageAny
{
  int type;             /* type of message */
  JList widgets;        /* destination widgets */
  bool used : 1;        /* was used */
  int shifts;           /* key shifts pressed when message was created */
};

struct MessageKey
{
  MessageAny any;
  unsigned scancode : 8;        /* Allegro scancode */
  unsigned ascii : 8;           /* ASCII code */
  unsigned repeat;              /* repeat=0 means the first time the key is pressed */
  bool propagate_to_children : 1;
  bool propagate_to_parent : 1;
};

struct MessageDraw
{
  MessageAny any;
  int count;                    /* cound=0 if it's last msg of draw-chain */
  struct jrect rect;            /* area to draw */
};

struct MessageMouse
{
  MessageAny any;
  int x, y;                     /* mouse position */
  unsigned flags : 4;           /* all buttons */
  bool left : 1;                /* left button */
  bool right : 1;               /* right button */
  bool middle : 1;              /* middle button */
};

struct MessageSignal
{
  MessageAny any;
  int num;                      /* signal number */
  Widget* from;                 /* signal generator */
};

struct MessageTimer
{
  MessageAny any;
  int count;                    // Accumulated calls
  gui::Timer* timer;            // Timer handle
};

struct MessageSetPos
{
  MessageAny any;
  struct jrect rect;            /* set position */
};

struct MessageReqSize
{
  MessageAny any;
  int w, h;                     /* requested size */
};

struct MessageDrawRgn
{
  MessageAny any;
  JRegion region;               /* region to redraw */
};

struct MessageUser
{
  MessageAny any;
  int a, b, c;
  void *dp;
};

union Message
{
  int type;
  MessageAny any;
  MessageKey key;
  MessageDraw draw;
  MessageMouse mouse;
  MessageSignal signal;
  MessageTimer timer;
  MessageSetPos setpos;
  MessageReqSize reqsize;
  MessageDrawRgn drawrgn;
  MessageUser user;
};

int ji_register_message_type();

Message* jmessage_new(int type);
Message* jmessage_new_key_related(int type, int readkey_value);
Message* jmessage_new_copy(const Message* msg);
Message* jmessage_new_copy_without_dests(const Message* msg);
void jmessage_free(Message* msg);

void jmessage_add_dest(Message* msg, Widget* widget);
void jmessage_add_pre_dest(Message* msg, Widget* widget);

void jmessage_broadcast_to_children(Message* msg, Widget* widget);
void jmessage_broadcast_to_parents(Message* msg, Widget* widget);

#endif
