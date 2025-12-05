// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/dock.h"

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/pref/preferences.h"
#include "app/ui/dockable.h"
#include "app/ui/layout_selector.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "gfx/path.h"
#include "os/system.h"
#include "ui/cursor_type.h"
#include "ui/label.h"
#include "ui/menu.h"
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

Dock::SideIndex side_index(int sideFlag)
{
  switch (sideFlag) {
    case ui::TOP:    return Dock::kTopIndex;
    case ui::BOTTOM: return Dock::kBottomIndex;
    case ui::LEFT:   return Dock::kLeftIndex;
    case ui::RIGHT:  return Dock::kRightIndex;
  }
  return Dock::kCenterIndex; // ui::CENTER
}

int side_from_index(const Dock::SideIndex sideIndex)
{
  switch (sideIndex) {
    case Dock::kTopIndex:    return ui::TOP;
    case Dock::kBottomIndex: return ui::BOTTOM;
    case Dock::kLeftIndex:   return ui::LEFT;
    case Dock::kRightIndex:  return ui::RIGHT;
  }
  return ui::CENTER; // kCenterIndex
}

} // anonymous namespace

// TODO: Duplicated from main_window.cpp
static constexpr auto kLegacyLayoutMainWindowSection = "layout:main_window";
static constexpr auto kLegacyLayoutTimelineSplitter = "timeline_splitter";

//////////////////////////////////////////////////////////////////////
// Dock::Hit

gfx::Rect Dock::Hit::bounds() const
{
  if (widget)
    return widget->bounds();
  return {};
}

//////////////////////////////////////////////////////////////////////
// Dock::DraggingFeedback

Dock::DraggingFeedback::DraggingFeedback(Display* display,
                                         const Hit& hit,
                                         const gfx::Point& mousePosition)
  : m_display(display)
  , m_dragWidget(hit.widget)
  , m_mouseOffset(mousePosition - hit.widget->origin())
{
  const os::SurfaceRef surface = os::System::instance()->makeRgbaSurface(m_dragWidget->size().w,
                                                                         m_dragWidget->size().h);
  {
    const os::SurfaceLock lock(surface.get());
    Paint paint;
    paint.color(gfx::rgba(0, 0, 0, 0));
    paint.style(os::Paint::Fill);
    surface->drawRect(surface->bounds(), paint);
  }

  {
    Graphics g(surface);
    g.setFont(m_dragWidget->font());
    m_dragWidget->paint(&g, gfx::Region(m_dragWidget->bounds()), true, (DrawableRegionFlags)0);
  }

  m_floatingUILayer = UILayer::Make();
  m_floatingUILayer->setSurface(surface);
  m_floatingUILayer->setPosition(m_dragWidget->origin());

  os::Paint p;
  p.color(gfx::rgba(0, 0, 0, 200));
  m_floatingUILayer->setPaint(p);

  m_display->addLayer(m_floatingUILayer);
}

Dock::DraggingFeedback::~DraggingFeedback()
{
  m_display->removeLayer(m_floatingUILayer);

  if (m_dropUILayer)
    m_display->removeLayer(m_dropUILayer);
}

void Dock::DraggingFeedback::setHit(const Hit& hit)
{
  m_hit = hit;

  if (m_hit.widget && m_hit.targetSide) {
    if (!m_dropUILayer) {
      m_dropUILayer = UILayer::Make();
      m_display->addLayer(m_dropUILayer);
      if (m_floatingUILayer) {
        m_display->removeLayer(m_floatingUILayer);
        m_display->addLayer(m_floatingUILayer);
      }
    }

    gfx::Rect dropBounds = hit.bounds();
    if (dropBounds.isEmpty())
      return;

    dropBounds.enlarge(2 * guiscale());

    m_display->dirtyRect(m_dropUILayer->bounds());

    os::SurfaceRef surface = os::System::instance()->makeRgbaSurface(dropBounds.w, dropBounds.h);
    m_dropUILayer->setSurface(surface);
    m_dropUILayer->setPosition(dropBounds.origin());
    m_display->dirtyRect(m_dropUILayer->bounds());

    os::Paint p;
    p.color(gfx::rgba(0, 0, 0, 128));
    p.blendMode(os::BlendMode::Difference);
    m_dropUILayer->setPaint(p);

    Graphics g(surface);
    paint(&g, gfx::Rect(dropBounds.size()));
  }
  else if (m_dropUILayer) {
    m_display->dirtyRect(m_dropUILayer->bounds());
    m_display->removeLayer(m_dropUILayer);
    m_dropUILayer.reset();
  }
}

void Dock::DraggingFeedback::setFloatingPosition(const gfx::Point& position) const
{
  ASSERT(m_floatingUILayer);

  m_display->dirtyRect(m_floatingUILayer->bounds());
  m_floatingUILayer->setPosition(position - m_mouseOffset);
  m_display->dirtyRect(m_floatingUILayer->bounds());
}

void Dock::DraggingFeedback::paint(Graphics* g, gfx::Rect bounds) const
{
  bounds.shrink(4 * guiscale());

  const gfx::Point pts[4] = {
    bounds.origin(),
    gfx::Point(bounds.x2() - 1, bounds.y),
    gfx::Point(bounds.x, bounds.y2() - 1),
    bounds.point2(),
  };

  if (!m_hit.targetSide)
    return;

  gfx::Path path;
  path.moveTo(bounds.center());
  switch (m_hit.targetSide) {
    case TOP:
      path.lineTo(pts[0]);
      path.lineTo(pts[1]);
      break;
    case LEFT:
      path.lineTo(pts[0]);
      path.lineTo(pts[2]);
      break;
    case RIGHT:
      path.lineTo(pts[1]);
      path.lineTo(pts[3]);
      break;
    case BOTTOM:
      path.lineTo(pts[2]);
      path.lineTo(pts[3]);
      break;
  }
  path.close();

  os::Paint paint;
  paint.color(gfx::rgba(255, 255, 255, 128));
  paint.style(Paint::StrokeAndFill);
  paint.strokeWidth(4.0f * guiscale());
#if LAF_SKIA
  paint.skPaint().setStrokeCap(SkPaint::kRound_Cap);
  paint.skPaint().setStrokeJoin(SkPaint::kRound_Join);
#endif
  g->drawPath(path, paint);
}

//////////////////////////////////////////////////////////////////////
// Dock

Dock::Dock()
{
  for (int i = 0; i < kSides; ++i) {
    m_sides[i] = nullptr;
    m_aligns[i] = 0;
    m_sizes[i] = gfx::Size(0, 0);
  }

  initTheme();
}

void Dock::setCustomizing(bool enable, bool doLayout)
{
  m_customizing = enable;

  for (int i = 0; i < kSides; ++i) {
    auto* child = m_sides[i];
    if (!child)
      continue;

    if (auto* subdock = dynamic_cast<Dock*>(child))
      subdock->setCustomizing(enable, false);
  }

  if (doLayout)
    layout();
}

void Dock::resetDocks()
{
  for (int i = 0; i < kSides; ++i) {
    auto* child = m_sides[i];
    if (!child)
      continue;

    if (auto* subdock = dynamic_cast<Dock*>(child)) {
      subdock->resetDocks();
      if (subdock->m_autoDelete)
        delete subdock;
    }

    m_sides[i] = nullptr;
  }
  removeAllChildren();
}

void Dock::dock(const int sideFlag, ui::Widget* widget, const gfx::Size& prefSize)
{
  ASSERT(widget);

  const SideIndex i = side_index(sideFlag);
  if (!m_sides[i]) {
    setSide(i, widget);
    addChild(widget);

    if (prefSize != gfx::Size(0, 0))
      m_sizes[i] = prefSize;
  }
  else if (Dock* subdock = this->subdock(sideFlag)) {
    subdock->dock(sideFlag, widget, prefSize);
  }
  else {
    ASSERT(false); // Docking failure!
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

  auto* subdock = new Dock;
  subdock->m_autoDelete = true;
  subdock->m_customizing = m_customizing;
  parent->replaceChild(relative, subdock);
  subdock->dock(CENTER, relative);
  subdock->dock(side, widget, prefSize);

  // Fix the m_sides item if the parent is a Dock
  if (auto* relativeDock = dynamic_cast<Dock*>(parent)) {
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

  if (auto* parentDock = dynamic_cast<Dock*>(parent)) {
    parentDock->removeChild(widget);

    for (int i = 0; i < kSides; ++i) {
      if (parentDock->m_sides[i] == widget) {
        parentDock->setSide(i, nullptr);
        m_sizes[i] = gfx::Size();
        break;
      }
    }

    if (parentDock->m_autoDelete) {
      if (parentDock->children().empty()) {
        undock(parentDock);
        parentDock->deferDelete();
      }
    }
  }
  else {
    parent->removeChild(widget);
  }
}

int Dock::whichSideChildIsDocked(const ui::Widget* widget) const
{
  for (int i = 0; i < kSides; ++i)
    if (m_sides[i] == widget)
      return side_from_index(SideIndex(i));
  return 0;
}

const gfx::Size Dock::getUserDefinedSizeAtSide(const int sideFlag) const
{
  SideIndex i = side_index(sideFlag);
  // Only EXPANSIVE sides can be user-defined (has a splitter so the
  // user can expand or shrink it)
  if (m_aligns[i] & EXPANSIVE)
    return m_sizes[i];

  return gfx::Size();
}

Dock* Dock::subdock(int side)
{
  int i = side_index(side);
  if (auto* subdock = dynamic_cast<Dock*>(m_sides[i]))
    return subdock;

  auto* oldWidget = m_sides[i];
  auto* newSubdock = new Dock;
  newSubdock->m_autoDelete = true;
  newSubdock->m_customizing = m_customizing;
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
  gfx::Size fitIn = ev.fitInSize();
  gfx::Size sz;

  for (int i = 0; i < kSides; ++i) {
    auto* widget = m_sides[i];
    if (!widget || !widget->isVisible() || widget->isDecorative())
      continue;

    const int spacing = (m_aligns[i] & EXPANSIVE ? childSpacing() : 0);
    const auto hint = (m_aligns[i] & EXPANSIVE ? m_sizes[i] : widget->sizeHint(fitIn));

    switch (i) {
      case kTopIndex:
      case kBottomIndex:
        sz.h += hint.h;
        fitIn.h = std::max(0, fitIn.h - hint.h);
        if (spacing > 0) {
          sz.h += spacing;
          fitIn.h = std::max(0, fitIn.h - spacing);
        }
        break;
      case kLeftIndex:
      case kRightIndex:
        sz.w += hint.w;
        fitIn.w = std::max(0, fitIn.w - hint.w);
        if (spacing > 0) {
          sz.w += spacing;
          fitIn.w = std::max(0, fitIn.w - spacing);
        }
        break;
      case kCenterIndex:
        sz += gfx::Size(std::max(hint.w, std::max(m_sizes[kTopIndex].w, m_sizes[kBottomIndex].w)),
                        std::max(hint.h, std::max(m_sizes[kLeftIndex].h, m_sizes[kRightIndex].h)));
        break;
    }
  }

  sz += border();
  ev.setSizeHint(sz);
}

void Dock::onResize(ui::ResizeEvent& ev)
{
  gfx::Rect bounds = ev.bounds();
  setBoundsQuietly(bounds);
  bounds = childrenBounds();

  updateDockVisibility();

  forEachSide(bounds,
              [this](ui::Widget* widget,
                     const gfx::Rect& widgetBounds,
                     const gfx::Rect& separator,
                     const int index) {
                gfx::Rect rc = widgetBounds;
                auto th = textHeight();
                if (isCustomizing()) {
                  int handleSide = 0;
                  if (auto* dockable = dynamic_cast<Dockable*>(widget))
                    handleSide = dockable->dockHandleSide();
                  switch (handleSide) {
                    case ui::TOP:
                      rc.y += th;
                      rc.h -= th;
                      break;
                    case ui::LEFT:
                      rc.x += th;
                      rc.w -= th;
                      break;
                  }
                }
                widget->setBounds(rc);
              });
}

void Dock::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();

  const gfx::Rect& bounds = clientBounds();
  g->fillRect(bgColor(), bounds);

  if (isCustomizing()) {
    forEachSide(bounds,
                [this, g](ui::Widget* widget,
                          const gfx::Rect& widgetBounds,
                          const gfx::Rect& separator,
                          const int index) {
                  gfx::Rect rc = widgetBounds;
                  auto th = textHeight();

                  auto* theme = SkinTheme::get(this);
                  const gfx::Color color = theme->colors.workspaceText();
                  int handleSide = 0;
                  if (auto* dockable = dynamic_cast<Dockable*>(widget))
                    handleSide = dockable->dockHandleSide();
                  switch (handleSide) {
                    case ui::TOP:
                      rc.h = th;
                      for (int y = rc.y; y + 1 < rc.y2(); y += 2)
                        g->drawHLine(color,
                                     rc.x + widget->border().left(),
                                     y,
                                     rc.w - widget->border().width());
                      break;
                    case ui::LEFT:
                      rc.w = th;
                      for (int x = rc.x; x + 1 < rc.x2(); x += 2)
                        g->drawVLine(color,
                                     x,
                                     rc.y + widget->border().top(),
                                     rc.h - widget->border().height());
                      break;
                  }
                });
  }
}

void Dock::onInitTheme(ui::InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  setBorder(gfx::Border(0));
  setChildSpacing(4 * ui::guiscale());

  for (int i = 0; i < kSides; ++i) {
    Widget* widget = m_sides[i];
    if (widget)
      widget->initTheme();
  }
}

bool Dock::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage: {
      auto* mouseMessage = static_cast<MouseMessage*>(msg);
      const gfx::Point& pos = mouseMessage->position();

      if (m_hit.sideIndex >= 0 || m_hit.dockable) {
        m_startPos = pos;

        if (m_hit.sideIndex >= 0)
          m_startSize = m_sizes[m_hit.sideIndex];

        captureMouse();

        if (m_hit.dockable && !mouseMessage->right()) {
          m_dragging = true;
        }

        return true;
      }
      break;
    }

    case kMouseMoveMessage: {
      if (hasCapture()) {
        const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point pos = mouseMsg->position();
        const gfx::Point screenPos = mouseMsg->screenPosition();

        if (m_draggingFeedback)
          m_draggingFeedback->setFloatingPosition(pos);

        if (m_hit.sideIndex >= 0) {
          if (!display()->bounds().contains(pos) ||
              (m_hit.widget && m_hit.widget->parent() &&
               !m_hit.widget->parent()->bounds().contains(pos)))
            break; // Do not handle anything outside bounds.

          gfx::Size& sz = m_sizes[m_hit.sideIndex];
          gfx::Size minSize(16 * guiscale(), 16 * guiscale());

          if (m_hit.widget) {
            minSize.w = std::max(m_hit.widget->minSize().w, minSize.w);
            minSize.h = std::max(m_hit.widget->minSize().h, minSize.h);
          }

          switch (m_hit.sideIndex) {
            case kTopIndex: sz.h = std::max(m_startSize.h + pos.y - m_startPos.y, minSize.h); break;
            case kBottomIndex:
              sz.h = std::max(m_startSize.h - pos.y + m_startPos.y, minSize.h);
              break;
            case kLeftIndex:
              sz.w = std::max(m_startSize.w + pos.x - m_startPos.x, minSize.w);
              break;
            case kRightIndex:
              sz.w = std::max(m_startSize.w - pos.x + m_startPos.x, minSize.w);
              break;
          }

          layout();
          Resize();
        }
        else if (m_hit.widget && m_hit.dockable && m_dragging) {
          const Dockable* draggingDockable = m_hit.dockable;
          const int dockableAt = draggingDockable->dockableAt();

          if (!m_draggingFeedback)
            m_draggingFeedback = std::make_unique<DraggingFeedback>(display(), m_hit, pos);

          Widget* pick = manager()->pickFromScreenPos(screenPos);

          Widget* iter = pick;
          Dockable* targetDockable = nullptr;
          Dock* targetDock = nullptr;
          while (iter) {
            if (auto* dockable = dynamic_cast<Dockable*>(iter)) {
              if (!targetDockable)
                targetDockable = dockable;
            }
            if (auto* dock = dynamic_cast<Dock*>(iter)) {
              if (!targetDock && dock->isCustomizing())
                targetDock = dock;
            }
            iter = iter->parent();
          }

          if (!pick || !targetDockable || !targetDock)
            break;

          Hit target;
          if (targetDockable != draggingDockable) {
            target.widget = dynamic_cast<Widget*>(targetDockable);
            target.dockable = targetDockable;
            target.dock = targetDock;
          }

          gfx::Rect dropBounds = target.bounds();
          if (!dropBounds.isEmpty()) {
            gfx::PointF diff(pos - dropBounds.center());
            if (dropBounds.w / 2.0)
              diff.x /= dropBounds.w / 2.0f;
            if (dropBounds.h / 2.0)
              diff.y /= dropBounds.h / 2.0f;

            if ((dockableAt & LEFT) && diff.x < 0 && (ABS(diff.x) > ABS(diff.y))) {
              target.targetSide = LEFT;
            }
            else if ((dockableAt & RIGHT) && diff.x > 0 && (ABS(diff.x) > ABS(diff.y))) {
              target.targetSide = RIGHT;
            }
            else if ((dockableAt & TOP) && diff.y < 0 && (ABS(diff.x) < ABS(diff.y))) {
              target.targetSide = TOP;
            }
            else if ((dockableAt & BOTTOM) && diff.y > 0 && (ABS(diff.x) < ABS(diff.y))) {
              target.targetSide = BOTTOM;
            }
          }

          if (m_draggingFeedback->hit() != target)
            m_draggingFeedback->setHit(target);
        }
      }
      break;
    }

    case kMouseUpMessage: {
      if (hasCapture()) {
        releaseMouse();

        const auto* mouseMessage = static_cast<MouseMessage*>(msg);

        // Right-click to redock with popup menu.
        if (mouseMessage->right() && !m_dragging && m_hit.dockable) {
          auto* dockableWidget = dynamic_cast<Widget*>(m_hit.dockable);
          auto* widgetDock = dynamic_cast<Dock*>(dockableWidget->parent());

          int currentSide = widgetDock->whichSideChildIsDocked(dockableWidget);

          ASSERT(dockableWidget && widgetDock);

          Menu menu;
          MenuItem left(Strings::dock_left());
          MenuItem right(Strings::dock_right());
          MenuItem top(Strings::dock_top());
          MenuItem bottom(Strings::dock_bottom());

          if (m_hit.dockable->dockableAt() & ui::LEFT) {
            menu.addChild(&left);
          }
          if (m_hit.dockable->dockableAt() & ui::RIGHT) {
            menu.addChild(&right);
          }
          if (m_hit.dockable->dockableAt() & ui::TOP) {
            menu.addChild(&top);
          }
          if (m_hit.dockable->dockableAt() & ui::BOTTOM) {
            menu.addChild(&bottom);
          }

          switch (currentSide) {
            case ui::LEFT:   left.setEnabled(false); break;
            case ui::RIGHT:  right.setEnabled(false); break;
            case ui::TOP:    top.setEnabled(false); break;
            case ui::BOTTOM: bottom.setEnabled(false); break;
          }

          left.Click.connect([&] { redockWidget(widgetDock, dockableWidget, ui::LEFT); });
          right.Click.connect([&] { redockWidget(widgetDock, dockableWidget, ui::RIGHT); });
          top.Click.connect([&] { redockWidget(widgetDock, dockableWidget, ui::TOP); });
          bottom.Click.connect([&] { redockWidget(widgetDock, dockableWidget, ui::BOTTOM); });

          menu.showPopup(mouseMessage->position(), display());
          return true;
        }

        // Drop the widget to a new dock
        if (m_dragging && m_draggingFeedback) {
          const Hit target = m_draggingFeedback->hit();
          const Dockable* dockable = m_hit.dockable;
          Widget* widget = m_hit.widget;
          Dock* dock = target.dock;

          if (dock && dockable && widget && target.targetSide) {
            ASSERT(dockable->dockableAt() & target.targetSide);

            redockWidget(dock, widget, target.targetSide);

            m_draggingFeedback.reset();
            m_dragging = false;
            m_hit = Hit();
            return true;
          }
        }

        m_draggingFeedback.reset();
        m_dragging = false;

        // Call UserResizedDock signal after resizing a Dock splitter
        if (m_hit.sideIndex >= 0)
          onUserResizedDock();

        m_hit = Hit();
      }
      break;
    }

    case kSetCursorMessage: {
      const gfx::Point& pos = static_cast<MouseMessage*>(msg)->position();
      ui::CursorType cursor = ui::kArrowCursor;

      if (!hasCapture())
        m_hit = calcHit(pos);

      if (m_hit.sideIndex >= 0) {
        switch (m_hit.sideIndex) {
          case kTopIndex:
          case kBottomIndex: cursor = ui::kSizeNSCursor; break;
          case kLeftIndex:
          case kRightIndex:  cursor = ui::kSizeWECursor; break;
        }
      }
      else if (m_hit.dockable && m_hit.targetSide == 0) {
        cursor = ui::kMoveCursor;
      }

      ui::set_mouse_cursor(cursor);
      return true;
    }
  }
  return Widget::onProcessMessage(msg);
}

void Dock::onUserResizedDock()
{
  // Generate the UserResizedDock signal, this can be used to know
  // when the user modified the dock configuration to save the new
  // layout in a user/preference file.
  UserResizedDock();

  // Send the same notification for the parent (as probably eh
  // MainWindow is listening the signal of just the root dock).
  if (auto* parentDock = dynamic_cast<Dock*>(parent())) {
    parentDock->onUserResizedDock();
  }
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
  else if (auto* subdock = dynamic_cast<Dock*>(widget)) {
    align = subdock->calcAlign(i);
  }
  else if (auto* dockable2 = dynamic_cast<Dockable*>(widget)) {
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

    if (auto* subdock = dynamic_cast<Dock*>(widget)) {
      subdock->updateDockVisibility();
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
                                          const SideIndex index)> f)
{
  for (int i = 0; i < kSides; ++i) {
    auto* widget = m_sides[i];
    if (!widget || !widget->isVisible() || widget->isDecorative()) {
      continue;
    }

    const int spacing = (m_aligns[i] & EXPANSIVE ? childSpacing() : 0);
    const gfx::Size sz = (m_aligns[i] & EXPANSIVE ? m_sizes[i] : widget->sizeHint(bounds.size()));

    gfx::Rect rc, separator;
    switch (SideIndex(i)) {
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

    f(widget, rc, separator, SideIndex(i));
  }
}

void Dock::redockWidget(app::Dock* newDock, ui::Widget* dockableWidget, const int side)
{
  gfx::Size size;

  if (dockableWidget->id() == "timeline") {
    const gfx::Rect workspaceBounds = newDock->bounds();

    size.w = 64;
    size.h = 64;
    const auto timelineSplitterPos =
      get_config_double(kLegacyLayoutMainWindowSection, kLegacyLayoutTimelineSplitter, 75.0) /
      100.0;

    auto pos = gen::TimelinePosition::LEFT;
    size.w = (workspaceBounds.w * (1.0 - timelineSplitterPos)) / guiscale();

    if (side & RIGHT) {
      pos = gen::TimelinePosition::RIGHT;
    }
    if (side & BOTTOM || side & TOP) {
      pos = gen::TimelinePosition::BOTTOM;
      size.h = (workspaceBounds.h * (1.0 - timelineSplitterPos)) / guiscale();
    }
    Preferences::instance().general.timelinePosition(pos);
  }

  undock(dockableWidget);
  newDock->dock(side, dockableWidget, size);

  if (Dock* root = rootDock())
    root->layout();
  onUserResizedDock();
}

Dock* Dock::rootDock()
{
  Dock* root = this;
  Widget* iter = this->parent();
  while (iter) {
    if (auto* dock = dynamic_cast<Dock*>(iter)) {
      if (dock->isCustomizing())
        root = dock;
    }
    iter = iter->parent();
  }
  return root;
}

Dock::Hit Dock::calcHit(const gfx::Point& pos)
{
  Hit hit;
  forEachSide(childrenBounds(),
              [this, pos, &hit](ui::Widget* widget,
                                const gfx::Rect& widgetBounds,
                                const gfx::Rect& separator,
                                const int index) {
                if (separator.contains(pos)) {
                  hit.widget = widget;
                  hit.sideIndex = index;
                }
                else if (isCustomizing()) {
                  auto th = textHeight();
                  gfx::Rect rc = widgetBounds;
                  if (auto* dockable = dynamic_cast<Dockable*>(widget)) {
                    switch (dockable->dockHandleSide()) {
                      case ui::TOP:
                        rc.h = th;
                        if (rc.contains(pos)) {
                          hit.widget = widget;
                          hit.dockable = dockable;
                        }
                        break;
                      case ui::LEFT:
                        rc.w = th;
                        if (rc.contains(pos)) {
                          hit.widget = widget;
                          hit.dockable = dockable;
                        }
                        break;
                    }
                  }
                }
              });
  return hit;
}

} // namespace app
