// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>

#include "gfx/size.h"
#include "ui/graphics.h"
#include "ui/gui.h"
#include "ui/intern.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"

using namespace gfx;

namespace ui {

PopupWindow::PopupWindow(const char* text, bool close_on_buttonpressed)
  : Window(false, text)
{
  m_close_on_buttonpressed = close_on_buttonpressed;
  m_hot_region = NULL;
  m_filtering = false;

  set_sizeable(false);
  set_moveable(false);
  set_wantfocus(false);
  setAlign(JI_LEFT | JI_TOP);

  removeDecorativeWidgets();

  initTheme();
  jwidget_noborders(this);
}

PopupWindow::~PopupWindow()
{
  stopFilteringMessages();

  if (m_hot_region != NULL)
    jregion_free(m_hot_region);
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

  startFilteringMessages();

  m_hot_region = region;
}

void PopupWindow::makeFloating()
{
  stopFilteringMessages();
  set_moveable(true);
}

void PopupWindow::makeFixed()
{
  startFilteringMessages();
  set_moveable(false);
}

bool PopupWindow::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      stopFilteringMessages();
      break;

    case JM_MOUSELEAVE:
      if (m_hot_region == NULL && !is_moveable())
        closeWindow(NULL);
      break;

    case JM_KEYPRESSED:
      if (m_filtering) {
        if (msg->key.scancode == KEY_ESC ||
            msg->key.scancode == KEY_ENTER ||
            msg->key.scancode == KEY_ENTER_PAD) {
          closeWindow(NULL);
        }

        // If we are filtering messages we don't propagate key-events
        // to other widgets. As we're a popup window and we're
        // filtering messages, the user shouldn't be able to start
        // other actions pressing keyboard shortcuts.
        return false;
      }
      break;

    case JM_BUTTONPRESSED:
      // If the user click outside the window, we have to close the
      // tooltip window.
      if (m_filtering) {
        Widget* picked = this->pick(msg->mouse.x, msg->mouse.y);
        if (!picked || picked->getRoot() != this) {
          closeWindow(NULL);
        }
      }

      // This is used when the user click inside a small text tooltip.
      if (m_close_on_buttonpressed)
        closeWindow(NULL);
      break;

    case JM_MOTION:
      if (!is_moveable() &&
          m_hot_region != NULL &&
          getManager()->getCapture() == NULL) {
        struct jrect box;

        // If the mouse is outside the hot-region we have to close the
        // window.
        if (!jregion_point_in(m_hot_region, msg->mouse.x, msg->mouse.y, &box))
          closeWindow(NULL);
      }
      break;

  }

  return Window::onProcessMessage(msg);
}

void PopupWindow::onPreferredSize(PreferredSizeEvent& ev)
{
  ScreenGraphics g;
  g.setFont(getFont());
  Size resultSize(0, 0);

  if (hasText())
    resultSize = g.fitString(getText(),
                             (getClientBounds() - getBorder()).w,
                             getAlign());

  resultSize.w += border_width.l + border_width.r;
  resultSize.h += border_width.t + border_width.b;

  if (!getChildren().empty()) {
    Size maxSize(0, 0);
    Size reqSize;

    UI_FOREACH_WIDGET(getChildren(), it) {
      Widget* child = *it;

      reqSize = child->getPreferredSize();

      maxSize.w = MAX(maxSize.w, reqSize.w);
      maxSize.h = MAX(maxSize.h, reqSize.h);
    }

    resultSize.w = MAX(resultSize.w, border_width.l + maxSize.w + border_width.r);
    resultSize.h += maxSize.h;
  }

  ev.setPreferredSize(resultSize);
}

void PopupWindow::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  gfx::Rect pos = getClientBounds();

  g->drawRect(makecol(0, 0, 0), pos);
  pos.shrink(1);

  g->fillRect(this->getBgColor(), pos);
  pos.shrink(getBorder());

  g->drawString(getText(), ji_color_foreground(), this->getBgColor(), pos, getAlign());
}

void PopupWindow::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  this->border_width.l = 3 * jguiscale();
  this->border_width.t = 3 * jguiscale();
  this->border_width.r = 3 * jguiscale();
  this->border_width.b = 3 * jguiscale();
}

void PopupWindow::startFilteringMessages()
{
  if (!m_filtering) {
    m_filtering = true;

    Manager* manager = Manager::getDefault();
    manager->addMessageFilter(JM_MOTION, this);
    manager->addMessageFilter(JM_BUTTONPRESSED, this);
    manager->addMessageFilter(JM_KEYPRESSED, this);
  }
}

void PopupWindow::stopFilteringMessages()
{
  if (m_filtering) {
    m_filtering = false;

    Manager* manager = Manager::getDefault();
    manager->removeMessageFilter(JM_MOTION, this);
    manager->removeMessageFilter(JM_BUTTONPRESSED, this);
    manager->removeMessageFilter(JM_KEYPRESSED, this);
  }
}

} // namespace ui
