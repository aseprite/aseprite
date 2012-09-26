// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_MESSAGE_H_INCLUDED
#define UI_MESSAGE_H_INCLUDED

#include "ui/base.h"
#include "ui/rect.h"
#include "ui/widgets_list.h"

namespace ui {

  class Timer;
  class Widget;

  /* TODO add mutexes */
#define JM_MESSAGE(name)                        \
  static int _jm_##name = 0;                    \
  int jm_##name()                               \
  {                                             \
    if (!_jm_##name)                            \
      _jm_##name = ji_register_message_type();  \
        return _jm_##name;                      \
  }                                             \

  struct MessageAny
  {
    int type;                   // Type of message
    WidgetsList* widgets;       // Destination widgets
    bool used : 1;              // Was used
    int shifts;                 // Key shifts pressed when message was created
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

  struct MessageTimer
  {
    MessageAny any;
    int count;                    // Accumulated calls
    Timer* timer;                 // Timer handle
  };

  struct MessageSetPos
  {
    MessageAny any;
    struct jrect rect;            /* set position */
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
    MessageTimer timer;
    MessageSetPos setpos;
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

} // namespace ui

#endif
