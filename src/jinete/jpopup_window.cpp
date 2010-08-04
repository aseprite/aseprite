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

#include <allegro.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"
#include "Vaca/Size.h"
#include "Vaca/PreferredSizeEvent.h"

PopupWindow::PopupWindow(const char* text, bool close_on_buttonpressed)
  : Frame(false, text)
{
  m_close_on_buttonpressed = close_on_buttonpressed;
  m_hot_region = NULL;
  m_filtering = false;

  set_sizeable(false);
  set_moveable(false);
  set_wantfocus(false);
  setAlign(JI_LEFT | JI_TOP);

  // remove decorative widgets
  JLink link, next;
  JI_LIST_FOR_EACH_SAFE(this->children, link, next)
    jwidget_free(reinterpret_cast<JWidget>(link->data));

  jwidget_init_theme(this);
  jwidget_noborders(this);
}

PopupWindow::~PopupWindow()
{
  if (m_filtering) {
    m_filtering = false;
    jmanager_remove_msg_filter(JM_MOTION, this);
    jmanager_remove_msg_filter(JM_BUTTONPRESSED, this);
    jmanager_remove_msg_filter(JM_KEYPRESSED, this);
  }
  if (m_hot_region != NULL) {
    jregion_free(m_hot_region);
  }
}

/**
 * @param region The new hot-region. This pointer is holded by the @a widget.
 * So you cannot destroy it after calling this routine.
 */
void PopupWindow::setHotRegion(JRegion region)
{
  ASSERT(region != NULL);

  if (m_hot_region != NULL)
    jregion_free(m_hot_region);

  if (!m_filtering) {
    m_filtering = true;
    jmanager_add_msg_filter(JM_MOTION, this);
    jmanager_add_msg_filter(JM_BUTTONPRESSED, this);
    jmanager_add_msg_filter(JM_KEYPRESSED, this);
  }
  m_hot_region = region;
}

bool PopupWindow::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      if (m_filtering) {
	m_filtering = false;
	jmanager_remove_msg_filter(JM_MOTION, this);
	jmanager_remove_msg_filter(JM_BUTTONPRESSED, this);
	jmanager_remove_msg_filter(JM_KEYPRESSED, this);
      }
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	int w = 0, h = 0;

	this->border_width.l = 3 * jguiscale();
	this->border_width.t = 3 * jguiscale();
	this->border_width.r = 3 * jguiscale();
	this->border_width.b = 3 * jguiscale();
	
	if (hasText()) {
	  _ji_theme_textbox_draw(NULL, this, &w, &h, 0, 0);

	  this->border_width.t = h - 3 * jguiscale();
	}
	return true;
      }
      break;

    case JM_MOUSELEAVE:
      if (m_hot_region == NULL)
	closeWindow(NULL);
      break;

    case JM_KEYPRESSED:
      if (m_filtering && msg->key.scancode < KEY_MODIFIERS)
	closeWindow(NULL);
      break;

    case JM_BUTTONPRESSED:
      /* if the user click outside the window, we have to close the
	 tooltip window */
      if (m_filtering) {
	Widget* picked = jwidget_pick(this, msg->mouse.x, msg->mouse.y);
	if (!picked || picked->getRoot() != this) {
	  this->closeWindow(NULL);
	}
      }

      /* this is used when the user click inside a small text
	 tooltip */
      if (m_close_on_buttonpressed)
	this->closeWindow(NULL);
      break;

    case JM_MOTION:
      if (m_hot_region != NULL &&
	  jmanager_get_capture() == NULL) {
	struct jrect box;

	/* if the mouse is outside the hot-region we have to close the window */
	if (!jregion_point_in(m_hot_region,
			      msg->mouse.x, msg->mouse.y, &box)) {
	  this->closeWindow(NULL);
	}
      }
      break;

    case JM_DRAW: {
      JRect pos = jwidget_get_rect(this);
      int oldt;

      jdraw_rect(pos, makecol(0, 0, 0));

      jrect_shrink(pos, 1);
      jdraw_rectfill(pos, this->getBgColor());

      oldt = this->border_width.t;
      this->border_width.t = 3 * jguiscale();
      _ji_theme_textbox_draw(ji_screen, this, NULL, NULL,
			     this->getBgColor(),
			     ji_color_foreground());
      this->border_width.t = oldt;

      jrect_free(pos);
      return true;
    }

  }

  return Frame::onProcessMessage(msg);
}

void PopupWindow::onPreferredSize(PreferredSizeEvent& ev)
{
  Size resultSize(0, 0);

  _ji_theme_textbox_draw(NULL, this, &resultSize.w, &resultSize.h, 0, 0);
  resultSize.h = this->border_width.t + this->border_width.b;

  if (!jlist_empty(this->children)) {
    Size maxSize(0, 0);
    Size reqSize;
    JWidget child;
    JLink link;

    JI_LIST_FOR_EACH(this->children, link) {
      child = (JWidget)link->data;

      reqSize = child->getPreferredSize();

      maxSize.w = MAX(maxSize.w, reqSize.w);
      maxSize.h = MAX(maxSize.h, reqSize.h);
    }

    resultSize.w = MAX(resultSize.w, this->border_width.l + maxSize.w + this->border_width.r);
    resultSize.h += maxSize.h;
  }

  ev.setPreferredSize(resultSize);
}
