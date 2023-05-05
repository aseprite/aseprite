// Aseprite UI Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

// #define REPORT_EVENTS
// #define REPORT_FOCUS_MOVEMENT
// #define DEBUG_PAINT_EVENTS
// #define LIMIT_DISPATCH_TIME
// #define DEBUG_UI_THREADS
#define GARBAGE_TRACE(...)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"

#include "base/concurrent_queue.h"
#include "base/scoped_value.h"
#include "base/thread.h"
#include "base/time.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "os/window_spec.h"
#include "ui/intern.h"
#include "ui/ui.h"

#if defined(DEBUG_PAINT_EVENTS) || defined(DEBUG_UI_THREADS)
#include <thread>
#endif

#include <algorithm>
#include <limits>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#if defined(_WIN32) && defined(DEBUG_PAINT_EVENTS)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #undef min
  #undef max
#endif

namespace ui {

namespace {

// The redraw state is used to avoid drawing the manager when a window
// has been just closed by the user, so we delay the redrawing (the
// kPaintMessages generation) for the next generateMessages() round.
enum class RedrawState {
   Normal,
   AWindowHasJustBeenClosed,
   RedrawDelayed,
   ClosingApp,
};
RedrawState redrawState = RedrawState::Normal;

} // anonymous namespace

static const int NFILTERS = (int)(kFirstRegisteredMessage+1);

struct Filter {
  int message;
  Widget* widget;

  Filter(int message, Widget* widget)
    : message(message)
    , widget(widget) { }
};

typedef std::list<Message*> Messages;
typedef std::list<Filter*> Filters;

Manager* Manager::m_defaultManager = nullptr;

#ifdef DEBUG_UI_THREADS
static std::thread::id manager_thread;
#endif

static WidgetsList mouse_widgets_list; // List of widgets to send mouse events
static Messages msg_queue;             // Messages queue
static Messages used_msg_queue;        // Messages queue
static base::concurrent_queue<Message*> concurrent_msg_queue;
static Filters msg_filters[NFILTERS]; // Filters for every enqueued message
static int filter_locks = 0;

// Current display with the mouse, used to avoid processing a
// os::Event::MouseLeave of the non-current display/window as when we
// move the mouse between two windows we can receive:
//
//   1. A os::Event::MouseEnter of the new window
//   2. A os::Event::MouseLeave of the old window
//
// Instead of the MouseLeave event of the old window first.
static Display* mouse_display = nullptr;

static Widget* focus_widget;    // The widget with the focus
static Widget* mouse_widget;    // The widget with the mouse
static Widget* capture_widget;  // The widget that captures the mouse

static bool first_time = true;    // true when we don't enter in poll yet

// Don't adjust window positions automatically when it's false. Used
// when Screen/UI scaling is changed to avoid adjusting windows as
// when the os::Display is resized by the user.
static bool auto_window_adjustment = true;

// Keyboard focus movement stuff
inline bool does_accept_focus(Widget* widget)
{
  return ((((widget)->flags() & (FOCUS_STOP |
                                 DISABLED |
                                 HIDDEN |
                                 DECORATIVE)) == FOCUS_STOP) &&
          ((widget)->isVisible()));
}

static int count_widgets_accept_focus(Widget* widget);
static bool child_accept_focus(Widget* widget, bool first);
static Widget* next_widget(Widget* widget);
static int cmp_left(Widget* widget, int x, int y);
static int cmp_right(Widget* widget, int x, int y);
static int cmp_up(Widget* widget, int x, int y);
static int cmp_down(Widget* widget, int x, int y);

namespace {

class LockFilters {
public:
  LockFilters() {
    ++filter_locks;
  }
  ~LockFilters() {
    ASSERT(filter_locks > 0);
    --filter_locks;

    if (filter_locks == 0) {
      // Clear empty filters
      for (Filters& msg_filter : msg_filters) {
        for (auto it = msg_filter.begin(); it != msg_filter.end(); ) {
          Filter* filter = *it;
          if (filter->widget == nullptr) {
            delete filter;
            it = msg_filter.erase(it);
          }
          else {
            ++it;
          }
        }
      }
    }
  }
};

os::Hit handle_native_hittest(os::Window* osWindow,
                              const gfx::Point& pos)
{
  Display* display = Manager::getDisplayFromNativeWindow(osWindow);
  if (display) {
    auto window = static_cast<Window*>(display->containedWidget());
    switch (window->hitTest(pos)) {
      case HitTestNowhere:  return os::Hit::Content;
      case HitTestCaption:  return os::Hit::TitleBar;
      case HitTestClient:   return os::Hit::Content;
      case HitTestBorderNW: return os::Hit::TopLeft;
      case HitTestBorderN:  return os::Hit::Top;
      case HitTestBorderNE: return os::Hit::TopRight;
      case HitTestBorderE:  return os::Hit::Right;
      case HitTestBorderSE: return os::Hit::BottomRight;
      case HitTestBorderS:  return os::Hit::Bottom;
      case HitTestBorderSW: return os::Hit::BottomLeft;
      case HitTestBorderW:  return os::Hit::Left;
    }
  }
  return os::Hit::None;
}

} // anonymous namespace

// static
bool Manager::widgetAssociatedToManager(Widget* widget)
{
  return (focus_widget == widget ||
          mouse_widget == widget ||
          capture_widget == widget ||
          std::find(mouse_widgets_list.begin(),
                    mouse_widgets_list.end(),
                    widget) != mouse_widgets_list.end());
}

Manager::Manager(const os::WindowRef& nativeWindow)
  : Widget(kManagerWidget)
  , m_display(nullptr, nativeWindow, this)
  , m_eventQueue(os::instance()->eventQueue())
  , m_lockedWindow(nullptr)
  , m_mouseButton(kButtonNone)
{
  // The native window can be nullptr when running tests
  if (nativeWindow)
    nativeWindow->setUserData(&m_display);

#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::thread::id());
  manager_thread = std::this_thread::get_id();
#endif

  if (!m_defaultManager) {
    // Empty lists
    ASSERT(msg_queue.empty());
    mouse_widgets_list.clear();

    // Reset variables
    focus_widget = nullptr;
    mouse_widget = nullptr;
    capture_widget = nullptr;
  }

  setBounds(m_display.bounds());
  setVisible(true);

  // Default manager is the first one (and is always visible).
  if (!m_defaultManager)
    m_defaultManager = this;

  // TODO check if this is needed
  onNewDisplayConfiguration(&m_display);
}

Manager::~Manager()
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  // There are some messages in queue? Dispatch everything.
  dispatchMessages();
  collectGarbage();

  // Finish the main manager.
  if (m_defaultManager == this) {
    // No more cursor
    set_mouse_cursor(kNoCursor);

    // Check timers & filters
#ifdef _DEBUG
    if (get_app_state() != AppState::kClosingWithException) {
      ASSERT(!Timer::haveTimers());
      for (Filters& msg_filter : msg_filters)
        ASSERT(msg_filter.empty());
    }
    ASSERT(msg_queue.empty());
#endif

    // No more default manager
    m_defaultManager = nullptr;

    // Shutdown system
    mouse_widgets_list.clear();
  }
}

// static
Display* Manager::getDisplayFromNativeWindow(os::Window* window)
{
  if (window)
    return window->userData<Display>();
  else
    return nullptr;
}

void Manager::run()
{
  MessageLoop loop(this);

  if (first_time) {
    first_time = false;

    invalidate();
    set_mouse_cursor(kArrowCursor);
  }

  while (!children().empty())
    loop.pumpMessages();
}

void Manager::flipAllDisplays()
{
  OverlayManager* overlays = OverlayManager::instance();

  update_cursor_overlay();

  // Draw overlays.
  overlays->drawOverlays();

  m_display.flipDisplay();
  if (get_multiple_displays()) {
    for (auto child : children()) {
      auto window = static_cast<Window*>(child);
      if (window->ownDisplay())
        window->display()->flipDisplay();
    }
  }
}

void Manager::updateAllDisplaysWithNewScale(int scale)
{
  os::Window* nativeWindow = m_display.nativeWindow();
  nativeWindow->setScale(scale);

  if (get_multiple_displays()) {
    for (auto child : children()) {
      auto window = static_cast<Window*>(child);
      if (window->ownDisplay()) {
        Display* display = static_cast<Window*>(child)->display();
        display->nativeWindow()->setScale(scale);
        onNewDisplayConfiguration(display);
      }
    }
  }

  onNewDisplayConfiguration(&m_display);
}

bool Manager::generateMessages()
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  // First check: there are windows to manage?
  if (children().empty())
    return false;

  // Generate messages from other threads
  if (!concurrent_msg_queue.empty()) {
    Message* msg = nullptr;
    while (concurrent_msg_queue.try_pop(msg))
      msg_queue.push_back(msg);
  }

  // Generate messages from OS input
  generateMessagesFromOSEvents();

  // Generate messages for timers
  Timer::pollTimers();

  // Returns true if we have to dispatch messages (if the redraw was
  // delayed, we have to pump messages because there is where paint
  // messages are flushed)
  if (!msg_queue.empty() || redrawState != RedrawState::Normal)
    return true;
  else
    return false;
}

void Manager::generateSetCursorMessage(Display* display,
                                       const gfx::Point& mousePos,
                                       KeyModifiers modifiers,
                                       PointerType pointerType)
{
  if (get_mouse_cursor() == kOutsideDisplay)
    return;

  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  if (dst)
    enqueueMessage(
      newMouseMessage(
        kSetCursorMessage,
        display, dst,
        mousePos,
        pointerType,
        m_mouseButton,
        modifiers));
  else
    set_mouse_cursor(kArrowCursor);
}

static MouseButton mouse_button_from_os_to_ui(const os::Event& osEvent)
{
  static_assert((int)os::Event::NoneButton == (int)ui::kButtonNone &&
                (int)os::Event::LeftButton == (int)ui::kButtonLeft &&
                (int)os::Event::RightButton == (int)ui::kButtonRight &&
                (int)os::Event::MiddleButton == (int)ui::kButtonMiddle &&
                (int)os::Event::X1Button == (int)ui::kButtonX1 &&
                (int)os::Event::X2Button == (int)ui::kButtonX2,
                "Mouse button constants do not match");
  return (MouseButton)osEvent.button();
}

void Manager::generateMessagesFromOSEvents()
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  os::Event lastMouseMoveEvent;

  // Events from laf-os
  os::Event osEvent;
  for (;;) {
    // Calculate how much time we can wait for the next message in the
    // event queue.
    double timeout = 0.0;
    if (msg_queue.empty() && redrawState == RedrawState::Normal) {
      if (!Timer::getNextTimeout(timeout))
        timeout = os::EventQueue::kWithoutTimeout;
    }

    if (timeout == os::EventQueue::kWithoutTimeout && used_msg_queue.empty())
      collectGarbage();
#if _DEBUG
    else if (!m_garbage.empty()) {
      GARBAGE_TRACE("collectGarbage() wasn't called #objects=%d"
                    " (msg_queue=%d used_msg_queue=%d redrawState=%d timeout=%.16g)\n",
                    int(m_garbage.size()),
                    msg_queue.size(),
                    used_msg_queue.size(),
                    int(redrawState),
                    timeout);
    }
#endif

    m_eventQueue->getEvent(osEvent, timeout);
    if (osEvent.type() == os::Event::None)
      break;

    Display* display = getDisplayFromNativeWindow(osEvent.window().get());
    if (!display)
      display = this->display();

    switch (osEvent.type()) {

      case os::Event::CloseApp: {
        Message* msg = new Message(kCloseDisplayMessage);
        msg->setDisplay(display);
        msg->setRecipient(this);
        msg->setPropagateToChildren(true);
        enqueueMessage(msg);
        break;
      }

      case os::Event::CloseWindow: {
        Message* msg = new Message(kCloseDisplayMessage);
        msg->setDisplay(display);
        msg->setRecipient(this);
        msg->setPropagateToChildren(true);
        enqueueMessage(msg);
        break;
      }

      case os::Event::ResizeWindow: {
        Message* msg = new Message(kResizeDisplayMessage);
        msg->setDisplay(display);
        msg->setRecipient(this);
        msg->setPropagateToChildren(false);
        enqueueMessage(msg);
        break;
      }

      case os::Event::DropFiles: {
        Message* msg = new DropFilesMessage(osEvent.files());
        msg->setDisplay(display);
        msg->setRecipient(this);
        enqueueMessage(msg);
        break;
      }

      case os::Event::KeyDown:
      case os::Event::KeyUp: {
        Message* msg = new KeyMessage(
          (osEvent.type() == os::Event::KeyDown ?
             kKeyDownMessage:
             kKeyUpMessage),
          osEvent.scancode(),
          osEvent.modifiers(),
          osEvent.unicodeChar(),
          osEvent.repeat());

        msg->setDisplay(display);

        if (osEvent.isDeadKey())
          static_cast<KeyMessage*>(msg)->setDeadKey(true);

        broadcastKeyMsg(msg);
        enqueueMessage(msg);
        break;
      }

      case os::Event::MouseEnter: {
        if (get_multiple_displays()) {
          if (osEvent.window()) {
            ASSERT(display != nullptr);
            _internal_set_mouse_display(display);
          }
        }
        set_mouse_cursor(kArrowCursor);
        lastMouseMoveEvent = osEvent;
        mouse_display = display;
        break;
      }

      case os::Event::MouseLeave: {
        if (mouse_display == display) {
          set_mouse_cursor(kOutsideDisplay);
          setMouse(nullptr);

          _internal_no_mouse_position();
          mouse_display = nullptr;

          // To avoid calling kSetCursorMessage when the mouse leaves
          // the window.
          lastMouseMoveEvent = os::Event();
        }
        break;
      }

      case os::Event::MouseMove: {
        handleMouseMove(
          display,
          osEvent.position(),
          osEvent.modifiers(),
          osEvent.pointerType(),
          osEvent.pressure());
        lastMouseMoveEvent = osEvent;
        break;
      }

      case os::Event::MouseDown: {
        handleMouseDown(
          display,
          osEvent.position(),
          m_mouseButton = mouse_button_from_os_to_ui(osEvent),
          osEvent.modifiers(),
          osEvent.pointerType(),
          osEvent.pressure());
        break;
      }

      case os::Event::MouseUp: {
        handleMouseUp(
          display,
          osEvent.position(),
          mouse_button_from_os_to_ui(osEvent),
          osEvent.modifiers(),
          osEvent.pointerType());
        m_mouseButton = kButtonNone;
        break;
      }

      case os::Event::MouseDoubleClick: {
        handleMouseDoubleClick(
          display,
          osEvent.position(),
          m_mouseButton = mouse_button_from_os_to_ui(osEvent),
          osEvent.modifiers(),
          osEvent.pointerType(),
          osEvent.pressure());
        break;
      }

      case os::Event::MouseWheel: {
        handleMouseWheel(display,
                         osEvent.position(),
                         osEvent.modifiers(),
                         osEvent.pointerType(),
                         osEvent.wheelDelta(),
                         osEvent.preciseWheel());
        break;
      }

      case os::Event::TouchMagnify: {
        handleTouchMagnify(display,
                           osEvent.position(),
                           osEvent.modifiers(),
                           osEvent.magnification());
        break;
      }

      case os::Event::Callback: {
        // Call from the UI thread
        osEvent.execCallback();
        break;
      }

    }
  }

  // Generate just one kSetCursorMessage for the last mouse position
  if (lastMouseMoveEvent.type() != os::Event::None) {
    osEvent = lastMouseMoveEvent;

    Display* display = getDisplayFromNativeWindow(osEvent.window().get());
    if (!display)
      display = this->display();

    generateSetCursorMessage(display,
                             osEvent.position(),
                             osEvent.modifiers(),
                             osEvent.pointerType());
  }
}

void Manager::handleMouseMove(Display* display,
                              const gfx::Point& mousePos,
                              const KeyModifiers modifiers,
                              const PointerType pointerType,
                              const float pressure)
{
  updateMouseWidgets(mousePos, display);

  // Send the mouse movement message
  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  enqueueMessage(
    newMouseMessage(
      kMouseMoveMessage,
      display, dst,
      mousePos,
      pointerType,
      m_mouseButton,
      modifiers,
      gfx::Point(0, 0),
      false,
      pressure));
}

void Manager::handleMouseDown(Display* display,
                              const gfx::Point& mousePos,
                              MouseButton mouseButton,
                              KeyModifiers modifiers,
                              PointerType pointerType,
                              const float pressure)
{
  // Returns false in case that we click another Display (os::Window)
  // that is not the current running top-most foreground window.
  if (!handleWindowZOrder())
    return;

  enqueueMessage(
    newMouseMessage(
      kMouseDownMessage,
      display,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos,
      pointerType,
      mouseButton,
      modifiers,
      gfx::Point(0, 0),
      false,
      pressure));
}

void Manager::handleMouseUp(Display* display,
                            const gfx::Point& mousePos,
                            MouseButton mouseButton,
                            KeyModifiers modifiers,
                            PointerType pointerType)
{
  enqueueMessage(
    newMouseMessage(
      kMouseUpMessage,
      display,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos,
      pointerType,
      mouseButton,
      modifiers));
}

void Manager::handleMouseDoubleClick(Display* display,
                                     const gfx::Point& mousePos,
                                     MouseButton mouseButton,
                                     KeyModifiers modifiers,
                                     PointerType pointerType,
                                     const float pressure)
{
  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  if (dst) {
    enqueueMessage(
      newMouseMessage(
        kDoubleClickMessage,
        display, dst, mousePos, pointerType,
        mouseButton, modifiers,
        gfx::Point(0, 0), false,
        pressure));
  }
}

void Manager::handleMouseWheel(Display* display,
                               const gfx::Point& mousePos,
                               KeyModifiers modifiers,
                               PointerType pointerType,
                               const gfx::Point& wheelDelta,
                               bool preciseWheel)
{
  enqueueMessage(newMouseMessage(
      kMouseWheelMessage,
      display,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos, pointerType, m_mouseButton, modifiers,
      wheelDelta, preciseWheel));
}

void Manager::handleTouchMagnify(Display* display,
                                 const gfx::Point& mousePos,
                                 const KeyModifiers modifiers,
                                 const double magnification)
{
  Widget* widget = (capture_widget ? capture_widget: mouse_widget);
  if (widget) {
    Message* msg = new TouchMessage(
      kTouchMagnifyMessage,
      modifiers,
      mousePos,
      magnification);

    msg->setDisplay(display);
    msg->setRecipient(widget);

    enqueueMessage(msg);
  }
}

// Handles Z order: Send the window to top (only when you click in a
// window that aren't the desktop).
//
// TODO code similar to Display::handleWindowZOrder()
bool Manager::handleWindowZOrder()
{
  if (capture_widget || !mouse_widget)
    return true;

  // The clicked window
  Window* window = mouse_widget->window();
  Window* topWindow = getTopWindow();

  if ((window) &&
      // We cannot change Z-order of desktop windows
      (!window->isDesktop()) &&
      // We cannot change Z order of foreground windows because a
      // foreground window can launch other background windows
      // which should be kept on top of the foreground one.
      (!window->isForeground()) &&
      // If the window is not already the top window of the manager.
      (window != topWindow)) {
    // If there is already a top foreground window, cancel the z-order change
    if (topWindow && topWindow->isForeground())
      return false;

    base::ScopedValue<Widget*> scoped(m_lockedWindow, window);

    window->display()->handleWindowZOrder(window);

    // Put it in the top of the list
    removeChild(window);

    if (window->isOnTop())
      insertChild(0, window);
    else {
      int pos = (int)children().size();

      for (auto it=children().rbegin(),
             end=children().rend();
           it != end; ++it) {
        if (static_cast<Window*>(*it)->isOnTop())
          break;

        --pos;
      }
      insertChild(pos, window);
    }

    if (!window->ownDisplay())
      window->invalidate();
  }

  // Put the focus
  setFocus(mouse_widget);
  return true;
}

// If display is nullptr, mousePos is in screen coordinates, if not,
// it's relative to the display content rect.
void Manager::updateMouseWidgets(const gfx::Point& mousePos,
                                 Display* display)
{
  gfx::Point screenPos;
  if (display) {
    screenPos = display->nativeWindow()->pointToScreen(mousePos);
    display->updateLastMousePos(mousePos);
  }
  else {
    screenPos = mousePos;
  }

  // Get the list of widgets to send mouse messages.
  mouse_widgets_list.clear();
  broadcastMouseMessage(screenPos,
                        mouse_widgets_list);

  // Get the widget under the mouse
  Widget* widget = nullptr;
  for (auto mouseWidget : mouse_widgets_list) {
    if (get_multiple_displays()) {
      if (display) {
        if (display != mouseWidget->display()) {
          widget = mouseWidget->display()->containedWidget()->pickFromScreenPos(screenPos);
        }
        else {
          widget = mouseWidget->pick(mousePos);
        }
      }
      else {
        widget = mouseWidget->display()->containedWidget()->pickFromScreenPos(screenPos);
      }
    }
    else {
      if (display)
        widget = mouseWidget->pick(mousePos);
      else
        widget = mouseWidget->pickFromScreenPos(screenPos);
    }
    if (widget) {
      // Get the first ancestor of the picked widget that doesn't
      // ignore mouse events.
      while (widget && widget->hasFlags(IGNORE_MOUSE))
        widget = widget->parent();
      break;
    }
  }

  // Fixup "mouse" flag
  if (widget != mouse_widget) {
    if (!widget) {
      freeMouse();
    }
    else {
      setMouse(widget);
    }
  }
}

void Manager::dispatchMessages()
{
  // Send messages in the queue (mouse/key/timer/etc. events) This
  // might change the state of widgets, etc. In case pumpQueue()
  // returns a number greater than 0, it means that we've processed
  // some messages, so we've to redraw the screen.
  if (pumpQueue() > 0 || redrawState == RedrawState::RedrawDelayed) {
    if (redrawState == RedrawState::ClosingApp) {
      // Do nothing, we don't flush nor process paint messages
    }
    // If a window has just been closed with Manager::_closeWindow()
    // after processing messages, we'll wait the next event generation
    // to process painting events (so the manager doesn't lost the
    // DIRTY flag right now).
    else if (redrawState == RedrawState::AWindowHasJustBeenClosed) {
      redrawState = RedrawState::RedrawDelayed;
    }
    else {
      if (redrawState == RedrawState::RedrawDelayed)
        redrawState = RedrawState::Normal;

      // Generate and send just kPaintMessages with the latest UI state.
      flushRedraw();
      pumpQueue();

      // Flip back-buffers to real displays.
      flipAllDisplays();
    }
  }
}

void Manager::addToGarbage(Widget* widget)
{
  ASSERT(widget);
  m_garbage.push_back(widget);
}

void Manager::enqueueMessage(Message* msg)
{
  ASSERT(msg);

  if (is_ui_thread())
    msg_queue.push_back(msg);
  else
    concurrent_msg_queue.push(msg);
}

Window* Manager::getTopWindow()
{
  return static_cast<Window*>(UI_FIRST_WIDGET(children()));
}

Window* Manager::getDesktopWindow()
{
  for (auto child : children()) {
    Window* window = static_cast<Window*>(child);
    if (window->isDesktop())
      return window;
  }
  return nullptr;
}

Window* Manager::getForegroundWindow()
{
  for (auto child : children()) {
    Window* window = static_cast<Window*>(child);
    if (window->isForeground() ||
        window->isDesktop())
      return window;
  }
  return nullptr;
}

Display* Manager::getForegroundDisplay()
{
  if (get_multiple_displays()) {
    Window* window = getForegroundWindow();
    if (window)
      return window->display();
  }
  return &m_display;
}

Widget* Manager::getFocus()
{
  return focus_widget;
}

Widget* Manager::getMouse()
{
  return mouse_widget;
}

Widget* Manager::getCapture()
{
  return capture_widget;
}

void Manager::setFocus(Widget* widget)
{
  if ((focus_widget != widget)
      && (!(widget)
          || (!(widget->hasFlags(DISABLED))
              && !(widget->hasFlags(HIDDEN))
              && !(widget->hasFlags(DECORATIVE))
              && someParentIsFocusStop(widget)))) {
    Widget* commonAncestor = findLowestCommonAncestor(focus_widget, widget);

    // Fetch the focus
    if (focus_widget && focus_widget != commonAncestor) {
      auto msg = new Message(kFocusLeaveMessage);
      msg->setRecipient(focus_widget);
      msg->setPropagateToParent(true);
      msg->setCommonAncestor(commonAncestor);
      enqueueMessage(msg);

      // Remove HAS_FOCUS from all hierarchy
      auto a = focus_widget;
      while (a && a != commonAncestor) {
        a->disableFlags(HAS_FOCUS);
        a = a->parent();
      }
    }

    // Put the focus
    focus_widget = widget;
    if (widget) {
      auto msg = new Message(kFocusEnterMessage);
      msg->setRecipient(widget);
      msg->setPropagateToParent(true);
      msg->setCommonAncestor(commonAncestor);
      enqueueMessage(msg);

      // Add HAS_FOCUS to all hierarchy
      auto a = focus_widget;
      while (a && a != commonAncestor) {
        if (a->hasFlags(FOCUS_STOP))
          a->enableFlags(HAS_FOCUS);
        a = a->parent();
      }
    }
  }
}

void Manager::setMouse(Widget* widget)
{
#ifdef REPORT_EVENTS
  TRACEARGS("Manager::setMouse ",
            (widget ? typeid(*widget).name(): "null"),
            (widget ? widget->id(): ""));
#endif

  if ((mouse_widget != widget) && (!capture_widget)) {
    Widget* commonAncestor = findLowestCommonAncestor(mouse_widget, widget);

    // Fetch the mouse
    if (mouse_widget && mouse_widget != commonAncestor) {
      auto msg = new Message(kMouseLeaveMessage);
      msg->setRecipient(mouse_widget);
      msg->setPropagateToParent(true);
      msg->setCommonAncestor(commonAncestor);
      enqueueMessage(msg);

      // Remove HAS_MOUSE from all the hierarchy
      auto a = mouse_widget;
      while (a && a != commonAncestor) {
        a->disableFlags(HAS_MOUSE);
        a = a->parent();
      }
    }

    // Put the mouse
    mouse_widget = widget;
    if (widget) {
      Display* display = mouse_widget->display();
      gfx::Point mousePos = display->nativeWindow()->pointFromScreen(get_mouse_position());

      auto msg = newMouseMessage(
        kMouseEnterMessage,
        display, nullptr,
        mousePos,
        PointerType::Unknown,
        m_mouseButton,
        kKeyUninitializedModifier);

      msg->setRecipient(widget);
      msg->setPropagateToParent(true);
      msg->setCommonAncestor(commonAncestor);
      enqueueMessage(msg);
      generateSetCursorMessage(display,
                               mousePos,
                               kKeyUninitializedModifier,
                               PointerType::Unknown);

      // Add HAS_MOUSE to all the hierarchy
      auto a = mouse_widget;
      while (a && a != commonAncestor) {
        a->enableFlags(HAS_MOUSE);
        a = a->parent();
      }
    }
  }
}

void Manager::setCapture(Widget* widget)
{
  // To set the capture, we set first the mouse_widget (because
  // mouse_widget shouldn't be != capture_widget)
  setMouse(widget);

  widget->enableFlags(HAS_CAPTURE);
  capture_widget = widget;

  Display* display = (widget ? widget->display(): &m_display);
  ASSERT(display && display->nativeWindow());
  if (display && display->nativeWindow())
    display->nativeWindow()->captureMouse();
}

// Sets the focus to the "magnetic" widget inside the window
void Manager::attractFocus(Widget* widget)
{
  // Get the magnetic widget
  Widget* magnet = findMagneticWidget(widget->window());

  // If magnetic widget exists and it doesn't have the focus
  if (magnet && !magnet->hasFocus())
    setFocus(magnet);
}

void Manager::focusFirstChild(Widget* widget)
{
  for (Widget* it=widget->window(); it; it=next_widget(it)) {
    if (does_accept_focus(it) && !(child_accept_focus(it, true))) {
      setFocus(it);
      break;
    }
  }
}

void Manager::freeFocus()
{
  setFocus(nullptr);
}

void Manager::freeMouse()
{
  setMouse(nullptr);
}

void Manager::freeCapture()
{
  if (capture_widget) {
    Display* display = capture_widget->display();

    capture_widget->disableFlags(HAS_CAPTURE);
    capture_widget = nullptr;

    ASSERT(display && display->nativeWindow());
    if (display && display->nativeWindow())
      display->nativeWindow()->releaseMouse();
  }
}

void Manager::freeWidget(Widget* widget)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  if (widget->hasFocus() || (widget == focus_widget))
    freeFocus();

  // We shouldn't free widgets that are locked, it means, widgets that
  // will be re-added soon (e.g. when the stack of windows is
  // temporarily modified).
  if (m_lockedWindow == widget)
    return;

  // Break any relationship with the GUI manager
  if (widget->hasCapture() || (widget == capture_widget))
    freeCapture();

  if (widget->hasMouse() || (widget == mouse_widget))
    freeMouse();

  auto it = std::find(mouse_widgets_list.begin(),
                      mouse_widgets_list.end(),
                      widget);
  if (it != mouse_widgets_list.end())
    mouse_widgets_list.erase(it);

  ASSERT(!Manager::widgetAssociatedToManager(widget));
}

void Manager::removeMessagesFor(Widget* widget)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  for (Message* msg : msg_queue)
    msg->removeRecipient(widget);

  for (Message* msg : used_msg_queue)
    msg->removeRecipient(widget);
}

void Manager::removeMessagesFor(Widget* widget, MessageType type)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  for (Message* msg : msg_queue)
    if (msg->type() == type)
      msg->removeRecipient(widget);

  for (Message* msg : used_msg_queue)
    if (msg->type() == type)
      msg->removeRecipient(widget);
}

void Manager::removeMessagesForTimer(Timer* timer)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  for (Message* msg : msg_queue) {
    if (msg->type() == kTimerMessage &&
        static_cast<TimerMessage*>(msg)->timer() == timer) {
      msg->removeRecipient(msg->recipient());
      static_cast<TimerMessage*>(msg)->_resetTimer();
    }
  }

  for (Message* msg : used_msg_queue) {
    if (msg->type() == kTimerMessage &&
        static_cast<TimerMessage*>(msg)->timer() == timer) {
      msg->removeRecipient(msg->recipient());
      static_cast<TimerMessage*>(msg)->_resetTimer();
    }
  }
}

void Manager::removeMessagesForDisplay(Display* display)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  for (Message* msg : msg_queue) {
    if (msg->display() == display) {
      msg->removeRecipient(msg->recipient());
      msg->setDisplay(nullptr);
    }
  }

  for (Message* msg : used_msg_queue) {
    if (msg->display() == display) {
      msg->removeRecipient(msg->recipient());
      msg->setDisplay(nullptr);
    }
  }
}

void Manager::removePaintMessagesForDisplay(Display* display)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  for (auto it=msg_queue.begin(); it != msg_queue.end(); ) {
    Message* msg = *it;
    if (msg->type() == kPaintMessage &&
        msg->display() == display) {
      delete msg;
      it = msg_queue.erase(it);
    }
    else
      ++it;
  }
}

void Manager::addMessageFilter(int message, Widget* widget)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  LockFilters lock;
  int c = message;
  if (c >= kFirstRegisteredMessage)
    c = kFirstRegisteredMessage;

  msg_filters[c].push_back(new Filter(message, widget));
}

void Manager::removeMessageFilter(int message, Widget* widget)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  LockFilters lock;
  int c = message;
  if (c >= kFirstRegisteredMessage)
    c = kFirstRegisteredMessage;

  Filters& msg_filter = msg_filters[c];
  for (Filter* filter : msg_filter) {
    if (filter->widget == widget)
      filter->widget = nullptr;
  }
}

void Manager::removeMessageFilterFor(Widget* widget)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  LockFilters lock;
  for (Filters& msg_filter : msg_filters) {
    for (Filter* filter : msg_filter) {
      if (filter->widget == widget)
        filter->widget = nullptr;
    }
  }
}

bool Manager::isFocusMovementMessage(Message* msg)
{
  if (msg->type() != kKeyDownMessage &&
      msg->type() != kKeyUpMessage)
    return false;

  switch (static_cast<KeyMessage*>(msg)->scancode()) {
    case kKeyTab:
    case kKeyLeft:
    case kKeyRight:
    case kKeyUp:
    case kKeyDown:
      return true;
  }
  return false;
}

Widget* Manager::pickFromScreenPos(const gfx::Point& screenPos) const
{
  Display* mainDisplay = display();

  if (get_multiple_displays()) {
    for (auto child : children()) {
      auto window = static_cast<Window*>(child);
      if (window->ownDisplay() ||
          window->display() != mainDisplay) {
        os::Window* nativeWindow = window->display()->nativeWindow();
        if (nativeWindow->frame().contains(screenPos))
          return window->pick(nativeWindow->pointFromScreen(screenPos));
      }
    }

    gfx::Point displayPos = display()->nativeWindow()->pointFromScreen(screenPos);
    for (auto child : children()) {
      auto window = static_cast<Window*>(child);
      if (window->display() == mainDisplay) {
        if (auto picked = window->pick(displayPos))
          return picked;
      }
    }
  }
  return Widget::pickFromScreenPos(screenPos);
}

void Manager::_closingAppWithException()
{
  redrawState = RedrawState::ClosingApp;
}

// Configures the window for begin the loop
void Manager::_openWindow(Window* window, bool center)
{
  Display* parentDisplay = (window->parentDisplay() ?
                            window->parentDisplay():
                            getForegroundDisplay());
  ASSERT(parentDisplay);

  // Opening other window in the "close app" state, ok, let's back to normal.
  if (redrawState == RedrawState::ClosingApp)
    redrawState = RedrawState::Normal;

  // Free all widgets of special states.
  if (window->isWantFocus()) {
    freeCapture();
    freeMouse();
    freeFocus();
  }

  // Relayout before inserting the window to the list of children widgets prevents
  // the manager to invalidate a window currently being laid out when
  // ui::Manager::getDefault()->invalidate() is called. This situation could happen,
  // for instance, when a script opens a dialog whose canvas.onpaint handler opens
  // another dialog, because the onpaint handler is executed during window layout.
  if (center)
    window->centerWindow(parentDisplay);
  else
    window->layout();

  // Add the window to manager.
  insertChild(0, window);

  // Broadcast the open message.
  {
    Message msg(kOpenMessage);
    window->sendMessage(&msg);
  }

  // If the window already was set a display, we don't setup it
  // (i.e. in the case of combobox popup/window the display field is
  // set to the same display where the ComboBox widget is located)
  if (!window->hasDisplaySet()) {
    // In other case, we can try to create a display/native window for
    // the UI window.
    if (get_multiple_displays()
        && window->shouldCreateNativeWindow()) {
      const int scale = parentDisplay->nativeWindow()->scale();

      os::WindowSpec spec;
      gfx::Rect frame;
      bool changeFrame;
      if (!window->lastNativeFrame().isEmpty()) {
        frame = window->lastNativeFrame();
        changeFrame = true;
      }
      else {
        gfx::Rect relativeToFrame = parentDisplay->nativeWindow()->contentRect();
        frame = window->bounds();
        frame *= scale;
        frame.offset(relativeToFrame.origin());
        changeFrame = false;
      }

      limit_with_workarea(parentDisplay, frame);

      spec.position(os::WindowSpec::Position::Frame);
      spec.frame(frame);
      spec.scale(scale);
      // Only desktop will have the real native window title bar
      // TODO in the future other windows could use the native title bar
      //      when there are no special decorators (or we could just add
      //      the possibility to create new buttons in the native window
      //      title bar)
      spec.titled(window->isDesktop());
      spec.floating(!window->isDesktop());
      spec.resizable(window->isDesktop() || window->isSizeable());
      spec.maximizable(spec.resizable());
      spec.minimizable(window->isDesktop());
      spec.borderless(!window->isDesktop());
      spec.transparent(window->isTransparent());

      if (!window->isDesktop()) {
        spec.parent(parentDisplay->nativeWindow());
      }

      os::WindowRef newNativeWindow = os::instance()->makeWindow(spec);
      ui::Display* newDisplay = new ui::Display(parentDisplay, newNativeWindow, window);

      newNativeWindow->setUserData(newDisplay);
      window->setDisplay(newDisplay, true);

      // Set native title bar text
      newNativeWindow->setTitle(window->text());

      // Activate only non-floating windows
      if (!spec.floating())
        newNativeWindow->activate();
      else
        m_display.nativeWindow()->activate();

      // Move all widgets to the os::Display origin (0,0)
      if (changeFrame) {
        window->setBounds(newNativeWindow->bounds() / scale);
      }
      else {
        window->offsetWidgets(-window->origin().x, -window->origin().y);
      }

      // Handle native hit testing. Required to be able to move/resize
      // a window with a non-mouse pointer (e.g. stylus) on Windows.
      newNativeWindow->handleHitTest = handle_native_hittest;
    }
    else {
      // Same display for desktop window or when multiple displays is
      // disabled.
      window->setDisplay(this->display(), false);
    }
  }

  // Dirty the entire window and show it
  window->setVisible(true);
  window->invalidate();

  // Attract the focus to the magnetic widget...
  // 1) get the magnetic widget
  Widget* magnet = findMagneticWidget(window);
  // 2) if magnetic widget exists and it doesn't have the focus
  if (magnet && !magnet->hasFocus())
    setFocus(magnet);
  // 3) if not, put the focus in the first child
  else if (window->isWantFocus())
    focusFirstChild(window);

  // Update mouse widget (as it can be a widget below the
  // recently opened window).
  updateMouseWidgets(ui::get_mouse_position(), nullptr);
}

void Manager::_closeWindow(Window* window, bool redraw_background)
{
  if (!hasChild(window))
    return;

  gfx::Region reg1;
  if (!window->ownDisplay()) {
    if (redraw_background)
      window->getRegion(reg1);
  }

  // Close all windows to this desktop
  if (window->isDesktop()) {
    while (!children().empty()) {
      Window* child = static_cast<Window*>(children().front());
      if (child == window)
        break;
      else {
        gfx::Region reg2;
        window->getRegion(reg2);
        reg1 |= reg2;

        _closeWindow(child, false);
      }
    }
  }

  // Free all widgets of special states.
  if (capture_widget && capture_widget->window() == window)
    freeCapture();

  if (mouse_widget && mouse_widget->window() == window)
    freeMouse();

  if (focus_widget && focus_widget->window() == window)
    freeFocus();

  // Hide window.
  window->setVisible(false);

  // Close message.
  {
    Message msg(kCloseMessage);
    window->sendMessage(&msg);
  }

  // Destroy native window associated with this window's display if needed
  Display* windowDisplay = window->display();
  Display* parentDisplay;
  if (// The display can be nullptr if the window was not opened or
      // was closed before.
      window->ownDisplay()) {
    parentDisplay = (windowDisplay ? windowDisplay->parentDisplay(): nullptr);
    ASSERT(parentDisplay);
    ASSERT(windowDisplay);
    ASSERT(windowDisplay != this->display());

    // We are receiving several crashes from Windows users where
    // parentDisplay != nullptr and parentDisplay->nativeWindow() ==
    // nullptr, so we have to do some extra checks in these places
    // (anyway this might produce some crashes in other places)
    os::Window* nativeWindow = (windowDisplay ? windowDisplay->nativeWindow(): nullptr);
    os::Window* parentNativeWindow = (parentDisplay ? parentDisplay->nativeWindow(): nullptr);
    ASSERT(nativeWindow);
    ASSERT(parentNativeWindow);

    // Just as we've set the origin of the window bounds to (0, 0)
    // when we created the native window, we have to restore the
    // ui::Window bounds' origin now that we are going to remove/close
    // the native window.
    if (parentNativeWindow && nativeWindow) {
      const int scale = parentNativeWindow->scale();
      const gfx::Point parentOrigin = parentNativeWindow->contentRect().origin();
      const gfx::Point origin = nativeWindow->contentRect().origin();
      const gfx::Rect newBounds((origin - parentOrigin) / scale,
                                window->bounds().size());
      window->setBounds(newBounds);
    }

    // Set the native window user data to nullptr so any other queued
    // native message is not processed.
    window->setDisplay(nullptr, false);
    if (nativeWindow)
      nativeWindow->setUserData<void*>(nullptr);

    // Remove all messages for this display.
    removeMessagesForDisplay(windowDisplay);

    // Remove the mouse cursor from the display that we are going to
    // delete.
    _internal_set_mouse_display(parentDisplay);

    // Remove the display that we're going to delete (windowDisplay)
    // as parent of any other existent display.
    for (auto otherChild : children()) {
      if (auto otherWindow = static_cast<Window*>(otherChild)) {
        if (otherWindow != window &&
            otherWindow->display() &&
            otherWindow->display()->parentDisplay() == windowDisplay) {
          otherWindow->display()->_setParentDisplay(parentDisplay);
        }
      }
    }

    // The ui::Display should destroy the os::Window
    delete windowDisplay;

    // Activate main windows
    if (parentNativeWindow)
      parentNativeWindow->activate();
  }
  else {
    parentDisplay = windowDisplay;
    window->setDisplay(nullptr, false);
  }

  // Update manager list stuff.
  removeChild(window);

  // Redraw background.
  parentDisplay->containedWidget()->invalidateRegion(reg1);

  // Update mouse widget (as it can be a widget below the
  // recently closed window).
  updateMouseWidgets(ui::get_mouse_position(), nullptr);

  if (redrawState != RedrawState::ClosingApp) {
    if (children().empty()) {
      // All windows were closed...
      redrawState = RedrawState::ClosingApp;
    }
    else {
      redrawState = RedrawState::AWindowHasJustBeenClosed;
    }
  }
}

void Manager::_runModalWindow(Window* window)
{
  MessageLoop loop(manager());
  while (!window->hasFlags(HIDDEN))
    loop.pumpMessages();
}

void Manager::_updateMouseWidgets()
{
  // Update mouse widget.
  updateMouseWidgets(ui::get_mouse_position(), nullptr);
}

bool Manager::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kPaintMessage:
      // Draw nothing (the manager should be invisible). On Windows,
      // after closing the main window, the manager will not refresh
      // the os::Display content, so we'll avoid a gray background
      // (the last main window content is kept until the Display is
      // finally closed.)
      return true;

    case kCloseDisplayMessage: {
      if (msg->display() != &m_display) {
        if (Window* window = dynamic_cast<Window*>(msg->display()->containedWidget())) {
          window->closeWindow(this);
        }
      }
      break;
    }

    case kResizeDisplayMessage:
      onNewDisplayConfiguration(msg->display());
      break;

    case kKeyDownMessage:
    case kKeyUpMessage: {
      KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
      keymsg->setPropagateToChildren(true);
      keymsg->setPropagateToParent(false);

      // Continue sending the message to the children of all windows
      // (until a desktop or foreground window).
      Window* win = nullptr;
      for (auto manchild : children()) {
        win = static_cast<Window*>(manchild);

        // Send to the window.
        for (auto winchild : win->children())
          if (winchild->sendMessage(msg))
            return true;

        if (win->isForeground() ||
            win->isDesktop())
          break;
      }

      // Check the focus movement for foreground (non-desktop) windows.
      if (win && win->isForeground()) {
        if (msg->type() == kKeyDownMessage)
          processFocusMovementMessage(msg);
        return true;
      }
      else
        return false;
    }

  }

  return Widget::onProcessMessage(msg);
}

void Manager::onResize(ResizeEvent& ev)
{
  gfx::Rect old_pos = bounds();
  gfx::Rect new_pos = ev.bounds();
  setBoundsQuietly(new_pos);

  // The whole manager area is invalid now.
  m_display.setInvalidRegion(gfx::Region(new_pos));

  const int dx = new_pos.x - old_pos.x;
  const int dy = new_pos.y - old_pos.y;
  const int dw = new_pos.w - old_pos.w;
  const int dh = new_pos.h - old_pos.h;

  for (auto child : children()) {
    Window* window = static_cast<Window*>(child);
    if (window->ownDisplay())
      continue;

    if (window->isDesktop()) {
      window->setBounds(new_pos);
      break;
    }

    gfx::Rect bounds = window->bounds();
    const int cx = bounds.x+bounds.w/2;
    const int cy = bounds.y+bounds.h/2;

    if (auto_window_adjustment) {
      if (cx > old_pos.x+old_pos.w*3/5) {
        bounds.x += dw;
      }
      else if (cx > old_pos.x+old_pos.w*2/5) {
        bounds.x += dw / 2;
      }

      if (cy > old_pos.y+old_pos.h*3/5) {
        bounds.y += dh;
      }
      else if (cy > old_pos.y+old_pos.h*2/5) {
        bounds.y += dh / 2;
      }

      bounds.offset(dx, dy);
    }
    else {
      if (bounds.x2() > new_pos.x2()) {
        bounds.x = new_pos.x2() - bounds.w;
      }
      if (bounds.y2() > new_pos.y2()) {
        bounds.y = new_pos.y2() - bounds.h;
      }
    }
    window->setBounds(bounds);
  }
}

void Manager::onBroadcastMouseMessage(const gfx::Point& screenPos,
                                      WidgetsList& targets)
{
  // Ask to the first window in the "children" list to know how to
  // propagate mouse messages.
  Widget* widget = UI_FIRST_WIDGET(children());
  if (widget)
    widget->broadcastMouseMessage(screenPos, targets);
}

void Manager::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  // Remap the windows
  const int oldUIScale = ui::details::old_guiscale();
  const int newUIScale = ui::guiscale();
  for (auto widget : children()) {
    if (widget->type() == kWindowWidget) {
      auto window = static_cast<Window*>(widget);
      if (window->isDesktop()) {
        window->layout();
      }
      else {
        gfx::Size displaySize = m_display.size();
        gfx::Rect bounds = window->bounds();
        bounds *= newUIScale;
        bounds /= oldUIScale;
        bounds.x = std::clamp(bounds.x, 0, displaySize.w - bounds.w);
        bounds.y = std::clamp(bounds.y, 0, displaySize.h - bounds.h);
        window->setBounds(bounds);
      }
    }
  }
}

LayoutIO* Manager::onGetLayoutIO()
{
  return nullptr;
}

void Manager::onNewDisplayConfiguration(Display* display)
{
  ASSERT(display);
  Widget* container = display->containedWidget();

  gfx::Size displaySize = display->size();
  if ((bounds().w != displaySize.w ||
       bounds().h != displaySize.h)) {
    container->setBounds(gfx::Rect(displaySize));
  }

  // The native window can be nullptr when running tests.
  if (!display->nativeWindow())
    return;

  _internal_set_mouse_display(display);
  container->invalidate();
  container->flushRedraw();
}

void Manager::onSizeHint(SizeHintEvent& ev)
{
  int w = 0, h = 0;

  if (!parent()) {        // hasn' parent?
    w = bounds().w;
    h = bounds().h;
  }
  else {
    gfx::Rect pos = parent()->childrenBounds();

    for (auto child : children()) {
      gfx::Rect cpos = child->bounds();
      pos = pos.createUnion(cpos);
    }

    w = pos.w;
    h = pos.h;
  }

  ev.setSizeHint(gfx::Size(w, h));
}

int Manager::pumpQueue()
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

#ifdef LIMIT_DISPATCH_TIME
  base::tick_t t = base::current_tick();
#endif

  int count = 0;                // Number of processed messages
  while (!msg_queue.empty()) {
#ifdef LIMIT_DISPATCH_TIME
    if (base::current_tick()-t > 250)
      break;
#endif

    // The message to process
    auto it = msg_queue.begin();
    Message* msg = *it;
    ASSERT(msg);

    // Move the message from msg_queue to used_msg_queue
    msg_queue.erase(it);
    auto eraseIt = used_msg_queue.insert(used_msg_queue.end(), msg);

    // Call Timer::tick() if this is a tick message.
    if (msg->type() == kTimerMessage) {
      // The timer can be nullptr if it was removed with removeMessagesForTimer()
      if (auto timer = static_cast<TimerMessage*>(msg)->timer())
        timer->tick();
    }

    bool done = false;

    // Send this message to filters
    {
      Filters& msg_filter = msg_filters[std::min(msg->type(), kFirstRegisteredMessage)];
      if (!msg_filter.empty()) {
        LockFilters lock;
        for (Filter* filter : msg_filter) {
          // The widget can be nullptr in case that the filter was
          // "pre-removed" (it'll finally erased from the
          // msg_filter list from ~LockFilters()).
          if (filter->widget != nullptr &&
              msg->type() == filter->message) {
            msg->setFromFilter(true);
            done = sendMessageToWidget(msg, filter->widget);
            msg->setFromFilter(false);

            if (done)
              break;
          }
        }
      }
    }

    if (!done) {
      // Then send the message to its recipient
      if (Widget* widget = msg->recipient())
        done = sendMessageToWidget(msg, widget);
    }

    // Remove the message from the used_msg_queue
    used_msg_queue.erase(eraseIt);

    // Destroy the message
    delete msg;
    ++count;
  }

  return count;
}

bool Manager::sendMessageToWidget(Message* msg, Widget* widget)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == std::this_thread::get_id());
#endif

  if (!widget)
    return false;

#ifdef REPORT_EVENTS
  {
    static const char* msg_name[] = {
      "kOpenMessage",
      "kCloseMessage",
      "kCloseDisplayMessage",
      "kResizeDisplayMessage",
      "kPaintMessage",
      "kTimerMessage",
      "kDropFilesMessage",
      "kWinMoveMessage",

      "kKeyDownMessage",
      "kKeyUpMessage",
      "kFocusEnterMessage",
      "kFocusLeaveMessage",

      "kMouseDownMessage",
      "kMouseUpMessage",
      "kDoubleClickMessage",
      "kMouseEnterMessage",
      "kMouseLeaveMessage",
      "kMouseMoveMessage",
      "kSetCursorMessage",
      "kMouseWheelMessage",
      "kTouchMagnifyMessage",
    };
    static_assert(kOpenMessage == 0 &&
                  kTouchMagnifyMessage == sizeof(msg_name)/sizeof(const char*)-1,
                  "MessageType enum has changed");
    const char* string =
      (msg->type() >= 0 &&
       msg->type() < sizeof(msg_name)/sizeof(const char*)) ?
      msg_name[msg->type()]: "Unknown";

    TRACEARGS("Event", msg->type(), "(", string, ")",
              "for", ((void*)widget),
              typeid(*widget).name(),
              widget->id().empty());
  }
#endif

  bool used = false;

  // We need to configure the clip region for paint messages
  // before we call Widget::sendMessage().
  if (msg->type() == kPaintMessage) {
    if (widget->hasFlags(HIDDEN))
      return false;

    // Ignore all paint messages when we are closing the app
    if (redrawState == RedrawState::ClosingApp)
      return false;

    PaintMessage* paintMsg = static_cast<PaintMessage*>(msg);
    Display* display = paintMsg->display();

    // TODO use paintMsg->display() here
    // Restore overlays in the region that we're going to paint.
    OverlayManager::instance()->restoreOverlappedAreas(paintMsg->rect());

    os::SurfaceRef surface(base::AddRef(display->surface()));
    surface->saveClip();

    if (surface->clipRect(paintMsg->rect())) {
#ifdef REPORT_EVENTS
      TRACEARGS(" - clipRect ", paintMsg->rect());
#endif

#ifdef DEBUG_PAINT_EVENTS
      {
        os::SurfaceLock lock(surface.get());
        os::Paint p;
        p.color(gfx::rgba(0, 0, 255));
        p.style(os::Paint::Fill);
        surface->drawRect(paintMsg->rect(), p);

        display->nativeWindow()
          ->invalidateRegion(gfx::Region(paintMsg->rect()));

#ifdef _WIN32 // TODO add a display->nativeWindow()->updateNow() method ??
        HWND hwnd = (HWND)display->nativeWindow()->nativeHandle();
        UpdateWindow(hwnd);
#else
        base::this_thread::sleep_for(0.002);
#endif
      }
#endif

      // Call the message handler
      used = widget->sendMessage(msg);
    }

    // Restore clip region for paint messages.
    surface->restoreClip();

    // As this kPaintMessage's rectangle was updated, we can
    // remove it from "m_invalidRegion".
    display->subtractInvalidRegion(gfx::Region(paintMsg->rect()));
  }
  else {
    // Call the message handler
    used = widget->sendMessage(msg);
  }

  return used;
}

// It's like Widget::onInvalidateRegion() but optimized for the
// Manager (as we know that all children in a Manager will be windows,
// we can use this knowledge to avoid some calculations).
//
// TODO similar to Window::onInvalidateRegion
void Manager::onInvalidateRegion(const gfx::Region& region)
{
  if (!isVisible() || region.contains(bounds()) == gfx::Region::Out)
    return;

  // Intersect only with manager bounds, we don't need to use
  // getDrawableRegion() because each window will be processed in the
  // following for() loop (and it's highly probable that a desktop
  // Window will use the whole manager portion anyway).
  gfx::Region reg1;
  reg1.createIntersection(region, gfx::Region(bounds()));

  // Redraw windows from top to background.
  bool withDesktop = false;
  for (auto child : children()) {
    ASSERT(dynamic_cast<Window*>(child));
    ASSERT(child->type() == kWindowWidget);
    Window* window = static_cast<Window*>(child);

    // Invalidating the manager only works for the main display, to
    // invalidate windows you have to invalidate them.
    if (window->ownDisplay())
      continue;

    // Invalidate regions of this window
    window->invalidateRegion(reg1);

    // There is desktop?
    if (window->isDesktop()) {
      withDesktop = true;
      break; // Work done
    }

    // Clip this window area for the next window.
    gfx::Region reg2;
    window->getRegion(reg2);
    reg1.createSubtraction(reg1, reg2);
  }

  // Invalidate areas outside windows (only when there are not a
  // desktop window).
  if (!withDesktop) {
    // TODO we should be able to modify m_updateRegion directly here,
    // so we avoid the getDrawableRegion() call from
    // Widget::onInvalidateRegion().
    Widget::onInvalidateRegion(reg1);
  }
}

LayoutIO* Manager::getLayoutIO()
{
  return onGetLayoutIO();
}

void Manager::collectGarbage()
{
  if (m_garbage.empty())
    return;

  GARBAGE_TRACE("Manager::collectGarbage() #objects=%d\n", int(m_garbage.size()));

  for (auto widget : m_garbage) {
    GARBAGE_TRACE(" -> deleting %s %s ---\n",
                  typeid(*widget).name(),
                  widget->id().c_str());
    delete widget;
  }
  m_garbage.clear();
}

/**********************************************************************
                        Internal routines
 **********************************************************************/

// static
Widget* Manager::findLowestCommonAncestor(Widget* a, Widget* b)
{
  if (!a || !b)
    return nullptr;

  Widget* u = a;
  Widget* v = b;
  int aDepth = 0;
  int bDepth = 0;
  while (u) {
    ++aDepth;
    u = u->parent();
  }
  while (v) {
    ++bDepth;
    v = v->parent();
  }

  while (aDepth > bDepth) {
    --aDepth;
    a = a->parent();
  }
  while (bDepth > aDepth) {
    --bDepth;
    b = b->parent();
  }

  while (a && b) {
    if (a == b)
      break;
    a = a->parent();
    b = b->parent();
  }
  return a;
}

// static
bool Manager::someParentIsFocusStop(Widget* widget)
{
  if (widget->isFocusStop())
    return true;

  if (widget->parent())
    return someParentIsFocusStop(widget->parent());
  else
    return false;
}

// static
Widget* Manager::findMagneticWidget(Widget* widget)
{
  Widget* found;

  for (auto child : widget->children()) {
    found = findMagneticWidget(child);
    if (found)
      return found;
  }

  if (widget->isFocusMagnet())
    return widget;
  else
    return nullptr;
}

// static
Message* Manager::newMouseMessage(
  MessageType type,
  Display* display,
  Widget* widget,
  const gfx::Point& mousePos,
  PointerType pointerType,
  MouseButton button,
  KeyModifiers modifiers,
  const gfx::Point& wheelDelta,
  bool preciseWheel,
  float pressure)
{
#ifdef __APPLE__
  // Convert Ctrl+left click -> right-click
  if (widget &&
      widget->isVisible() &&
      widget->isEnabled() &&
      widget->hasFlags(CTRL_RIGHT_CLICK) &&
      (modifiers & kKeyCtrlModifier) &&
      (button == kButtonLeft)) {
    modifiers = KeyModifiers(int(modifiers) & ~int(kKeyCtrlModifier));
    button = kButtonRight;
  }
#endif

  Message* msg = new MouseMessage(
    type, pointerType, button, modifiers, mousePos,
    wheelDelta, preciseWheel, pressure);

  if (display)
    msg->setDisplay(display);

  if (widget)
    msg->setRecipient(widget);

  return msg;
}

// static
void Manager::broadcastKeyMsg(Message* msg)
{
  // Send the message to the widget with capture
  if (capture_widget) {
    msg->setRecipient(capture_widget);
  }
  // Send the msg to the focused widget
  else if (focus_widget) {
    msg->setRecipient(focus_widget);
  }
  // Finally, send the message to the manager, it'll know what to do
  else {
    msg->setRecipient(this);
  }
}

/***********************************************************************
                            Focus Movement
 ***********************************************************************/

// TODO rewrite this function, it is based in an old code from the
//      Allegro library GUI code

bool Manager::processFocusMovementMessage(Message* msg)
{
  int (*cmp)(Widget*, int, int) = nullptr;
  Widget* focus = nullptr;
  Widget* it;
  bool ret = false;
  Window* window = nullptr;
  int c, count;

  // Who have the focus
  if (focus_widget) {
    window = focus_widget->window();
  }
  else if (!this->children().empty()) {
    window = this->getTopWindow();
  }

  if (!window)
    return false;

  // How many children want the focus in this window?
  count = count_widgets_accept_focus(window);

  // One at least
  if (count > 0) {
    std::vector<Widget*> list(count);

    c = 0;

    // Create a list of possible candidates to receive the focus
    for (it=focus_widget; it; it=next_widget(it)) {
      if (does_accept_focus(it) && !(child_accept_focus(it, true)))
        list[c++] = it;
    }
    for (it=window; it != focus_widget; it=next_widget(it)) {
      if (does_accept_focus(it) && !(child_accept_focus(it, true)))
        list[c++] = it;
    }

    // Depending on the pressed key...
    switch (static_cast<KeyMessage*>(msg)->scancode()) {

      case kKeyTab:
        // Reverse tab
        if ((msg->modifiers() & (kKeyShiftModifier | kKeyCtrlModifier | kKeyAltModifier)) != 0) {
          focus = list[count-1];
        }
        // Normal tab
        else if (count > 1) {
          focus = list[1];
        }
        ret = true;
        break;

      // Arrow keys
      case kKeyLeft:  if (!cmp) cmp = cmp_left;  [[fallthrough]];
      case kKeyRight: if (!cmp) cmp = cmp_right; [[fallthrough]];
      case kKeyUp:    if (!cmp) cmp = cmp_up;    [[fallthrough]];
      case kKeyDown:  if (!cmp) cmp = cmp_down;
        // More than one widget
        if (count > 1) {
          // Position where the focus come
          gfx::Point pt = (focus_widget ? focus_widget->bounds().center():
                                          window->bounds().center());

          c = (focus_widget ? 1: 0);

          // Rearrange the list
          for (int i=c; i<count-1; ++i) {
            for (int j=i+1; j<count; ++j) {
              // Sort the list in ascending order
              if ((*cmp)(list[i], pt.x, pt.y) > (*cmp)(list[j], pt.x, pt.y))
                std::swap(list[i], list[j]);
            }
          }

#ifdef REPORT_FOCUS_MOVEMENT
          // Print list of widgets
          for (int i=c; i<count-1; ++i) {
            TRACE("list[%d] = %d (%s)\n",
                  i, (*cmp)(list[i], pt.x, pt.y),
                  typeid(*list[i]).name());
          }
#endif

          // Check if the new widget to put the focus is not in the wrong way.
          if ((*cmp)(list[c], pt.x, pt.y) < std::numeric_limits<int>::max())
            focus = list[c];
        }
        // If only there are one widget, put the focus in this
        else
          focus = list[0];

        ret = true;
        break;
    }

    if ((focus) && (focus != focus_widget))
      setFocus(focus);
  }

  return ret;
}

static int count_widgets_accept_focus(Widget* widget)
{
  int count = 0;

  for (auto child : widget->children())
    count += count_widgets_accept_focus(child);

  if ((count == 0) && (does_accept_focus(widget)))
    count++;

  return count;
}

static bool child_accept_focus(Widget* widget, bool first)
{
  for (auto child : widget->children())
    if (child_accept_focus(child, false))
      return true;

  return (first ? false: does_accept_focus(widget));
}

static Widget* next_widget(Widget* widget)
{
  if (!widget->children().empty())
    return UI_FIRST_WIDGET(widget->children());

  while (widget->parent() &&
         widget->parent()->type() != kManagerWidget) {
    WidgetsList::const_iterator begin = widget->parent()->children().begin();
    WidgetsList::const_iterator end = widget->parent()->children().end();
    WidgetsList::const_iterator it = std::find(begin, end, widget);

    ASSERT(it != end);

    if ((it+1) != end)
      return *(it+1);
    else
      widget = widget->parent();
  }

  return nullptr;
}

static int cmp_left(Widget* widget, int x, int y)
{
  int z = x - (widget->bounds().x+widget->bounds().w/2);
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().y+widget->bounds().h/2) - y) * 8;
}

static int cmp_right(Widget* widget, int x, int y)
{
  int z = (widget->bounds().x+widget->bounds().w/2) - x;
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().y+widget->bounds().h/2) - y) * 8;
}

static int cmp_up(Widget* widget, int x, int y)
{
  int z = y - (widget->bounds().y+widget->bounds().h/2);
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().x+widget->bounds().w/2) - x) * 8;
}

static int cmp_down(Widget* widget, int x, int y)
{
  int z = (widget->bounds().y+widget->bounds().h/2) - y;
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().x+widget->bounds().w/2) - x) * 8;
}

} // namespace ui
