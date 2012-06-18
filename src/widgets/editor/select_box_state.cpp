/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "widgets/editor/select_box_state.h"

#include "gfx/rect.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"
#include "widgets/editor/editor.h"

#include <allegro/color.h>

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

bool SelectBoxState::onMouseDown(Editor* editor, Message* msg)
{
  if (msg->mouse.left || msg->mouse.right) {
    m_movingRuler = -1;

    for (int i=0; i<(int)m_rulers.size(); ++i) {
      if (touchRuler(editor, m_rulers[i], msg->mouse.x, msg->mouse.y)) {
        m_movingRuler = i;
        break;
      }
    }

    editor->captureMouse();
    return true;
  }

  return StandbyState::onMouseDown(editor, msg);
}

bool SelectBoxState::onMouseUp(Editor* editor, Message* msg)
{
  m_movingRuler = -1;
  return StandbyState::onMouseUp(editor, msg);
}

bool SelectBoxState::onMouseMove(Editor* editor, Message* msg)
{
  if (m_movingRuler >= 0) {
    int u, v;
    editor->screenToEditor(msg->mouse.x, msg->mouse.y, &u, &v);

    switch (m_rulers[m_movingRuler].getOrientation()) {

      case Ruler::Horizontal:
        m_rulers[m_movingRuler].setPosition(v);
        break;

      case Ruler::Vertical:
        m_rulers[m_movingRuler].setPosition(u);
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
      case Ruler::Horizontal: jmouse_set_cursor(JI_CURSOR_SIZE_T); return true;
      case Ruler::Vertical: jmouse_set_cursor(JI_CURSOR_SIZE_L); return true;
    }
  }

  int x = jmouse_x(0);
  int y = jmouse_y(0);

  for (Rulers::iterator it = m_rulers.begin(), end = m_rulers.end(); it != end; ++it) {
    if (touchRuler(editor, *it, x, y)) {
      switch (it->getOrientation()) {
        case Ruler::Horizontal:
          jmouse_set_cursor(JI_CURSOR_SIZE_T);
          return true;
        case Ruler::Vertical:
          jmouse_set_cursor(JI_CURSOR_SIZE_L);
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
  Image* image = render->getImage();
  Sprite* sprite = render->getEditor()->getSprite();
  int sprite_w = sprite->getWidth();
  int sprite_h = sprite->getHeight();

  // Top band
  if (rc.y > 0)
    render->fillRect(gfx::Rect(0, 0, sprite_w, rc.y), _rgba(0, 0, 0, 255), 128);

  // Bottom band
  if (rc.y+rc.h < sprite_h)
    render->fillRect(gfx::Rect(0, rc.y+rc.h, sprite_w, sprite_h-(rc.y+rc.h)), _rgba(0, 0, 0, 255), 128);

  // Left band
  if (rc.x > 0)
    render->fillRect(gfx::Rect(0, rc.y, rc.x, rc.h), _rgba(0, 0, 0, 255), 128);

  // Right band
  if (rc.x+rc.w < sprite_w)
    render->fillRect(gfx::Rect(rc.x+rc.w, rc.y, sprite_w-(rc.x+rc.w), rc.h), _rgba(0, 0, 0, 255), 128);
}

void SelectBoxState::postRenderDecorator(EditorPostRender* render)
{
  Editor* editor = render->getEditor();
  int zoom = editor->getZoom();
  gfx::Rect vp = View::getView(editor)->getViewportBounds();

  vp.w += 1<<zoom;
  vp.h += 1<<zoom;
  editor->screenToEditor(vp, &vp);

  // Paint a grid generated by the box
  if (hasPaintFlag(PaintGrid)) {
    int gridColor = makecol(100, 200, 100);
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
    int rulerColor = makecol(0, 0, 255);

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
  int u, v;
  editor->editorToScreen(ruler.getPosition(), ruler.getPosition(), &u, &v);

  switch (ruler.getOrientation()) {
    case Ruler::Horizontal: return (y >= v-2 && y <= v+2);
    case Ruler::Vertical:   return (x >= u-2 && x <= u+2);
  }

  return false;
}

bool SelectBoxState::hasPaintFlag(PaintFlags flag) const
{
  return ((m_paintFlags & flag) == flag);
}
