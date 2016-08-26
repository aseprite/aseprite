// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/select_box_state.h"

#include "app/app.h"
#include "app/tools/tool_box.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "gfx/rect.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;

SelectBoxState::SelectBoxState(SelectBoxDelegate* delegate, const gfx::Rect& rc, Flags flags)
  : m_delegate(delegate)
  , m_rulers(4)
  , m_movingRuler(-1)
  , m_selectingBox(false)
  , m_flags(flags)
{
  setBoxBounds(rc);
}

SelectBoxState::~SelectBoxState()
{
  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForActiveTool();
}

void SelectBoxState::setFlags(Flags flags)
{
  m_flags = flags;
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

void SelectBoxState::onEnterState(Editor* editor)
{
  StandbyState::onEnterState(editor);

  updateContextBar();

  editor->setDecorator(this);
  editor->invalidate();
}

void SelectBoxState::onBeforePopState(Editor* editor)
{
  editor->setDecorator(NULL);
  editor->invalidate();
}

bool SelectBoxState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (msg->left() || msg->right()) {
    m_movingRuler = -1;

    if (hasFlag(Flags::Rulers)) {
      for (int i=0; i<(int)m_rulers.size(); ++i) {
        if (touchRuler(editor, m_rulers[i], msg->position().x, msg->position().y)) {
          m_movingRuler = i;
          break;
        }
      }
    }

    if (hasFlag(Flags::QuickBox) && m_movingRuler == -1) {
      m_selectingBox = true;
      m_selectingButtons = msg->buttons();
      m_startingPos = editor->screenToEditor(msg->position());
      setBoxBounds(gfx::Rect(m_startingPos, gfx::Size(1, 1)));
    }

    editor->captureMouse();
    return true;
  }
  return StandbyState::onMouseDown(editor, msg);
}

bool SelectBoxState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  m_movingRuler = -1;

  if (m_selectingBox) {
    m_selectingBox = false;

    if (m_delegate) {
      if (m_selectingButtons == msg->buttons())
        m_delegate->onQuickboxEnd(editor, getBoxBounds(), msg->buttons());
      else
        m_delegate->onQuickboxCancel(editor);
    }
  }

  return StandbyState::onMouseUp(editor, msg);
}

bool SelectBoxState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  bool used = false;

  updateContextBar();

  if (hasFlag(Flags::Rulers) && m_movingRuler >= 0) {
    gfx::Point pt = editor->screenToEditor(msg->position());

    switch (m_rulers[m_movingRuler].getOrientation()) {

      case Ruler::Horizontal:
        m_rulers[m_movingRuler].setPosition(pt.y);
        break;

      case Ruler::Vertical:
        m_rulers[m_movingRuler].setPosition(pt.x);
        break;
    }
    used = true;
  }

  if (hasFlag(Flags::QuickBox) && m_selectingBox) {
    gfx::Point p1 = m_startingPos;
    gfx::Point p2 = editor->screenToEditor(msg->position());

    if (p2.x < p1.x) std::swap(p1.x, p2.x);
    if (p2.y < p1.y) std::swap(p1.y, p2.y);
    ++p2.x;
    ++p2.y;

    setBoxBounds(gfx::Rect(p1, p2));
    used = true;
  }

  if (used) {
    if (m_delegate)
      m_delegate->onChangeRectangle(getBoxBounds());

    editor->invalidate();
    return true;
  }
  else
    return StandbyState::onMouseMove(editor, msg);
}

bool SelectBoxState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  if (hasFlag(Flags::Rulers)) {
    if (m_movingRuler >= 0) {
      switch (m_rulers[m_movingRuler].getOrientation()) {

        case Ruler::Horizontal:
          editor->showMouseCursor(kSizeNSCursor);
          return true;

        case Ruler::Vertical:
          editor->showMouseCursor(kSizeWECursor);
          return true;
      }
    }

    for (Rulers::iterator it = m_rulers.begin(), end = m_rulers.end(); it != end; ++it) {
      if (touchRuler(editor, *it, mouseScreenPos.x, mouseScreenPos.y)) {
        switch (it->getOrientation()) {
          case Ruler::Horizontal:
            editor->showMouseCursor(kSizeNSCursor);
            return true;
          case Ruler::Vertical:
            editor->showMouseCursor(kSizeWECursor);
            return true;
        }
      }
    }
  }

  if (!requireBrushPreview()) {
    editor->showMouseCursor(kArrowCursor);
    return true;
  }

  return StandbyState::onSetCursor(editor, mouseScreenPos);
}

bool SelectBoxState::acceptQuickTool(tools::Tool* tool)
{
  return false;
}

bool SelectBoxState::requireBrushPreview()
{
  if (hasFlag(Flags::QuickBox))
    return true;

  // Returns false as it overrides default standby state behavior &
  // look. This state uses normal arrow cursors.
  return false;
}

tools::Ink* SelectBoxState::getStateInk()
{
  if (hasFlag(Flags::QuickBox))
    return App::instance()->toolBox()->getInkById(
      tools::WellKnownInks::Selection);
  else
    return nullptr;
}

void SelectBoxState::preRenderDecorator(EditorPreRender* render)
{
  // Without black shadow?
  if (!hasFlag(Flags::DarkOutside))
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
  render::Zoom zoom = editor->zoom();
  gfx::Rect sp = editor->sprite()->bounds();
  gfx::Rect vp = View::getView(editor)->viewportBounds();
  vp.w += zoom.apply(1);
  vp.h += zoom.apply(1);
  vp = editor->screenToEditor(vp);

  // Paint a grid generated by the box
  gfx::Color rulerColor = skin::SkinTheme::instance()->colors.selectBoxRuler();
  gfx::Color gridColor = skin::SkinTheme::instance()->colors.selectBoxGrid();
  gfx::Rect boxBounds = getBoxBounds();

  if (hasFlag(Flags::Grid)) {
    if (boxBounds.w > 0) {
      for (int x=boxBounds.x+boxBounds.w*2; x<=sp.x+sp.w; x+=boxBounds.w)
        render->drawLine(x, boxBounds.y, x, sp.y+sp.h, gridColor);
    }

    if (boxBounds.h > 0) {
      for (int y=boxBounds.y+boxBounds.h*2; y<=sp.y+sp.h; y+=boxBounds.h)
        render->drawLine(boxBounds.x, y, sp.x+sp.w, y, gridColor);
    }
  }
  else if (hasFlag(Flags::HGrid)) {
    if (boxBounds.w > 0) {
      for (int x=boxBounds.x+boxBounds.w*2; x<=sp.x+sp.w; x+=boxBounds.w)
        render->drawLine(x, boxBounds.y, x, boxBounds.y+boxBounds.h, gridColor);
    }
  }
  else if (hasFlag(Flags::VGrid)) {
    if (boxBounds.h > 0) {
      for (int y=boxBounds.y+boxBounds.h*2; y<=sp.y+sp.h; y+=boxBounds.h)
        render->drawLine(boxBounds.x, y, boxBounds.x+boxBounds.w, y, gridColor);
    }
  }

  // Draw the rulers enclosing the box
  if (hasFlag(Flags::Rulers)) {
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

  if (hasFlag(Flags::QuickBox)) {
    render->drawRectXor(boxBounds);
  }
}

void SelectBoxState::getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region)
{
  // Do nothing
}

void SelectBoxState::updateContextBar()
{
  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForSelectingBox(m_delegate->onGetContextBarHelp());
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

bool SelectBoxState::hasFlag(Flags flag) const
{
  return (int(m_flags) & int(flag)) == int(flag);
}

} // namespace app
