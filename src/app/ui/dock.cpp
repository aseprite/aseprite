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

int side_from_index(int index)
{
  switch (index) {
    case kTopIndex:    return ui::TOP;
    case kBottomIndex: return ui::BOTTOM;
    case kLeftIndex:   return ui::LEFT;
    case kRightIndex:  return ui::RIGHT;
  }
  return ui::CENTER; // kCenterIndex
}

} // anonymous namespace

// TODO: Duplicated from main_window.cpp
static constexpr auto kLegacyLayoutMainWindowSection = "layout:main_window";
static constexpr auto kLegacyLayoutTimelineSplitter = "timeline_splitter";

Dock::DropzonePlaceholder::DropzonePlaceholder(Widget* dragWidget, const gfx::Point& mousePosition)
  : Widget(kGenericWidget)
{
  setExpansive(true);
  setSizeHint(dragWidget->sizeHint());
  setMinSize(dragWidget->size());

  m_mouseOffset = mousePosition - dragWidget->bounds().origin();

  const os::SurfaceRef surface = os::System::instance()->makeRgbaSurface(dragWidget->size().w,
                                                                         dragWidget->size().h);
  {
    const os::SurfaceLock lock(surface.get());
    Paint paint;
    paint.color(gfx::rgba(0, 0, 0, 0));
    paint.style(os::Paint::Fill);
    surface->drawRect(gfx::Rect(0, 0, surface->width(), surface->height()), paint);
  }

  {
    Graphics g(display(), surface, 0, 0);
    g.setFont(font());

    Paint paint;
    paint.color(gfx::rgba(0, 0, 0, 200));

    // TODO: This will render any open things, especially the preview editor, need to close or hide
    // that for a frame or paint the widget itself to a surface instead of croppping the backLayer.
    auto backLayerSurface = display()->backLayer()->surface();
    g.drawSurface(backLayerSurface.get(),
                  dragWidget->bounds(),
                  gfx::Rect(0, 0, surface->width(), surface->height()),
                  os::Sampling(),
                  &paint);
  }

  m_floatingUILayer = UILayer::Make();
  m_floatingUILayer->setSurface(surface);
  m_floatingUILayer->setPosition(dragWidget->bounds().origin());
  display()->addLayer(m_floatingUILayer);
}

inline Dock::DropzonePlaceholder::~DropzonePlaceholder()
{
  display()->removeLayer(m_floatingUILayer);
}

void Dock::DropzonePlaceholder::setGhostPosition(const gfx::Point& position) const
{
  ASSERT(m_floatingUILayer);

  display()->dirtyRect(m_floatingUILayer->bounds());
  m_floatingUILayer->setPosition(position - m_mouseOffset);
  display()->dirtyRect(m_floatingUILayer->bounds());
}

void Dock::DropzonePlaceholder::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  gfx::Rect bounds = clientBounds();

  g->fillRect(bgColor(), bounds);

  bounds.shrink(2 * guiscale());

  const auto* theme = SkinTheme::get(this);
  const gfx::Color color = theme->colors.workspaceText();

  g->drawRect(color, bounds);
  g->drawLine(color, bounds.center(), bounds.origin());
  g->drawLine(color, bounds.center(), bounds.point2());
  g->drawLine(color, bounds.center(), bounds.point2() - gfx::Point(bounds.w, 0));
  g->drawLine(color, bounds.center(), bounds.origin() + gfx::Point(bounds.w, 0));
  g->drawRect(
    color,
    gfx::Rect(bounds.center() - gfx::Point(2, 2) * guiscale(), gfx::Size(4, 4) * guiscale()));
}

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

    if (parentDock != this && parentDock->children().empty()) {
      undock(parentDock);
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
      return side_from_index(i);
  return 0;
}

const gfx::Size Dock::getUserDefinedSizeAtSide(int side) const
{
  int i = side_index(side);
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
    if (!widget || !widget->isVisible() || widget->isDecorative()) {
      m_sizes[i] = gfx::Size(0, 0);
      continue;
    }

    const int spacing = (m_aligns[i] & EXPANSIVE ? childSpacing() : 0);
    const auto hint = m_sides[i]->sizeHint(fitIn);
    m_sizes[i] = hint;

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
                  if (isCustomizing()) {
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
        const gfx::Point& pos = static_cast<MouseMessage*>(msg)->position();

        if (m_dropzonePlaceholder)
          m_dropzonePlaceholder->setGhostPosition(pos);

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
        else if (m_hit.dockable && m_dragging) {
          invalidate();

          auto* parentDock = dynamic_cast<Dock*>(m_hit.widget->parent());
          ASSERT(parentDock);

          if (!parentDock)
            break;

          if (!m_dropzonePlaceholder)
            m_dropzonePlaceholder.reset(new DropzonePlaceholder(m_hit.widget, pos));

          auto dockedAt = parentDock->whichSideChildIsDocked(m_hit.widget);
          const auto& bounds = parentDock->bounds();

          if (!bounds.contains(pos))
            break; // Do not handle anything outside the bounds of the dock.

          const int kBufferZone =
            std::max(12 * guiscale(), std::min(m_hit.widget->size().w, m_hit.widget->size().h));

          int newTargetSide = -1;
          if (m_hit.dockable->dockableAt() & LEFT && !(dockedAt & LEFT) &&
              pos.x < bounds.x + kBufferZone) {
            newTargetSide = LEFT;
          }
          else if (m_hit.dockable->dockableAt() & RIGHT && !(dockedAt & RIGHT) &&
                   pos.x > (bounds.w - kBufferZone)) {
            newTargetSide = RIGHT;
          }
          else if (m_hit.dockable->dockableAt() & TOP && !(dockedAt & TOP) &&
                   pos.y < bounds.y + kBufferZone) {
            newTargetSide = TOP;
          }
          else if (m_hit.dockable->dockableAt() & BOTTOM && !(dockedAt & BOTTOM) &&
                   pos.y > (bounds.h - kBufferZone)) {
            newTargetSide = BOTTOM;
          }

          if (m_hit.targetSide == newTargetSide)
            break;

          m_hit.targetSide = newTargetSide;

          // Always undock the placeholder
          if (m_dropzonePlaceholder && m_dropzonePlaceholder->parent()) {
            auto* placeholderCurrentDock = dynamic_cast<Dock*>(m_dropzonePlaceholder->parent());
            placeholderCurrentDock->undock(m_dropzonePlaceholder.get());
          }

          if (m_dropzonePlaceholder && m_hit.targetSide != -1) {
            parentDock->dock(m_hit.targetSide,
                             m_dropzonePlaceholder.get(),
                             m_hit.widget->sizeHint());
          }

          layout();
        }
      }
      break;
    }

    case kMouseUpMessage: {
      if (hasCapture()) {
        releaseMouse();
        const auto* mouseMessage = static_cast<MouseMessage*>(msg);

        if (m_dropzonePlaceholder && m_dropzonePlaceholder->parent()) {
          // Always undock the dropzone placeholder to avoid dangling sizes.
          auto* placeholderCurrentDock = dynamic_cast<Dock*>(m_dropzonePlaceholder->parent());
          placeholderCurrentDock->undock(m_dropzonePlaceholder.get());
        }

        if (m_hit.dockable) {
          auto* dockableWidget = dynamic_cast<Widget*>(m_hit.dockable);
          auto* widgetDock = dynamic_cast<Dock*>(dockableWidget->parent());

          int currentSide = widgetDock->whichSideChildIsDocked(dockableWidget);

          assert(dockableWidget && widgetDock);

          if (mouseMessage->right() && !m_dragging) {
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
            return false;
          }
          else if (m_hit.targetSide > 0 && m_dragging) {
            ASSERT(m_hit.dockable->dockableAt() & m_hit.targetSide);
            redockWidget(widgetDock, dockableWidget, m_hit.targetSide);
            m_dropzonePlaceholder = nullptr;
            m_dragging = false;
            m_hit = Hit();
            return false;
          }
        }

        m_dropzonePlaceholder = nullptr;
        m_dragging = false;
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
      else if (m_hit.dockable && m_hit.targetSide == -1) {
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
                                          const int index)> f)
{
  for (int i = 0; i < kSides; ++i) {
    auto* widget = m_sides[i];
    if (!widget || !widget->isVisible() || widget->isDecorative()) {
      continue;
    }

    const int spacing = (m_aligns[i] & EXPANSIVE ? childSpacing() : 0);
    const gfx::Size sz = (m_aligns[i] & EXPANSIVE ? m_sizes[i] : widget->sizeHint(bounds.size()));

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

void Dock::redockWidget(app::Dock* widgetDock, ui::Widget* dockableWidget, const int side)
{
  const gfx::Rect workspaceBounds = widgetDock->bounds();

  gfx::Size size;
  if (dockableWidget->id() == "timeline") {
    size.w = 64;
    size.h = 64;
    auto timelineSplitterPos =
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

  widgetDock->undock(dockableWidget);
  widgetDock->dock(side, dockableWidget, size);

  App::instance()->mainWindow()->invalidate();
  layout();
  onUserResizedDock();
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
