// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "gui/jdraw.h"
#include "gui/jmessage.h"
#include "gui/jrect.h"
#include "gui/jsystem.h"
#include "gui/jtheme.h"
#include "gui/widget.h"

static bool image_msg_proc(JWidget widget, JMessage msg);

JWidget jimage_new(BITMAP *bmp, int align)
{
  JWidget widget = new Widget(JI_IMAGE);

  jwidget_add_hook(widget, JI_IMAGE, image_msg_proc, bmp);
  widget->setAlign(align);

  return widget;
}

static bool image_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      BITMAP* bmp = reinterpret_cast<BITMAP*>(jwidget_get_data(widget, JI_IMAGE));
      struct jrect box, text, icon;

      jwidget_get_texticon_info(widget, &box, &text, &icon,
				widget->getAlign(), bmp->w, bmp->h);

      msg->reqsize.w = widget->border_width.l + jrect_w(&box) + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + jrect_h(&box) + widget->border_width.b;
      return true;
    }

    case JM_DRAW: {
      BITMAP* bmp = reinterpret_cast<BITMAP*>(jwidget_get_data(widget, JI_IMAGE));
      struct jrect box, text, icon;

      jwidget_get_texticon_info(widget, &box, &text, &icon,
				widget->getAlign(), bmp->w, bmp->h);

      jdraw_rectexclude(widget->rc, &icon,
			jwidget_get_bg_color(widget));

      blit(bmp, ji_screen, 0, 0,
	   icon.x1, icon.y1, jrect_w(&icon), jrect_h(&icon));
      return true;
    }
  }

  return false;
}
