/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/select_box_state.h"

#include "app/ui/editor/editor.h"
#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;

SelectBoxState::SelectBoxState(SelectBoxDelegate* delegate, const gfx::Rect& rc, PaintFlags paintFlags)
  : m_delegate(delegate)
  , m_rulers(4)
  , m_movingRuler(-1)
  , m_paintFlags(paintFlags)
{
  setBoxBounds(rc);
}

gfx::Rect SelectBoxState::getBoxBounds() const
{
  int x1 = std::min(m_rulers[V1].getPosition(), m_rulers[V2].getPosition());
  int y1 = std::min(m_rulers[H1].getPosition(), m_rulers[H2].getPosition());
  int x2 = std::max(m_rulers[V1].getPosition(), m_rulers[V2].getPosition());
  int y2 = std::max(m_rulers[H1].getPosition(), m_rulers[H2].getPosition());
  return gfx::Rect(x1, y1, x2 - x1, y2 - y1);
}

void SelectBoxState::setBoxBounds(const gfx::Rect& box)
{
  m_rulers[H1] = Ruler(Ruler::Horizontal, box.y);
  m_rulers[H2] = Ruler(Ruler::Horizontal, box.y+box.h);
  m_rulers[V1] = Ruler(Ruler::Vertical, box.x);
  m_rulers[V2] = Ruler(Ruler::Vertical, box.x+box.w);
}

void SelectBoxState::onAfterChangeState(Editor* editor)
{
  editor->setDecorator(this);
}

void SelectBoxState::onBeforePopState(Editor* editor)
{
  editor->setDecorator(NULL);
}

bool SelectBoxState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (msg->left() || msg->right()) {
    m_movingRuler = -1;

    for (int i=0; i<(int)m_rulers.size(); ++i) {
      if (touchRuler(editor, m_rulers[i], msg->position().x, msg->position().y)) {
        m_movingRuler = i;
        break;
      }
    }

    editor->captureMouse();
    return true;
  }

  return StandbyState::onMouseDown(editor, msg);
}

bool SelectBoxState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  m_movingRuler = -1;
  return StandbyState::onMouseUp(editor, msg);
}

bool SelectBoxState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  if (m_movingRuler >= 0) {
    gfx::Point pt = editor->screenToEditor(msg->position());

    switch (m_rulers[m_movingRuler].getOrientation()) {

      case Ruler::Horizontal:
        m_rulers[m_movingRuler].setPosition(pt.y);
        break;

      case Ruler::Vertical:
        m_rulers[m_movingRuler].setPosition(pt.x);
        break;
    }

    if (m_delegate)
      m_delegate->onChangeRectangle(getBoxBounds());

    editor->invalidate();
    return true;
  }
  return StandbyState::onMouseMove(editor, msg);
}

bool SelectBoxState::onSetCursor(Editor* editor)
{
  if (m_movingRuler >= 0) {
    switch (m_rulers[m_movingRuler].getOrientation()) {
      case Ruler::Horizontal: ui::set_mouse_cursor(kSizeNSCursor); return true;
      case Ruler::Vertical: ui::set_mouse_cursor(kSizeWECursor); return true;
    }
  }

  int x = ui::get_mouse_position().x;
  int y = ui::get_mouse_position().y;

  for (Rulers::iterator it = m_rulers.begin(), end = m_rulers.end(); it != end; ++it) {
    if (touchRuler(editor, *it, x, y)) {
      switch (it->getOrientation()) {
        case Ruler::Horizontal:
          ui::set_mouse_cursor(kSizeNSCursor);
          return true;
        case Ruler::Vertical:
          ui::set_mouse_cursor(kSizeWECursor);
          return true;
      }
    }
  }

  return false;
}

void SelectBoxState::preRenderDecorator(EditorPreRender* render)
{
  // Without black shadow?
  if (!hasPaintFlag(PaintDarkOutside))
    return;

  gfx::Rect rc = getBoxBounds();
  Sprite* sprite = render->getEditor()->sprite();
  int sprite_w = sprite->width();
  int sprite_h = sprite->height();

  // Top band
  if (rc.y > 0)
    render->fillRect(gfx::Rect(0, 0, sprite_w, rc.y), doc::rgba(0, 0, 0, 255), 128);

  // Bottom band
  if (rc.y+rc.h < sprite_h)
    render->fillRect(gfx::Rect(0, rc.y+rc.h, sprite_w, sprite_h-(rc.y+rc.h)), doc::rgba(0, 0, 0, 255), 128);

  // Left band
  if (rc.x > 0)
    render->fillRect(gfx::Rect(0, rc.y, rc.x, rc.h), doc::rgba(0, 0, 0, 255), 128);

  // Right band
  if (rc.x+rc.w < sprite_w)
    render->fillRect(gfx::Rect(rc.x+rc.w, rc.y, sprite_w-(rc.x+rc.w), rc.h), doc::rgba(0, 0, 0, 255), 128);
}

void SelectBoxState::postRenderDecorator(EditorPostRender* render)
{
  Editor* editor = render->getEditor();
  Zoom zoom = editor->zoom();
  gfx::Rect vp = View::getView(editor)->getViewportBounds();
  vp.w += zoom.apply(1);
  vp.h += zoom.apply(1);
  vp = editor->screenToEditor(vp);

  // Paint a grid generated by the box
  if (hasPaintFlag(PaintGrid)) {
    gfx::Color gridColor = gfx::rgba(100, 200, 100);
    gfx::Rect boxBounds = getBoxBounds();

    if (boxBounds.w > 0)
      for (int x=boxBounds.x+boxBounds.w*2; x<vp.x+vp.w; x+=boxBounds.w)
        render->drawLine(x, boxBounds.y, x, vp.y+vp.h-1, gridColor);

    if (boxBounds.h > 0)
      for (int y=boxBounds.y+boxBounds.h*2; y<vp.y+vp.h; y+=boxBounds.h)
        render->drawLine(boxBounds.x, y, vp.x+vp.w-1, y, gridColor);
  }

  // Draw the rulers enclosing the box
  if (hasPaintFlag(PaintRulers)) {
    gfx::Color rulerColor = gfx::rgba(0, 0, 255);

    for (Rulers::iterator it = m_rulers.begin(), end = m_rulers.end(); it != end; ++it) {
      switch (it->getOrientation()) {

        case Ruler::Horizontal:
          render->drawLine(vp.x, it->getPosition(), vp.x+vp.w-1, it->getPosition(), rulerColor);
          break;

        case Ruler::Vertical:
          render->drawLine(it->getPosition(), vp.y, it->getPosition(), vp.y+vp.h-1, rulerColor);
          break;
      }
    }
  }
}

bool SelectBoxState::touchRuler(Editor* editor, Ruler& ruler, int x, int y)
{
  gfx::Point pt = editor->editorToScreen(
    gfx::Point(ruler.getPosition(), ruler.getPosition()));

  switch (ruler.getOrientation()) {
    case Ruler::Horizontal: return (y >= pt.y-2 && y <= pt.y+2);
    case Ruler::Vertical:   return (x >= pt.x-2 && x <= pt.x+2);
  }

  return false;
}

bool SelectBoxState::hasPaintFlag(PaintFlags flag) const
{
  return ((m_paintFlags & flag) == flag);
}

} // namespace app
