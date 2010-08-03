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

#include "jinete/jlist.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"
#include "Vaca/Size.h"

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
      Size maxSize(0, 0);
      Size reqSize;
      JWidget child;
      JLink link;

      JI_LIST_FOR_EACH(widget->children, link) {
	child = (JWidget)link->data;

	reqSize = child->getPreferredSize();
	maxSize.w = MAX(maxSize.w, reqSize.w);
	maxSize.h = MAX(maxSize.h, reqSize.h);
      }

      if (widget->hasText())
	maxSize.w = MAX(maxSize.w, jwidget_get_text_length(widget));

      msg->reqsize.w = widget->border_width.l + maxSize.w + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + maxSize.h + widget->border_width.b;
      return true;
    }

    case JM_DRAW:
      widget->theme->draw_separator(widget, &msg->draw.rect);
      return true;
  }

  return false;
}
