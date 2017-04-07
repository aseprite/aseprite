// Aseprite
// Copyright (C) 2001-2017  David Capello
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
  , m_rulersDragAlign(0)
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
  int x1 = std::min(m_rulers[V1].position(), m_rulers[V2].position());
  int y1 = std::min(m_rulers[H1].position(), m_rulers[H2].position());
  int x2 = std::max(m_rulers[V1].position(), m_rulers[V2].position());
  int y2 = std::max(m_rulers[H1].position(), m_rulers[H2].position());
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
    m_startingPos = editor->screenToEditor(msg->position());

    if (hasFlag(Flags::Rulers)) {
      m_rulersDragAlign = hitTestRulers(editor, msg->position(), true);
      if (m_rulersDragAlign)
        m_startRulers = m_rulers; // Capture start positions
    }

    if (hasFlag(Flags::QuickBox) && m_movingRulers.empty()) {
      m_selectingBox = true;
      m_selectingButtons = msg->buttons();
      setBoxBounds(gfx::Rect(m_startingPos, gfx::Size(1, 1)));
    }

    editor->captureMouse();
    return true;
  }
  return StandbyState::onMouseDown(editor, msg);
}

bool SelectBoxState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  m_movingRulers.clear();
  m_rulersDragAlign = 0;

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

  if (hasFlag(Flags::Rulers) && !m_movingRulers.empty()) {
    gfx::Point newPos = editor->screenToEditor(msg->position());
    gfx::Point delta = newPos - m_startingPos;

    for (int i : m_movingRulers) {
      Ruler& ruler = m_rulers[i];
      const Ruler& start = m_startRulers[i];

      switch (ruler.orientation()) {
        case Ruler::Horizontal: ruler.setPosition(start.position() + delta.y); break;
        case Ruler::Vertical: ruler.setPosition(start.position() + delta.x); break;
      }
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
    if (!m_movingRulers.empty()) {
      editor->showMouseCursor(cursorFromAlign(m_rulersDragAlign));
      return true;
    }

    int align = hitTestRulers(editor, mouseScreenPos, false);
    if (align != 0) {
      editor->showMouseCursor(cursorFromAlign(align));
      return true;
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
  render::Projection proj = editor->projection();
  gfx::Rect sp = editor->sprite()->bounds();
  gfx::Rect vp = View::getView(editor)->viewportBounds();
  vp.w += proj.applyX(1);
  vp.h += proj.applyY(1);
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
      switch (it->orientation()) {

        case Ruler::Horizontal:
          render->drawLine(vp.x, it->position(), vp.x+vp.w-1, it->position(), rulerColor);
          break;

        case Ruler::Vertical:
          render->drawLine(it->position(), vp.y, it->position(), vp.y+vp.h-1, rulerColor);
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

int SelectBoxState::hitTestRulers(Editor* editor,
                                  const gfx::Point& mousePos,
                                  const bool updateMovingRulers)
{
  ASSERT(H1 == 0);
  ASSERT(H2 == 1);
  ASSERT(V1 == 2);
  ASSERT(V2 == 3);
  int aligns[] = { TOP, BOTTOM, LEFT, RIGHT };
  int align = 0;

  if (updateMovingRulers)
    m_movingRulers.clear();

  for (int i=0; i<int(m_rulers.size()); ++i) {
    if (hitTestRuler(editor, m_rulers[i], mousePos)) {
      align |= aligns[i];
      if (updateMovingRulers)
        m_movingRulers.push_back(i);
    }
  }

  // Check moving all rulers at the same time
  if (align == 0 && !hasFlag(Flags::QuickBox)) {
    if (editor->editorToScreen(getBoxBounds()).contains(mousePos)) {
      align = LEFT | TOP | RIGHT | BOTTOM;
      if (updateMovingRulers) {
        // Add all rulers
        for (int i=0; i<4; ++i)
          m_movingRulers.push_back(i);
      }
    }
  }

  return align;
}

bool SelectBoxState::hitTestRuler(Editor* editor, const Ruler& ruler,
                                  const gfx::Point& mousePos)
{
  gfx::Point pt = editor->editorToScreen(
    gfx::Point(ruler.position(), ruler.position()));

  switch (ruler.orientation()) {
    case Ruler::Horizontal: return (mousePos.y >= pt.y-2*guiscale() && mousePos.y <= pt.y+2*guiscale());
    case Ruler::Vertical:   return (mousePos.x >= pt.x-2*guiscale() && mousePos.x <= pt.x+2*guiscale());
  }

  return false;
}

bool SelectBoxState::hasFlag(Flags flag) const
{
  return (int(m_flags) & int(flag)) == int(flag);
}

CursorType SelectBoxState::cursorFromAlign(const int align) const
{
  switch (align) {
    case LEFT: return kSizeWCursor;
    case RIGHT: return kSizeECursor;
    case TOP: return kSizeNCursor;
    case TOP | LEFT: return kSizeNWCursor;
    case TOP | RIGHT: return kSizeNECursor;
    case BOTTOM: return kSizeSCursor;
    case BOTTOM | LEFT: return kSizeSWCursor;
    case BOTTOM | RIGHT: return kSizeSECursor;
    case TOP | BOTTOM | LEFT | RIGHT: return kMoveCursor;
  }
  return kArrowCursor;
}

} // namespace app
