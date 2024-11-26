// Aseprite UI Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/window.h"

#include "gfx/size.h"
#include "ui/button.h"
#include "ui/display.h"
#include "ui/fit_bounds.h"
#include "ui/graphics.h"
#include "ui/intern.h"
#include "ui/label.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/message_loop.h"
#include "ui/move_region.h"
#include "ui/resize_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <algorithm>

namespace ui {

using namespace gfx;

namespace {

enum {
  WINDOW_NONE = 0,
  WINDOW_MOVE = 1,
  WINDOW_RESIZE_LEFT = 2,
  WINDOW_RESIZE_RIGHT = 4,
  WINDOW_RESIZE_TOP = 8,
  WINDOW_RESIZE_BOTTOM = 16,
};

gfx::Point clickedMousePos;
gfx::Rect* clickedWindowPos = nullptr;

class WindowTitleLabel : public Label {
public:
  WindowTitleLabel(const std::string& text) : Label(text)
  {
    setDecorative(true);
    setType(kWindowTitleLabelWidget);
    initTheme();
  }
};

// Controls the "X" button in a window to close it.
class WindowCloseButton : public ButtonBase {
public:
  WindowCloseButton() : ButtonBase("", kWindowCloseButtonWidget, kButtonWidget, kButtonWidget)
  {
    setDecorative(true);
    initTheme();
  }

protected:
  void onClick() override
  {
    ButtonBase::onClick();
    closeWindow();
  }

  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kSetCursorMessage: ui::set_mouse_cursor(kArrowCursor); return true;

      case kKeyDownMessage:
        if (window()->shouldProcessEscKeyToCloseWindow() &&
            static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
          setSelected(true);
          return true;
        }
        break;

      case kKeyUpMessage:
        if (window()->shouldProcessEscKeyToCloseWindow() &&
            static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
          if (isSelected()) {
            setSelected(false);
            closeWindow();
            return true;
          }
        }
        break;
    }

    return ButtonBase::onProcessMessage(msg);
  }
};

} // anonymous namespace

Window::Window(Type type, const std::string& text)
  : Widget(kWindowWidget)
  , m_display(nullptr)
  , m_closer(nullptr)
  , m_titleLabel(nullptr)
  , m_closeButton(nullptr)
  , m_ownDisplay(false)
  , m_isDesktop(type == DesktopWindow)
  , m_isMoveable(!m_isDesktop)
  , m_isSizeable(!m_isDesktop)
  , m_isOnTop(false)
  , m_isWantFocus(true)
  , m_isForeground(false)
  , m_isAutoRemap(true)
{
  setVisible(false);
  setAlign(LEFT | MIDDLE);
  if (type == WithTitleBar) {
    setText(text);
    addChild(m_closeButton = new WindowCloseButton);
  }

  initTheme();
}

Window::~Window()
{
  if (auto man = manager())
    man->_closeWindow(this, isVisible());
}

Display* Window::display() const
{
  if (m_display)
    return m_display;
  else if (auto man = manager())
    return man->display();
  else
    return nullptr;
}

void Window::setDisplay(Display* display, const bool own)
{
  if (m_display)
    m_display->removeWindow(this);

  m_display = display;
  m_ownDisplay = own;

  if (m_display)
    m_display->addWindow(this);
}

void Window::setAutoRemap(bool state)
{
  m_isAutoRemap = state;
}

void Window::setMoveable(bool state)
{
  m_isMoveable = state;
}

void Window::setSizeable(bool state)
{
  m_isSizeable = state;
}

void Window::setOnTop(bool state)
{
  m_isOnTop = state;
}

void Window::setWantFocus(bool state)
{
  m_isWantFocus = state;
}

HitTest Window::hitTest(const gfx::Point& point)
{
  HitTestEvent ev(this, point, HitTestNowhere);
  onHitTest(ev);
  return ev.hit();
}

void Window::loadNativeFrame(const gfx::Rect& frame)
{
  m_lastFrame = frame;

  // Just in case the saved value is too small, we can take the value
  // as invalid.
  gfx::Size sz = sizeHint() * guiscale();
  if (display())
    sz *= display()->scale();
  if (m_lastFrame.w < sz.w / 5 || m_lastFrame.h < sz.h / 5) {
    m_lastFrame.setSize(sz);
  }
}

void Window::onClose(CloseEvent& ev)
{
  // Fire Close signal
  Close(ev);
}

void Window::onHitTest(HitTestEvent& ev)
{
  HitTest ht = HitTestNowhere;

  // If this window is not movable
  if (!m_isMoveable) {
    ev.setHit(ht);
    return;
  }

  // TODO check why this is necessary, there should be a bug in
  // the manager where we are receiving mouse events and are not
  // the top most window.
  Widget* picked = pick(ev.point());
  if (picked && picked != this && picked->type() != kWindowTitleLabelWidget) {
    ev.setHit(ht);
    return;
  }

  int x = ev.point().x;
  int y = ev.point().y;
  gfx::Rect pos = bounds();
  gfx::Rect cpos = childrenBounds();

  // Move
  if ((hasText()) &&
      (((x >= cpos.x) && (x < cpos.x2()) && (y >= pos.y + border().bottom()) && (y < cpos.y)))) {
    ht = HitTestCaption;
  }
  // Resize
  else if (m_isSizeable) {
#ifdef __APPLE__
    // TODO on macOS we cannot start resize actions on native windows
    if (ownDisplay()) {
      ev.setHit(ht);
      return;
    }
#endif

    if ((x >= pos.x) && (x < cpos.x)) {
      if ((y >= pos.y) && (y < cpos.y))
        ht = HitTestBorderNW;
      else if ((y > cpos.y2() - 1) && (y <= pos.y2() - 1))
        ht = HitTestBorderSW;
      else
        ht = HitTestBorderW;
    }
    else if ((y >= pos.y) && (y < cpos.y)) {
      if ((x >= pos.x) && (x < cpos.x))
        ht = HitTestBorderNW;
      else if ((x > cpos.x2() - 1) && (x <= pos.x2() - 1))
        ht = HitTestBorderNE;
      else
        ht = HitTestBorderN;
    }
    else if ((x > cpos.x2() - 1) && (x <= pos.x2() - 1)) {
      if ((y >= pos.y) && (y < cpos.y))
        ht = HitTestBorderNE;
      else if ((y > cpos.y2() - 1) && (y <= pos.y2() - 1))
        ht = HitTestBorderSE;
      else
        ht = HitTestBorderE;
    }
    else if ((y > cpos.y2() - 1) && (y <= pos.y2() - 1)) {
      if ((x >= pos.x) && (x < cpos.x))
        ht = HitTestBorderSW;
      else if ((x > cpos.x2() - 1) && (x <= pos.x2() - 1))
        ht = HitTestBorderSE;
      else
        ht = HitTestBorderS;
    }
  }
  else {
    // Client area
    ht = HitTestClient;
  }

  ev.setHit(ht);
}

void Window::onOpen(Event& ev)
{
  // Fire Open signal
  Open(ev);
}

void Window::onWindowResize()
{
  // Do nothing
}

void Window::onWindowMovement()
{
  // Do nothing
}

void Window::remapWindow()
{
  if (m_isAutoRemap) {
    m_isAutoRemap = false;
    setVisible(true);
  }

  expandWindow(sizeHint());

  // load layout
  loadLayout();

  invalidate();
}

void Window::centerWindow(Display* parentDisplay)
{
  if (m_isAutoRemap)
    remapWindow();

  if (!parentDisplay) {
    if (m_parentDisplay)
      parentDisplay = m_parentDisplay;
    else
      parentDisplay = manager()->getDefault()->display();
  }

  ASSERT(parentDisplay);

  if (m_isAutoRemap)
    remapWindow();

  const gfx::Size displaySize = parentDisplay->size();
  const gfx::Size windowSize = bounds().size();

  fit_bounds(parentDisplay,
             this,
             gfx::Rect(displaySize.w / 2 - windowSize.w / 2,
                       displaySize.h / 2 - windowSize.h / 2,
                       windowSize.w,
                       windowSize.h));
}

void Window::moveWindow(const gfx::Rect& rect)
{
  moveWindow(rect, true);
}

void Window::expandWindow(const gfx::Size& size)
{
  const gfx::Rect oldBounds = bounds();

  if (ownDisplay()) {
    os::Window* nativeWindow = display()->nativeWindow();
    const int scale = nativeWindow->scale();
    gfx::Rect frame = nativeWindow->frame();
    frame.setSize(size * scale);
    nativeWindow->setFrame(frame);
    setBounds(gfx::Rect(bounds().origin(), size));

    layout();
    invalidate();
  }
  else {
    setBounds(gfx::Rect(bounds().origin(), size));

    layout();
    manager()->invalidateRect(oldBounds);
  }
}

void Window::openWindow()
{
  if (!parent()) {
    Manager::getDefault()->_openWindow(this, m_isAutoRemap);

    // Open event
    Event ev(this);
    onOpen(ev);
  }
}

void Window::openWindowInForeground()
{
  m_isForeground = true;

  openWindow();

  Manager::getDefault()->_runModalWindow(this);

  m_isForeground = false;
}

void Window::closeWindow(Widget* closer)
{
  // Close event
  CloseEvent ev(closer);
  onBeforeClose(ev);
  if (ev.canceled())
    return;

  m_closer = closer;
  if (m_ownDisplay)
    m_lastFrame = m_display->nativeWindow()->frame();

  if (auto man = manager())
    man->_closeWindow(this, true);

  onClose(ev);
}

bool Window::isTopLevel()
{
  Widget* manager = this->manager();
  if (!manager->children().empty())
    return (this == UI_FIRST_WIDGET(manager->children()));
  else
    return false;
}

bool Window::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kOpenMessage:      m_closer = nullptr; break;

    case kCloseMessage:     saveLayout(); break;

    case kMouseDownMessage: {
      if (!m_isMoveable)
        break;

      clickedMousePos = static_cast<MouseMessage*>(msg)->position();
      m_hitTest = hitTest(clickedMousePos);

      if (m_hitTest != HitTestNowhere && m_hitTest != HitTestClient) {
        if (clickedWindowPos == nullptr)
          clickedWindowPos = new gfx::Rect(bounds());
        else
          *clickedWindowPos = bounds();

        // Handle native window action
        if (ownDisplay()) {
          os::WindowAction action = os::WindowAction::Cancel;
          switch (m_hitTest) {
            case HitTestCaption:  action = os::WindowAction::Move; break;
            case HitTestBorderNW: action = os::WindowAction::ResizeFromTopLeft; break;
            case HitTestBorderN:  action = os::WindowAction::ResizeFromTop; break;
            case HitTestBorderNE: action = os::WindowAction::ResizeFromTopRight; break;
            case HitTestBorderW:  action = os::WindowAction::ResizeFromLeft; break;
            case HitTestBorderE:  action = os::WindowAction::ResizeFromRight; break;
            case HitTestBorderSW: action = os::WindowAction::ResizeFromBottomLeft; break;
            case HitTestBorderS:  action = os::WindowAction::ResizeFromBottom; break;
            case HitTestBorderSE: action = os::WindowAction::ResizeFromBottomRight; break;
          }
          if (action != os::WindowAction::Cancel) {
            display()->nativeWindow()->performWindowAction(action, nullptr);

            // As Window::moveWindow() will not be called, we have to
            // call onWindowMovement() event from here.
            if (action == os::WindowAction::Move)
              onWindowMovement();

            return true;
          }
        }

        captureMouse();
        return true;
      }
      else
        break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
        set_mouse_cursor(kArrowCursor);

        if (clickedWindowPos != nullptr) {
          delete clickedWindowPos;
          clickedWindowPos = nullptr;
        }

        m_hitTest = HitTestNowhere;
        return true;
      }
      break;

    case kMouseMoveMessage:
      if (!m_isMoveable)
        break;

      // Does it have the mouse captured?
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        // Reposition/resize
        if (m_hitTest == HitTestCaption) {
          gfx::Point pos = clickedWindowPos->origin() + mousePos - clickedMousePos;
          gfx::Rect rect(pos, bounds().size());
          // Limit window position to avoid hiding the title bar
          limitPosition(rect);

          moveWindow(rect, true);
        }
        else {
          gfx::Size size = clickedWindowPos->size();

          bool hitLeft = (m_hitTest == HitTestBorderNW || m_hitTest == HitTestBorderW ||
                          m_hitTest == HitTestBorderSW);
          bool hitTop = (m_hitTest == HitTestBorderNW || m_hitTest == HitTestBorderN ||
                         m_hitTest == HitTestBorderNE);
          bool hitRight = (m_hitTest == HitTestBorderNE || m_hitTest == HitTestBorderE ||
                           m_hitTest == HitTestBorderSE);
          bool hitBottom = (m_hitTest == HitTestBorderSW || m_hitTest == HitTestBorderS ||
                            m_hitTest == HitTestBorderSE);

          if (hitLeft) {
            size.w += clickedMousePos.x - mousePos.x;
          }
          else if (hitRight) {
            size.w += mousePos.x - clickedMousePos.x;
          }

          if (hitTop) {
            size.h += (clickedMousePos.y - mousePos.y);
          }
          else if (hitBottom) {
            size.h += (mousePos.y - clickedMousePos.y);
          }

          limitSize(size);

          if (bounds().size() != size) {
            gfx::Point pos;
            if (hitLeft)
              pos.x = clickedWindowPos->x - (size.w - clickedWindowPos->w);
            else
              pos.x = bounds().x;

            if (hitTop)
              pos.y = clickedWindowPos->y - (size.h - clickedWindowPos->h);
            else
              pos.y = bounds().y;

            gfx::Rect rect(pos, size);
            // Limit window rectangle to avoid hiding its borders when resizing
            limitPosition(rect);

            moveWindow(rect, false);
            invalidate();
          }
        }
      }
      break;

    case kSetCursorMessage:
      if (m_isMoveable) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        HitTest ht = hitTest(mousePos);
        CursorType cursor = kArrowCursor;

        switch (ht) {
          case HitTestCaption:  cursor = kArrowCursor; break;
          case HitTestBorderNW: cursor = kSizeNWCursor; break;
          case HitTestBorderW:  cursor = kSizeWCursor; break;
          case HitTestBorderSW: cursor = kSizeSWCursor; break;
          case HitTestBorderNE: cursor = kSizeNECursor; break;
          case HitTestBorderE:  cursor = kSizeECursor; break;
          case HitTestBorderSE: cursor = kSizeSECursor; break;
          case HitTestBorderN:  cursor = kSizeNCursor; break;
          case HitTestBorderS:  cursor = kSizeSCursor; break;
        }

        set_mouse_cursor(cursor);
        return true;
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

// TODO similar to Manager::onInvalidateRegion
void Window::onInvalidateRegion(const gfx::Region& region)
{
  if (!ownDisplay()) {
    Widget::onInvalidateRegion(region);
    return;
  }

  if (!isVisible() || region.contains(bounds()) == gfx::Region::Out)
    return;

  Display* display = this->display();

  // Intersect only with window bounds, we don't need to use
  // getDrawableRegion() because each sub-window in the display will
  // be processed in the following for() loop
  gfx::Region reg1;
  reg1.createIntersection(region, gfx::Region(bounds()));

  // Redraw windows from top to background.
  for (auto window : display->getWindows()) {
    // Invalidating the manager only works for the main display, to
    // invalidate windows you have to invalidate them.
    if (window->ownDisplay()) {
      ASSERT(this == window);
      break;
    }

    // Invalidate regions of this window
    window->invalidateRegion(reg1);

    // Clip this window area for the next window.
    gfx::Region reg2;
    window->getRegion(reg2);
    reg1 -= reg2;
  }

  // TODO we should be able to modify m_updateRegion directly here,
  // so we avoid the getDrawableRegion() call from
  // Widget::onInvalidateRegion().
  if (!reg1.isEmpty())
    Widget::onInvalidateRegion(reg1);
}

void Window::onResize(ResizeEvent& ev)
{
  windowSetPosition(ev.bounds());
  // Fire Resize signal
  Resize(ev);
}

void Window::onSizeHint(SizeHintEvent& ev)
{
  Widget* manager = this->manager();

  if (m_isDesktop) {
    Rect cpos = manager->childrenBounds();
    ev.setSizeHint(cpos.w, cpos.h);
  }
  else {
    Size maxSize(0, 0);
    Size reqSize;

    if (m_titleLabel)
      maxSize.w = maxSize.h = 16 * guiscale();

    for (auto child : children()) {
      if (!child->isDecorative()) {
        reqSize = child->sizeHint();

        maxSize.w = std::max(maxSize.w, reqSize.w);
        maxSize.h = std::max(maxSize.h, reqSize.h);
      }
    }

    if (m_titleLabel)
      maxSize.w = std::max(maxSize.w, m_titleLabel->sizeHint().w);

    ev.setSizeHint(maxSize.w + border().width(), maxSize.h + border().height());
  }
}

void Window::onBroadcastMouseMessage(const gfx::Point& screenPos, WidgetsList& targets)
{
  if (!ownDisplay() || display()->nativeWindow()->frame().contains(screenPos))
    targets.push_back(this);

  // Continue sending the message to siblings windows until a desktop
  // or foreground window.
  if (isForeground() || isDesktop())
    return;

  Widget* sibling = nextSibling();
  if (sibling)
    sibling->broadcastMouseMessage(screenPos, targets);
}

void Window::onSetText()
{
  onBuildTitleLabel();
  Widget::onSetText();
  initTheme();
}

void Window::onVisible(bool visible)
{
  Widget::onVisible(visible);
  Display* display = this->display();
  if (ownDisplay() && display && display->nativeWindow()) {
    display->nativeWindow()->setVisible(visible);
  }
}

void Window::onBuildTitleLabel()
{
  if (text().empty()) {
    if (m_titleLabel) {
      removeChild(m_titleLabel);
      delete m_titleLabel;
      m_titleLabel = nullptr;
    }
  }
  else {
    if (!m_titleLabel) {
      m_titleLabel = new WindowTitleLabel(text());
      addChild(m_titleLabel);
    }
    else {
      m_titleLabel->setText(text());
      m_titleLabel->setBounds(gfx::Rect(m_titleLabel->bounds()).setSize(m_titleLabel->sizeHint()));
    }
  }
}

void Window::limitTitleLabelBounds()
{
  if (!m_titleLabel)
    return;

  // TODO Support themes with buttons at the left side
  int mostLeftMiniButtonX = bounds().x2();
  for (auto child : children()) {
    if (child->isDecorative() && child->type() != WidgetType::kWindowTitleLabelWidget &&
        child->bounds().x < mostLeftMiniButtonX) {
      mostLeftMiniButtonX = child->bounds().x;
    }
  }

  gfx::Rect titleBounds = m_titleLabel->bounds();
  if (titleBounds.x2() > mostLeftMiniButtonX) {
    titleBounds.w = mostLeftMiniButtonX - titleBounds.x;
    m_titleLabel->setBounds(titleBounds);
  }
}

void Window::windowSetPosition(const gfx::Rect& rect)
{
  m_isResizing = (bounds().size() != rect.size());

  // Copy the new position rectangle
  setBoundsQuietly(rect);
  Rect cpos = childrenBounds();

  // Set all the children to the same "cpos"
  for (auto child : children()) {
    if (child->isDecorative())
      child->setDecorativeWidgetBounds();
    else
      child->setBounds(cpos);
  }

  // Title label bounds adjustment due to decorative buttons. This
  // must be done after all decorative widgets have their final
  // position. It has been seen that it's not convenient to do it
  // before this point.
  limitTitleLabelBounds();

  onWindowResize();
  m_isResizing = false;
}

void Window::limitSize(gfx::Size& size)
{
  size.w = std::max(size.w, border().width());
  size.h = std::max(size.h, border().height());
}

void Window::limitPosition(gfx::Rect& rect)
{
  auto parent = this->parent();
  ASSERT(parent);
  if (!parent)
    return;

  if (rect.y < 0) {
    rect.y = 0;
    rect.h = bounds().h;
  }
  auto titlebarH = childrenBounds().y - bounds().y;
  auto limitB = parent->bounds().y2() - titlebarH;
  if (rect.y > limitB) {
    rect.y = limitB;
    rect.h = bounds().h;
  }
  int dx = rect.w - bounds().w;
  auto limitL = border().right() - bounds().w;
  if (rect.x + dx < limitL) {
    rect.x = limitL;
    rect.w = bounds().w;
  }
  auto limitR = parent->bounds().x2() - border().right();
  if (rect.x > limitR) {
    rect.x = limitR;
    rect.w = bounds().w;
  }
}

void Window::moveWindow(const gfx::Rect& rect, bool use_blit)
{
  Manager* manager = this->manager();

  // Discard enqueued kWinMoveMessage for this window because we are
  // going to send a new kWinMoveMessage with the latest window
  // bounds.
  manager->removeMessagesFor(this, kWinMoveMessage);

  // Get the window's current position
  Rect old_pos = bounds();
  int dx = rect.x - old_pos.x;
  int dy = rect.y - old_pos.y;

  // Get the manager's current position
  Rect man_pos = manager->bounds();

  // Send a kWinMoveMessage message to the window
  Message* msg = new Message(kWinMoveMessage);
  msg->setRecipient(this);
  manager->enqueueMessage(msg);

  // Get the region & the drawable region of the window
  Region oldDrawableRegion;
  getDrawableRegion(oldDrawableRegion, kCutTopWindowsAndUseChildArea);

  // If the size of the window changes...
  if (old_pos.w != rect.w || old_pos.h != rect.h) {
    // We have to change the position of all children.
    windowSetPosition(rect);
  }
  else {
    // We can just displace all the widgets by a delta (new_position -
    // old_position)...
    offsetWidgets(dx, dy);
  }

  // Get the new drawable region of the window (it's new because we
  // moved the window to "rect")
  Region newDrawableRegion;
  getDrawableRegion(newDrawableRegion, kCutTopWindowsAndUseChildArea);

  // First of all, we have to find the manager region to invalidate,
  // it's the old window drawable region without the new window
  // drawable region.
  Region invalidManagerRegion;
  invalidManagerRegion.createSubtraction(oldDrawableRegion, newDrawableRegion);

  // In second place, we have to setup the window invalid region...

  // If the GPU acceleration is enabled on this window we avoid
  // copying regions of pixels as it's super slow to read GPU
  // surfaces.
  if (display()->nativeWindow()->gpuAcceleration()
#if LAF_LINUX
      // On X11 it's better to avoid copying screen areas
      || true
#endif
  ) {
    use_blit = false;
  }

  // If "use_blit" isn't activated, we have to redraw the whole window
  // (sending kPaintMessage messages) in the new drawable region
  if (!use_blit) {
    invalidateRegion(newDrawableRegion);
  }
  // If "use_blit" is activated, we can move the old drawable to the
  // new position (to redraw as little as possible).
  else {
    Region reg1;
    reg1 = newDrawableRegion;
    reg1.offset(-dx, -dy);

    Region moveableRegion;
    moveableRegion.createIntersection(oldDrawableRegion, reg1);

    // Move the window's graphics
    Display* display = this->display();
    ScreenGraphics g(display);
    hide_mouse_cursor();
    {
      IntersectClip clip(&g, man_pos);
      if (clip) {
        ui::move_region(display, moveableRegion, dx, dy);
      }
    }
    show_mouse_cursor();

    reg1.createSubtraction(reg1, moveableRegion);
    reg1.offset(dx, dy);
    invalidateRegion(reg1);
  }

  manager->invalidateRegion(invalidManagerRegion);

  onWindowMovement();
}

} // namespace ui
