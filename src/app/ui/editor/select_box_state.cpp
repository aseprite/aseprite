// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
#include "app/ui_context.h"
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
  , m_rulers((int(flags) & int(Flags::PaddingRulers))? 6: 4)
  , m_rulersDragAlign(0)
  , m_selectingBox(false)
  , m_flags(flags)
{
  setBoxBounds(rc);
  if (hasFlag(Flags::PaddingRulers)) {
    gfx::Size padding(0, 0);
    setPaddingBounds(padding);
  }
}

SelectBoxState::~SelectBoxState()
{
  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForActiveTool();
}

SelectBoxState::Flags SelectBoxState::getFlags()
{
  return m_flags;
}

void SelectBoxState::setFlags(Flags flags)
{
  m_flags = flags;
}

void SelectBoxState::setFlag(Flags flag)
{
  m_flags = Flags(int(flag) | int(m_flags));
}

void SelectBoxState::clearFlag(Flags flag)
{
  m_flags = Flags(~(int(flag)) & int(m_flags));
}

gfx::Rect SelectBoxState::getBoxBounds() const
{
  int x1 = std::min(m_rulers[V1].position(), m_rulers[V2].position());
  int y1 = std::min(m_rulers[H1].position(), m_rulers[H2].position());
  int x2 = std::max(m_rulers[V1].position(), m_rulers[V2].position());
  int y2 = std::max(m_rulers[H1].position(), m_rulers[H2].position());
  return gfx::Rect(x1, y1, x2-x1, y2-y1);
}

void SelectBoxState::setBoxBounds(const gfx::Rect& box)
{
  m_rulers[H1] = Ruler(HORIZONTAL | TOP, box.y);
  m_rulers[H2] = Ruler(HORIZONTAL | BOTTOM, box.y+box.h);
  m_rulers[V1] = Ruler(VERTICAL | LEFT, box.x);
  m_rulers[V2] = Ruler(VERTICAL | RIGHT, box.x+box.w);
  if (hasFlag(Flags::PaddingRulers)) {
    const gfx::Size padding(
      m_rulers[PV].position() - m_rulers[V2].position(),
      m_rulers[PH].position() - m_rulers[H2].position());
    setPaddingBounds(padding);
  }
}

// Get and Set Padding for Import Sprite Sheet box state
gfx::Size SelectBoxState::getPaddingBounds() const
{
  ASSERT(hasFlag(Flags::PaddingRulers));
  int w = m_rulers[PV].position() - m_rulers[V2].position();
  int h = m_rulers[PH].position() - m_rulers[H2].position();
  if (w < 0)
    w = 0;
  if (h < 0)
    h = 0;
  return gfx::Size(w, h);
}

void SelectBoxState::setPaddingBounds(const gfx::Size& padding)
{
  ASSERT(hasFlag(Flags::PaddingRulers));
  m_rulers[PH] = Ruler(HORIZONTAL | BOTTOM, m_rulers[H2].position() + padding.h);
  m_rulers[PV] = Ruler(VERTICAL | RIGHT, m_rulers[V2].position() + padding.w);
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
    m_startingPos =
      editor->screenToEditor(msg->position())
      - editor->mainTilePosition();

    if (hasFlag(Flags::Rulers) && !hasFlag(Flags::QuickPoint)) {
      m_rulersDragAlign = hitTestRulers(editor, msg->position(), true);
      if (m_rulersDragAlign)
        m_startRulers = m_rulers; // Capture start positions
    }

    if (hasFlag(Flags::QuickBox) && m_movingRulers.empty()) {
      m_selectingBox = true;
      m_selectingButton = msg->button();
      setBoxBounds(gfx::Rect(m_startingPos, gfx::Size(1, 1)));

      // Redraw the editor so we can show the pixel where the mouse
      // button is pressed for first time.
      editor->invalidate();
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
      if (m_selectingButton == msg->button())
        m_delegate->onQuickboxEnd(editor, getBoxBounds(), msg->button());
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
    gfx::Point newPos =
      editor->screenToEditor(msg->position())
      - editor->mainTilePosition();

    gfx::Point delta = newPos - m_startingPos;

    for (int i : m_movingRulers) {
      Ruler& ruler = m_rulers[i];
      const Ruler& start = m_startRulers[i];
      Ruler& oppRuler = oppositeRuler(i);

      switch (ruler.align() & (HORIZONTAL | VERTICAL)) {

        case HORIZONTAL:
          if (hasFlag(Flags::PaddingRulers) && (i == H2)) {
            int pad = m_rulers[PH].position() - m_rulers[H2].position();
            m_rulers[PH].setPosition(start.position() + delta.y + pad);
          }

          ruler.setPosition(start.position() + delta.y);
          if (msg->modifiers() == os::kKeyShiftModifier)
            oppRuler.setPosition(m_startRulers[i ^ 1].position() - delta.y);
          break;

        case VERTICAL:
          if (hasFlag(Flags::PaddingRulers) && (i == V2)) {
            int pad = m_rulers[PV].position() - m_rulers[V2].position();
            m_rulers[PV].setPosition(start.position() + delta.x + pad);
          }

          ruler.setPosition(start.position() + delta.x);
          if (msg->modifiers() == os::kKeyShiftModifier)
            oppRuler.setPosition(m_startRulers[i ^ 1].position() - delta.x);
          break;
      }
    }
    used = true;
  }

  if (hasFlag(Flags::QuickBox) && m_selectingBox) {
    gfx::Point p1 = m_startingPos;
    gfx::Point p2 =
      editor->screenToEditor(msg->position())
      - editor->mainTilePosition();

    if (hasFlag(Flags::QuickPoint))
      p1 = p2;

    if (p2.x < p1.x) std::swap(p1.x, p2.x);
    if (p2.y < p1.y) std::swap(p1.y, p2.y);
    ++p2.x;
    ++p2.y;

    setBoxBounds(gfx::Rect(p1, p2));
    used = true;
  }

  if (used) {
    if (m_delegate) {
      m_delegate->onChangeRectangle(getBoxBounds());
      if (hasFlag(Flags::PaddingRulers))
        m_delegate->onChangePadding(getPaddingBounds());
    }

    editor->invalidate();
    return true;
  }
  else
    return StandbyState::onMouseMove(editor, msg);
}

bool SelectBoxState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  if (hasFlag(Flags::Rulers) && !hasFlag(Flags::QuickPoint)) {
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

bool SelectBoxState::onKeyDown(Editor* editor, ui::KeyMessage* msg)
{
  if (editor != UIContext::instance()->activeEditor())
    return false;

  // Cancel
  if (msg->scancode() == kKeyEsc) {
    if (hasFlag(Flags::QuickBox)) {
      if (m_delegate)
        m_delegate->onQuickboxCancel(editor);
      return true;
    }
  }

  // Use StandbyState implementation
  return StandbyState::onKeyDown(editor, msg);
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

tools::Ink* SelectBoxState::getStateInk() const
{
  if (hasFlag(Flags::QuickBox))
    return App::instance()->toolBox()->getInkById(
      tools::WellKnownInks::Selection);
  else
    return nullptr;
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
  auto theme = skin::SkinTheme::get(editor);
  const gfx::Color rulerColor = theme->colors.selectBoxRuler();
  const gfx::Color gridColor = theme->colors.selectBoxGrid();
  const gfx::Point mainOffset = editor->mainTilePosition();
  gfx::Rect rc = getBoxBounds();
  gfx::Size padding;
  if (hasFlag(Flags::PaddingRulers))
    padding = getPaddingBounds();
  rc.offset(mainOffset);
  sp.offset(mainOffset);

  // With black shadow?
  if (hasFlag(Flags::DarkOutside)) {
    if (hasFlag(Flags::PaddingRulers)) {
      const gfx::Color dark = doc::rgba(0, 0, 0, 128);
      // Top band
      if (rc.y > vp.y)
        render->fillRect(dark, gfx::Rect(vp.x, vp.y, vp.w, rc.y-vp.y));
      // Left band
      if (rc.x > vp.x)
        render->fillRect(dark, gfx::Rect(vp.x, rc.y, rc.x-vp.x, vp.y2()-rc.y));
      if (hasFlag(Flags::VGrid)) {
        // Bottom band
        if (sp.y2() < vp.y2())
          render->fillRect(dark, gfx::Rect(rc.x, sp.y2(), vp.x2()-rc.x, vp.y2()-sp.y2()));
        // Right band
        if (hasFlag(Flags::HGrid)) {
          // VGrid Active, HGrid Active:
          if (sp.x2() < vp.x2())
            render->fillRect(dark, gfx::Rect(sp.x2(), rc.y, vp.x2()-sp.x2(), sp.y2()-rc.y));
        }
        else {
          // Just VGrid Active, HGrid disabled:
          if (rc.x2() < vp.x2())
            render->fillRect(dark, gfx::Rect(rc.x2(), rc.y, vp.x2()-rc.x2(), sp.y2()-rc.y));
        }
      }
      else {
        if (hasFlag(Flags::HGrid)) {
          // Just HGrid Active, VGrid disabled
          // Bottom band
          if (rc.y2() < vp.y2())
            render->fillRect(dark, gfx::Rect(rc.x, rc.y2(), vp.x2()-rc.x, vp.y2()-rc.y2()));
          // Right band
          if (sp.x2() < vp.x2())
            render->fillRect(dark, gfx::Rect(sp.x2(), rc.y, vp.x2()-sp.x2(), rc.h));
        }
      }

      // Draw vertical dark padding big bands
      if (hasFlag(Flags::VGrid) && hasFlag(Flags::HGrid)) {
        std::vector<int> padXTips;
        std::vector<int> padYTips;
        if (rc.w > 0) {
          for (int x=rc.x+rc.w; x<=sp.x+sp.w; x+=(rc.w+padding.w)) {
            if (x + padding.w > sp.x2())
              render->fillRect(dark, gfx::Rect(x, rc.y, sp.x2()-x, sp.h-rc.y));
            else
              render->fillRect(dark, gfx::Rect(x, rc.y, padding.w, sp.h-rc.y));
            padXTips.push_back(x);
          }
        }
        if (rc.h > 0) {
          // Draw horizontal dark chopped padding bands (to complete the dark grid)
          for (int y=rc.y+rc.h; y<=sp.y+sp.h; y+=(rc.h+padding.h)) {
            for (const int& padXTip : padXTips) {
              if (y+padding.h > sp.y2())
                render->fillRect(dark, gfx::Rect(padXTip-rc.w, y, rc.w, sp.y2()-y));
              else
                render->fillRect(dark, gfx::Rect(padXTip-rc.w, y, rc.w, padding.h));
            }
            if (!padXTips.empty()) {
              if (padXTips.back() + padding.w < sp.x2()) {
                if (y + padding.h > sp.y2())
                  render->fillRect(dark, gfx::Rect(padXTips.back() + padding.w, y,
                                                   sp.x2() - padXTips.back() - padding.w,
                                                   sp.y2() - y));
                else
                  render->fillRect(dark, gfx::Rect(padXTips.back() + padding.w, y,
                                                  sp.x2() - padXTips.back() - padding.w,
                                                  padding.h));
              }
            }
            padYTips.push_back(y);
          }
        }
        // Draw chopped dark rectangles to ban partial tiles
        if (!hasFlag(Flags::IncludePartialTiles)) {
          if (!padXTips.empty() && !padYTips.empty()) {
            if (padXTips.back() + padding.w < sp.x2()) {
              for (const int& padYTip : padYTips)
                render->fillRect(dark, gfx::Rect(padXTips.back() + padding.w, padYTip - rc.h,
                                                 sp.x2() - padXTips.back() - padding.w, rc.h));
              if (padYTips.back() + padding.h < sp.y2()) {
                render->fillRect(dark, gfx::Rect(padXTips.back() + padding.w,
                                                 padYTips.back() + padding.h,
                                                 sp.x2() - padXTips.back() - padding.w,
                                                 sp.y2() - padYTips.back() - padding.h));
              }
            }
            if (padYTips.back() + padding.h < sp.y2()) {
              for (const int& padXTip : padXTips)
                render->fillRect(dark, gfx::Rect(padXTip - rc.w, padYTips.back() + padding.h,
                                                 rc.w, sp.y2() - padYTips.back() - padding.h));
            }
          }
        }
      }
      else if (hasFlag(Flags::HGrid)) {
        if (rc.w > 0) {
          int lastX = 0;
          for (int x=rc.x+rc.w; x<=sp.x+sp.w; x+=rc.w+padding.w) {
            if (x + padding.w > sp.x2())
              render->fillRect(dark, gfx::Rect(x, rc.y, sp.x2()-x, rc.h));
            else
              render->fillRect(dark, gfx::Rect(x, rc.y, padding.w, rc.h));
            lastX = x;
          }
          if (!hasFlag(Flags::IncludePartialTiles) && (lastX > 0)) {
            if (lastX + padding.w < sp.x2())
              render->fillRect(dark, gfx::Rect(lastX + padding.w, rc.y,
                                               sp.x2() - lastX - padding.w, rc.h));
          }
        }
      }
      else if (hasFlag(Flags::VGrid)) {
        if (rc.h > 0) {
        int lastY = 0;
          for (int y=rc.y+rc.h; y<=sp.y+sp.h; y+=rc.h+padding.h) {
            if (y + padding.h > sp.y2())
              render->fillRect(dark, gfx::Rect(rc.x, y, rc.w, sp.y2()-y));
            else
              render->fillRect(dark, gfx::Rect(rc.x, y, rc.w, padding.h));
            lastY = y;
          }
          if (!hasFlag(Flags::IncludePartialTiles) && (lastY > 0)) {
            if (lastY + padding.h < sp.y2())
              render->fillRect(dark, gfx::Rect(rc.x, lastY + padding.h,
                                               rc.w, sp.y2() - lastY - padding.h));
          }
        }
      }
    }
    else {
      // Draw dark zones when SelectBoxState is a simple box
      const gfx::Color dark = doc::rgba(0, 0, 0, 128);
      // Top band
      if (rc.y > vp.y)
        render->fillRect(dark, gfx::Rect(vp.x, vp.y, vp.w, rc.y-vp.y));
      // Bottom band
      if (rc.y2() < vp.y2())
        render->fillRect(dark, gfx::Rect(vp.x, rc.y2(), vp.w, vp.y2()-rc.y2()));
      // Left band
      if (rc.x > vp.x)
        render->fillRect(dark, gfx::Rect(vp.x, rc.y, rc.x-vp.x, rc.h));
      // Right band
      if (rc.x2() < vp.x2())
        render->fillRect(dark, gfx::Rect(rc.x2(), rc.y, vp.x2()-rc.x2(), rc.h));
    }
  }

  // Draw the grid rulers when padding is posible (i.e. Flag::PaddingRulers=true)
  if (hasFlag(Flags::PaddingRulers)) {
    if (hasFlag(Flags::HGrid) && hasFlag(Flags::VGrid)) {
      if (rc.w > 0 && padding.w == 0) {
        for (int x=rc.x+rc.w*2+padding.w; x<=sp.x+sp.w; x+=rc.w+padding.w)
          render->drawLine(gridColor, x, rc.y, x, sp.y2());
        for (int x=rc.x+rc.w+padding.w; x<=sp.x+sp.w; x+=rc.w+padding.w)
          render->drawLine(gridColor, x, rc.y, x, sp.y2());
      }

      if (rc.h > 0 && padding.h == 0) {
        for (int y=rc.y+rc.h*2+padding.h; y<=sp.y+sp.h; y+=rc.h+padding.h)
          render->drawLine(gridColor, rc.x, y, sp.x2(), y);
        for (int y=rc.y+rc.h+padding.h; y<=sp.y+sp.h; y+=rc.h+padding.h)
          render->drawLine(gridColor, rc.x, y, sp.x2(), y);
      }
    }
    else if (hasFlag(Flags::HGrid)) {
      if (rc.w > 0 && padding.w == 0) {
        for (int x=rc.x+rc.w*2+padding.w; x<=sp.x+sp.w; x+=rc.w+padding.w)
          render->drawLine(gridColor, x, rc.y, x, rc.y2());
        for (int x=rc.x+rc.w+padding.w; x<=sp.x+sp.w; x+=rc.w+padding.w)
          render->drawLine(gridColor, x, rc.y, x, rc.y2());
      }
    }
    else if (hasFlag(Flags::VGrid)) {
      if (rc.h > 0 && padding.h == 0) {
        for (int y=rc.y+rc.h*2+padding.h; y<=sp.y+sp.h; y+=rc.h+padding.h)
          render->drawLine(gridColor, rc.x, y, rc.x2(), y);
        for (int y=rc.y+rc.h+padding.h; y<=sp.y+sp.h; y+=rc.h+padding.h)
          render->drawLine(gridColor, rc.x, y, rc.x2(), y);
      }
    }
  }
  else {
    // Draw the grid rulers when padding is not posible (i.e. Flag::PaddingRulers=false)
    if (hasFlag(Flags::Grid)) {
      if (rc.w > 0) {
        for (int x=rc.x+rc.w*2; x<=sp.x+sp.w; x+=rc.w)
          render->drawLine(gridColor, x, rc.y, x, sp.y+sp.h);
      }

      if (rc.h > 0) {
        for (int y=rc.y+rc.h*2; y<=sp.y+sp.h; y+=rc.h)
          render->drawLine(gridColor, rc.x, y, sp.x+sp.w, y);
      }
    }
    else if (hasFlag(Flags::HGrid)) {
      if (rc.w > 0) {
        for (int x=rc.x+rc.w*2; x<=sp.x+sp.w; x+=rc.w)
          render->drawLine(gridColor, x, rc.y, x, rc.y+rc.h);
      }
    }
    else if (hasFlag(Flags::VGrid)) {
      if (rc.h > 0) {
        for (int y=rc.y+rc.h*2; y<=sp.y+sp.h; y+=rc.h)
          render->drawLine(gridColor, rc.x, y, rc.x+rc.w, y);
      }
    }
  }

  // Draw the rulers enclosing the box
  if (hasFlag(Flags::Rulers)) {
    for (const Ruler& ruler : m_rulers) {
      switch (ruler.align() & (HORIZONTAL | VERTICAL)) {

        case HORIZONTAL: {
          const int y = ruler.position()+mainOffset.y;
          render->drawLine(rulerColor, vp.x, y, vp.x+vp.w-1, y);
          break;
        }

        case VERTICAL: {
          const int x = ruler.position()+mainOffset.x;
          render->drawLine(rulerColor, x, vp.y, x, vp.y+vp.h-1);
          break;
        }
      }
    }
  }

  if (hasFlag(Flags::QuickBox)) {
    render->drawRect(gfx::rgba(255, 255, 255), rc);
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

Ruler& SelectBoxState::oppositeRuler(const int rulerIndex)
{
  // 0 and 1 are opposites, and 2 and 3
  return m_rulers[rulerIndex ^ 1];
}

int SelectBoxState::hitTestRulers(Editor* editor,
                                  const gfx::Point& mousePos,
                                  const bool updateMovingRulers)
{
  ASSERT(H1 == 0);
  ASSERT(H2 == 1);
  ASSERT(V1 == 2);
  ASSERT(V2 == 3);
  int align = 0;

  if (updateMovingRulers)
    m_movingRulers.clear();

  for (int i=0; i<int(m_rulers.size()); ++i) {
    const Ruler& ruler = m_rulers[i];
    const Ruler& oppRuler = oppositeRuler(i);

    if (hitTestRuler(editor, ruler, oppRuler, mousePos)) {
      if (!hasFlag(Flags::PaddingRulers) &&
          (((ruler.align() & (LEFT | TOP)) && ruler.position() > oppRuler.position()) ||
           ((ruler.align() & (RIGHT | BOTTOM)) && ruler.position() <= oppRuler.position())))
        align |= oppRuler.align();
      else
        align |= ruler.align();
      if (updateMovingRulers)
        m_movingRulers.push_back(i);
    }
  }

  // Check moving all rulers at the same time
  if (align == 0 && !hasFlag(Flags::QuickBox)) {
    if (editor->editorToScreen(
          getBoxBounds().offset(editor->mainTilePosition()))
        .contains(mousePos)) {
      align = LEFT | TOP | RIGHT | BOTTOM;
      if (updateMovingRulers) {
        // Add all rulers
        for (int i=0; i<m_rulers.size(); ++i)
          m_movingRulers.push_back(i);
      }
    }
  }

  return align;
}

int SelectBoxState::hitTestRuler(Editor* editor,
                                 const Ruler& ruler,
                                 const Ruler& oppRuler,
                                 const gfx::Point& mousePos)
{
  gfx::Point pt = editor->mainTilePosition();
  pt = editor->editorToScreen(
    pt + gfx::Point(ruler.position(), ruler.position()));

  switch (ruler.align() & (HORIZONTAL | VERTICAL)) {

    case HORIZONTAL:
      if (!hasFlag(Flags::PaddingRulers)) {
        if (ruler.position() <= oppRuler.position()) {
          if (mousePos.y <= pt.y+2*guiscale())
            return ruler.align();
        }
        else if (ruler.position() > oppRuler.position()) {
          if (mousePos.y >= pt.y-2*guiscale())
            return ruler.align();
        }
      }
      if (mousePos.y >= pt.y-2*guiscale() &&
          mousePos.y <= pt.y+2*guiscale()) {
        return ruler.align();
      }
      break;

    case VERTICAL:
      if (!hasFlag(Flags::PaddingRulers)) {
        if (ruler.position() <= oppRuler.position()) {
          if (mousePos.x <= pt.x+2*guiscale())
            return ruler.align();
        }
        else if (ruler.position() > oppRuler.position()) {
          if (mousePos.x >= pt.x-2*guiscale())
            return ruler.align();
        }
      }
      if (mousePos.x >= pt.x-2*guiscale() &&
          mousePos.x <= pt.x+2*guiscale())
        return ruler.align();
      break;
  }

  return 0;
}

bool SelectBoxState::hasFlag(Flags flag) const
{
  return (int(m_flags) & int(flag)) == int(flag);
}

CursorType SelectBoxState::cursorFromAlign(const int align) const
{
  switch (align & (LEFT | TOP | RIGHT | BOTTOM)) {
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
