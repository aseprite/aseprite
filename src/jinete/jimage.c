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

#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "jinete/jdraw.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

static bool image_msg_proc (JWidget widget, JMessage msg);

JWidget jimage_new(BITMAP *bmp, int align)
{
  JWidget widget = jwidget_new(JI_IMAGE);

  jwidget_add_hook(widget, JI_IMAGE, image_msg_proc, bmp);
  jwidget_set_align(widget, align);

  return widget;
}

static bool image_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      BITMAP *bmp = jwidget_get_data(widget, JI_IMAGE);
      struct jrect box, text, icon;

      jwidget_get_texticon_info(widget, &box, &text, &icon,
				jwidget_get_align (widget),
				bmp->w, bmp->h);

      msg->reqsize.w = widget->border_width.l + jrect_w(&box) + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + jrect_h(&box) + widget->border_width.b;
      return TRUE;
    }

    case JM_DRAW: {
      BITMAP *bmp = jwidget_get_data(widget, JI_IMAGE);
      struct jrect box, text, icon;

      jwidget_get_texticon_info(widget, &box, &text, &icon,
				jwidget_get_align (widget),
				bmp->w, bmp->h);

      jdraw_rectexclude(widget->rc, &icon,
			jwidget_get_bg_color(widget));

      blit(bmp, ji_screen, 0, 0,
	   icon.x1, icon.y1, jrect_w(&icon), jrect_h(&icon));
      return TRUE;
    }
  }

  return FALSE;
}
