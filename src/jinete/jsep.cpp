/* Jinete - a GUI library
 * Copyright (C) 2003-2009 David Capello.
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

#include "jinete/jlist.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

static bool separator_msg_proc(JWidget widget, JMessage msg);

JWidget ji_separator_new(const char* text, int align)
{
  Widget* widget = new Widget(JI_SEPARATOR);

  jwidget_add_hook(widget, JI_SEPARATOR, separator_msg_proc, NULL);
  widget->setAlign(align);
  widget->setText(text);
  jwidget_init_theme(widget);

  return widget;
}

static bool separator_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      int max_w, max_h;
      int req_w, req_h;
      JWidget child;
      JLink link;

      max_w = max_h = 0;
      JI_LIST_FOR_EACH(widget->children, link) {
	child = (JWidget)link->data;

	jwidget_request_size(child, &req_w, &req_h);
	max_w = MAX(max_w, req_w);
	max_h = MAX(max_h, req_h);
      }

      if (widget->hasText())
	max_w = MAX(max_w, jwidget_get_text_length(widget));

      msg->reqsize.w = widget->border_width.l + max_w + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + max_h + widget->border_width.b;
      return true;
    }

    case JM_DRAW:
      widget->theme->draw_separator(widget, &msg->draw.rect);
      return true;
  }

  return false;
}
