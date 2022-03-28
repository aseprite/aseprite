// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/dock.h"

#include "app/ui/dockable.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/cursor_type.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/widget.h"

namespace app {

using namespace app::skin;
using namespace ui;

namespace {

enum { kTopIndex, kBottomIndex, kLeftIndex, kRightIndex, kCenterIndex };

int side_index(int side)
{
  switch (side) {
    case ui::TOP:    return kTopIndex;
    case ui::BOTTOM: return kBottomIndex;
    case ui::LEFT:   return kLeftIndex;
    case ui::RIGHT:  return kRightIndex;
  }
  return kCenterIndex; // ui::CENTER
}

} // anonymous namespace

void DockTabs::onSizeHint(ui::SizeHintEvent& ev)
{
  gfx::Size sz;
  for (auto child : children()) {
    if (child->isVisible())
      sz |= child->sizeHint();
  }
  sz.h += textHeight();
  ev.setSizeHint(sz);
}

void DockTabs::onResize(ui::ResizeEvent& ev)
{
  auto bounds = ev.bounds();
  setBoundsQuietly(bounds);
  bounds = childrenBounds();
  bounds.y += textHeight();
  bounds.h -= textHeight();

  for (auto child : children()) {
    child->setBounds(bounds);
  }
}

void DockTabs::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  g->fillRect(gfx::rgba(0, 0, 255), clientBounds());
}

Dock::Dock()
{
  for (int i = 0; i < kSides; ++i) {
    m_sides[i] = nullptr;
    m_aligns[i] = 0;
    m_sizes[i] = gfx::Size(0, 0);
  }

  InitTheme.connect([this] {
    if (auto p = parent())
      setBgColor(p->bgColor());
  });
  initTheme();
}

void Dock::resetDocks()
{
  for (int i = 0; i < kSides; ++i) {
    auto child = m_sides[i];
    if (!child)
      continue;
    else if (auto subdock = dynamic_cast<Dock*>(child)) {
      subdock->resetDocks();
      if (subdock->m_autoDelete)
        delete subdock;
    }
    else if (auto tabs = dynamic_cast<DockTabs*>(child)) {
      for (auto child2 : tabs->children()) {
        if (auto subdock2 = dynamic_cast<Dock*>(child2)) {
          subdock2->resetDocks();
          if (subdock2->m_autoDelete)
            delete subdock2;
        }
      }
    }
    m_sides[i] = nullptr;
  }
  removeAllChildren();
}

void Dock::dock(int side, ui::Widget* widget, const gfx::Size& prefSize)
{
  ASSERT(widget);

  const int i = side_index(side);
  if (!m_sides[i]) {
    setSide(i, widget);
    addChild(widget);

    if (prefSize != gfx::Size(0, 0))
      m_sizes[i] = prefSize;
  }
  else if (auto subdock = dynamic_cast<Dock*>(m_sides[i])) {
    subdock->dock(CENTER, widget, prefSize);
  }
  else if (auto tabs = dynamic_cast<DockTabs*>(m_sides[i])) {
    tabs->addChild(widget);
  }
  // If this side already contains a widget, we create a DockTabs in
  // this side.
  else {
    auto oldWidget = m_sides[i];
    auto newTabs = new DockTabs;
    replaceChild(oldWidget, newTabs);
    newTabs->addChild(oldWidget);
    newTabs->addChild(widget);
    setSide(i, newTabs);
  }
}

void Dock::dockRelativeTo(ui::Widget* relative,
                          int side,
                          ui::Widget* widget,
                          const gfx::Size& prefSize)
{
  ASSERT(relative);

  Widget* parent = relative->parent();
  ASSERT(parent);

  Dock* subdock = new Dock;
  subdock->m_autoDelete = true;
  parent->replaceChild(relative, subdock);
  subdock->dock(CENTER, relative);
  subdock->dock(side, widget, prefSize);

  // Fix the m_sides item if the parent is a Dock
  if (auto relativeDock = dynamic_cast<Dock*>(parent)) {
    for (int i = 0; i < kSides; ++i) {
      if (relativeDock->m_sides[i] == relative) {
        relativeDock->setSide(i, subdock);
        break;
      }
    }
  }
}

void Dock::undock(Widget* widget)
{
  Widget* parent = widget->parent();
  if (!parent)
    return; // Already undocked

  if (auto parentDock = dynamic_cast<Dock*>(parent)) {
    parentDock->removeChild(widget);

    for (int i = 0; i < kSides; ++i) {
      if (parentDock->m_sides[i] == widget) {
        parentDock->setSide(i, nullptr);
        break;
      }
    }

    if (parentDock != this && parentDock->children().empty()) {
      undock(parentDock);
    }
  }
  else if (auto parentTabs = dynamic_cast<DockTabs*>(parent)) {
    parentTabs->removeChild(widget);

    if (parentTabs->children().empty()) {
      undock(parentTabs);
    }
  }
  else {
    parent->removeChild(widget);
  }
}

Dock* Dock::subdock(int side)
{
  int i = side_index(side);
  if (auto subdock = dynamic_cast<Dock*>(m_sides[i]))
    return subdock;

  auto oldWidget = m_sides[i];
  auto newSubdock = new Dock;
  newSubdock->m_autoDelete = true;
  setSide(i, newSubdock);

  if (oldWidget) {
    replaceChild(oldWidget, newSubdock);
    newSubdock->dock(CENTER, oldWidget);
  }
  else
    addChild(newSubdock);

  return newSubdock;
}

void Dock::onSizeHint(ui::SizeHintEvent& ev)
{
  gfx::Size sz = border().size();

  if (m_sides[kLeftIndex])
    sz.w += m_sides[kLeftIndex]->sizeHint().w + childSpacing();
  if (m_sides[kRightIndex])
    sz.w += m_sides[kRightIndex]->sizeHint().w + childSpacing();
  if (m_sides[kTopIndex])
    sz.h += m_sides[kTopIndex]->sizeHint().h + childSpacing();
  if (m_sides[kBottomIndex])
    sz.h += m_sides[kBottomIndex]->sizeHint().h + childSpacing();
  if (m_sides[kCenterIndex]) {
    sz += m_sides[kCenterIndex]->sizeHint();
  }

  ev.setSizeHint(sz);
}

void Dock::onResize(ui::ResizeEvent& ev)
{
  auto bounds = ev.bounds();
  setBoundsQuietly(bounds);
  bounds = childrenBounds();

  updateDockVisibility();

  forEachSide(bounds,
              [bounds](ui::Widget* widget,
                       const gfx::Rect& widgetBounds,
                       const gfx::Rect& separator,
                       const int index) { widget->setBounds(widgetBounds); });
}

void Dock::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  g->fillRect(bgColor(), clientBounds());
}

void Dock::onInitTheme(ui::InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  setBorder(gfx::Border(0));
  setChildSpacing(4 * ui::guiscale());
}

bool Dock::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage: {
      const gfx::Point pos = static_cast<MouseMessage*>(msg)->position();

      m_capturedSide = -1;
      forEachSide(childrenBounds(),
                  [this, pos](ui::Widget* widget,
                              const gfx::Rect& widgetBounds,
                              const gfx::Rect& separator,
                              const int index) {
                    if (separator.contains(pos)) {
                      m_capturedWidget = widget;
                      m_capturedSide = index;
                      m_startSize = m_sizes[index];
                      m_startPos = pos;
                    }
                  });

      if (m_capturedSide >= 0) {
        captureMouse();
        return true;
      }
      break;
    }

    case kMouseMoveMessage: {
      if (hasCapture()) {
        if (m_capturedSide >= 0) {
          const gfx::Point pos = static_cast<MouseMessage*>(msg)->position();
          gfx::Size& sz = m_sizes[m_capturedSide];

          switch (m_capturedSide) {
            case kTopIndex:    sz.h = (m_startSize.h + pos.y - m_startPos.y); break;
            case kBottomIndex: sz.h = (m_startSize.h - pos.y + m_startPos.y); break;
            case kLeftIndex:   sz.w = (m_startSize.w + pos.x - m_startPos.x); break;
            case kRightIndex:  sz.w = (m_startSize.w - pos.x + m_startPos.x); break;
          }

          layout();
        }
      }
      break;
    }

    case kMouseUpMessage: {
      if (hasCapture()) {
        releaseMouse();
      }
      break;
    }

    case kSetCursorMessage: {
      const gfx::Point pos = static_cast<MouseMessage*>(msg)->position();
      ui::CursorType cursor = ui::kArrowCursor;
      forEachSide(childrenBounds(),
                  [pos, &cursor](ui::Widget* widget,
                                 const gfx::Rect& widgetBounds,
                                 const gfx::Rect& separator,
                                 const int index) {
                    if (separator.contains(pos)) {
                      if (index == kTopIndex || index == kBottomIndex) {
                        cursor = ui::kSizeNSCursor;
                      }
                      else if (index == kLeftIndex || index == kRightIndex) {
                        cursor = ui::kSizeWECursor;
                      }
                    }
                  });
      ui::set_mouse_cursor(cursor);
      return true;
    }
  }
  return Widget::onProcessMessage(msg);
}

void Dock::setSide(const int i, Widget* newWidget)
{
  m_sides[i] = newWidget;
  m_aligns[i] = calcAlign(i);

  if (newWidget) {
    m_sizes[i] = newWidget->sizeHint();
  }
}

int Dock::calcAlign(const int i)
{
  Widget* widget = m_sides[i];
  int align = 0;
  if (!widget) {
    // Do nothing
  }
  else if (auto subdock = dynamic_cast<Dock*>(widget)) {
    align = subdock->calcAlign(i);
  }
  else if (auto tabs = dynamic_cast<DockTabs*>(widget)) {
    for (auto child : tabs->children()) {
      if (auto subdock2 = dynamic_cast<Dock*>(widget))
        align |= subdock2->calcAlign(i);
      else if (auto dockable = dynamic_cast<Dockable*>(child)) {
        align = dockable->dockableAt();
      }
    }
  }
  else if (auto dockable2 = dynamic_cast<Dockable*>(widget)) {
    align = dockable2->dockableAt();
  }
  return align;
}

void Dock::updateDockVisibility()
{
  bool visible = false;
  setVisible(true);
  for (int i = 0; i < kSides; ++i) {
    Widget* widget = m_sides[i];
    if (!widget)
      continue;

    if (auto subdock = dynamic_cast<Dock*>(widget)) {
      subdock->updateDockVisibility();
    }
    else if (auto tabs = dynamic_cast<DockTabs*>(widget)) {
      bool visible2 = false;
      for (auto child : tabs->children()) {
        if (auto subdock2 = dynamic_cast<Dock*>(widget)) {
          subdock2->updateDockVisibility();
        }
        if (child->isVisible()) {
          visible2 = true;
        }
      }
      tabs->setVisible(visible2);
      if (visible2)
        visible = true;
    }

    if (widget->isVisible()) {
      visible = true;
    }
  }
  setVisible(visible);
}

void Dock::forEachSide(gfx::Rect bounds,
                       std::function<void(ui::Widget* widget,
                                          const gfx::Rect& widgetBounds,
                                          const gfx::Rect& separator,
                                          const int index)> f)
{
  for (int i = 0; i < kSides; ++i) {
    auto widget = m_sides[i];
    if (!widget || !widget->isVisible()) {
      continue;
    }

    int spacing = (m_aligns[i] & EXPANSIVE ? childSpacing() : 0);

    const gfx::Size sz = (m_aligns[i] & EXPANSIVE ? m_sizes[i] : widget->sizeHint());
    gfx::Rect rc, separator;
    switch (i) {
      case kTopIndex:
        rc = gfx::Rect(bounds.x, bounds.y, bounds.w, sz.h);
        bounds.y += rc.h;
        bounds.h -= rc.h;

        if (spacing > 0) {
          separator = gfx::Rect(bounds.x, bounds.y, bounds.w, spacing);
          bounds.y += spacing;
          bounds.h -= spacing;
        }
        break;
      case kBottomIndex:
        rc = gfx::Rect(bounds.x, bounds.y2() - sz.h, bounds.w, sz.h);
        bounds.h -= rc.h;

        if (spacing > 0) {
          separator = gfx::Rect(bounds.x, bounds.y2() - spacing, bounds.w, spacing);
          bounds.h -= spacing;
        }
        break;
      case kLeftIndex:
        rc = gfx::Rect(bounds.x, bounds.y, sz.w, bounds.h);
        bounds.x += rc.w;
        bounds.w -= rc.w;

        if (spacing > 0) {
          separator = gfx::Rect(bounds.x, bounds.y, spacing, bounds.h);
          bounds.x += spacing;
          bounds.w -= spacing;
        }
        break;
      case kRightIndex:
        rc = gfx::Rect(bounds.x2() - sz.w, bounds.y, sz.w, bounds.h);
        bounds.w -= rc.w;

        if (spacing > 0) {
          separator = gfx::Rect(bounds.x2() - spacing, bounds.y, spacing, bounds.h);
          bounds.w -= spacing;
        }
        break;
      case kCenterIndex: rc = bounds; break;
    }

    f(widget, rc, separator, i);
  }
}

} // namespace app
