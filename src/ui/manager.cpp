// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
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
#include "base/time.h"
#include "os/display.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#if defined(DEBUG_PAINT_EVENTS) || defined(DEBUG_UI_THREADS)
#include "base/thread.h"
#endif

#ifdef REPORT_EVENTS
#include <iostream>
#endif

#include <algorithm>
#include <limits>
#include <list>
#include <memory>
#include <utility>
#include <vector>

namespace ui {

namespace {

// The redraw state is used to avoid drawing the manager when a window
// has been just closed by the user, so we delay the redrawing (the
// kPaintMessages generation) for the next generateMessages() round.
enum class RedrawState {
   Normal,
   AWindowHasJustBeenClosed,
   RedrawDelayed,
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

Manager* Manager::m_defaultManager = NULL;
gfx::Region Manager::m_dirtyRegion;

#ifdef DEBUG_UI_THREADS
static base::thread::native_handle_type manager_thread = 0;
#endif

static WidgetsList mouse_widgets_list; // List of widgets to send mouse events
static Messages msg_queue;             // Messages queue
static Messages used_msg_queue;        // Messages queue
static base::concurrent_queue<Message*> concurrent_msg_queue;
static Filters msg_filters[NFILTERS]; // Filters for every enqueued message
static int filter_locks = 0;

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

Manager::Manager()
  : Widget(kManagerWidget)
  , m_display(NULL)
  , m_eventQueue(NULL)
  , m_lockedWindow(NULL)
  , m_mouseButtons(kButtonNone)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(!manager_thread);
  manager_thread = base::this_thread::native_handle();
#endif

  if (!m_defaultManager) {
    // Empty lists
    ASSERT(msg_queue.empty());
    mouse_widgets_list.clear();

    // Reset variables
    focus_widget = NULL;
    mouse_widget = NULL;
    capture_widget = NULL;
  }

  setBounds(gfx::Rect(0, 0, ui::display_w(), ui::display_h()));
  setVisible(true);

  m_dirtyRegion = bounds();

  // Default manager is the first one (and is always visible).
  if (!m_defaultManager)
    m_defaultManager = this;
}

Manager::~Manager()
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == base::this_thread::native_handle());
#endif

  // There are some messages in queue? Dispatch everything.
  dispatchMessages();
  collectGarbage();

  // Finish the main manager.
  if (m_defaultManager == this) {
    // No more cursor
    set_mouse_cursor(kNoCursor);

    // Destroy timers
    ASSERT(!Timer::haveTimers());

    // Destroy filters
#ifdef _DEBUG
    for (Filters& msg_filter : msg_filters)
      ASSERT(msg_filter.empty());
#endif

    // No more default manager
    m_defaultManager = nullptr;

    // Shutdown system
    ASSERT(msg_queue.empty());
    mouse_widgets_list.clear();
  }
}

void Manager::setDisplay(os::Display* display)
{
  base::ScopedValue<bool> lock(
    auto_window_adjustment, false,
    auto_window_adjustment);

  m_display = display;
  m_eventQueue = os::instance()->eventQueue();

  onNewDisplayConfiguration();
}

void Manager::run()
{
  MessageLoop loop(this);

  if (first_time) {
    first_time = false;

    Manager::getDefault()->invalidate();
    set_mouse_cursor(kArrowCursor);
  }

  while (!children().empty())
    loop.pumpMessages();
}

void Manager::flipDisplay()
{
  if (!m_display)
    return;

  OverlayManager* overlays = OverlayManager::instance();

  update_cursor_overlay();

  // Draw overlays.
  overlays->drawOverlays();

  // Flip dirty region.
  {
    m_dirtyRegion.createIntersection(
      m_dirtyRegion,
      gfx::Region(gfx::Rect(0, 0, ui::display_w(), ui::display_h())));

    for (const auto& rc : m_dirtyRegion)
      m_display->flip(rc);

    m_dirtyRegion.clear();
  }
}

bool Manager::generateMessages()
{
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

void Manager::generateSetCursorMessage(const gfx::Point& mousePos,
                                       KeyModifiers modifiers,
                                       PointerType pointerType)
{
  if (get_mouse_cursor() == kOutsideDisplay)
    return;

  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  if (dst)
    enqueueMessage(
      newMouseMessage(
        kSetCursorMessage, dst,
        mousePos,
        pointerType,
        _internal_get_mouse_buttons(),
        modifiers));
  else
    set_mouse_cursor(kArrowCursor);
}

static MouseButtons mouse_buttons_from_os_to_ui(const os::Event& sheEvent)
{
  switch (sheEvent.button()) {
    case os::Event::LeftButton:   return kButtonLeft; break;
    case os::Event::RightButton:  return kButtonRight; break;
    case os::Event::MiddleButton: return kButtonMiddle; break;
    case os::Event::X1Button:     return kButtonX1; break;
    case os::Event::X2Button:     return kButtonX2; break;
    default: return kButtonNone;
  }
}

void Manager::generateMessagesFromOSEvents()
{
  os::Event lastMouseMoveEvent;

  // Events from "she" layer.
  os::Event sheEvent;
  for (;;) {
    // TODO Add timers to laf::os library so we can wait for then in
    //      the OS message loop.
    bool canWait = (msg_queue.empty() &&
                    redrawState == RedrawState::Normal &&
                    !Timer::haveRunningTimers());

    if (canWait && used_msg_queue.empty())
      collectGarbage();
#if _DEBUG
    else if (!m_garbage.empty()) {
      GARBAGE_TRACE("collectGarbage() wasn't called #objects=%d"
                    " (msg_queue=%d used_msg_queue=%d redrawState=%d runningTimers=%d)\n",
                    int(m_garbage.size()),
                    msg_queue.size(),
                    used_msg_queue.size(),
                    int(redrawState),
                    Timer::haveRunningTimers());
    }
#endif

    m_eventQueue->getEvent(sheEvent, canWait);
    if (sheEvent.type() == os::Event::None)
      break;

    switch (sheEvent.type()) {

      case os::Event::CloseDisplay: {
        Message* msg = new Message(kCloseDisplayMessage);
        msg->setRecipient(this);
        msg->setPropagateToChildren(true);
        enqueueMessage(msg);
        break;
      }

      case os::Event::ResizeDisplay: {
        Message* msg = new Message(kResizeDisplayMessage);
        msg->setRecipient(this);
        msg->setPropagateToChildren(true);
        enqueueMessage(msg);
        break;
      }

      case os::Event::DropFiles: {
        Message* msg = new DropFilesMessage(sheEvent.files());
        msg->setRecipient(this);
        enqueueMessage(msg);
        break;
      }

      case os::Event::KeyDown:
      case os::Event::KeyUp: {
        Message* msg = new KeyMessage(
          (sheEvent.type() == os::Event::KeyDown ?
             kKeyDownMessage:
             kKeyUpMessage),
          sheEvent.scancode(),
          sheEvent.modifiers(),
          sheEvent.unicodeChar(),
          sheEvent.repeat());

        if (sheEvent.isDeadKey())
          static_cast<KeyMessage*>(msg)->setDeadKey(true);

        broadcastKeyMsg(msg);
        enqueueMessage(msg);
        break;
      }

      case os::Event::MouseEnter: {
        _internal_set_mouse_position(sheEvent.position());
        set_mouse_cursor(kArrowCursor);
        lastMouseMoveEvent = sheEvent;
        break;
      }

      case os::Event::MouseLeave: {
        set_mouse_cursor(kOutsideDisplay);
        setMouse(NULL);

        _internal_no_mouse_position();

        // To avoid calling kSetCursorMessage when the mouse leaves
        // the window.
        lastMouseMoveEvent = os::Event();
        break;
      }

      case os::Event::MouseMove: {
        _internal_set_mouse_position(sheEvent.position());
        handleMouseMove(
          sheEvent.position(),
          m_mouseButtons,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        lastMouseMoveEvent = sheEvent;
        break;
      }

      case os::Event::MouseDown: {
        MouseButtons pressedButton = mouse_buttons_from_os_to_ui(sheEvent);
        m_mouseButtons = (MouseButtons)((int)m_mouseButtons | (int)pressedButton);
        _internal_set_mouse_buttons(m_mouseButtons);

        handleMouseDown(
          sheEvent.position(),
          pressedButton,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        break;
      }

      case os::Event::MouseUp: {
        MouseButtons releasedButton = mouse_buttons_from_os_to_ui(sheEvent);
        m_mouseButtons = (MouseButtons)((int)m_mouseButtons & ~(int)releasedButton);
        _internal_set_mouse_buttons(m_mouseButtons);

        handleMouseUp(
          sheEvent.position(),
          releasedButton,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        break;
      }

      case os::Event::MouseDoubleClick: {
        MouseButtons clickedButton = mouse_buttons_from_os_to_ui(sheEvent);
        handleMouseDoubleClick(
          sheEvent.position(),
          clickedButton,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        break;
      }

      case os::Event::MouseWheel: {
        handleMouseWheel(sheEvent.position(), m_mouseButtons,
                         sheEvent.modifiers(),
                         sheEvent.pointerType(),
                         sheEvent.wheelDelta(),
                         sheEvent.preciseWheel());
        break;
      }

      case os::Event::TouchMagnify: {
        _internal_set_mouse_position(sheEvent.position());

        handleTouchMagnify(sheEvent.position(),
                           sheEvent.modifiers(),
                           sheEvent.magnification());
        break;
      }

    }
  }

  // Generate just one kSetCursorMessage for the last mouse position
  if (lastMouseMoveEvent.type() != os::Event::None) {
    sheEvent = lastMouseMoveEvent;
    generateSetCursorMessage(sheEvent.position(),
                             sheEvent.modifiers(),
                             sheEvent.pointerType());
  }
}

void Manager::handleMouseMove(const gfx::Point& mousePos,
                              MouseButtons mouseButtons,
                              KeyModifiers modifiers,
                              PointerType pointerType)
{
  // Get the list of widgets to send mouse messages.
  mouse_widgets_list.clear();
  broadcastMouseMessage(mouse_widgets_list);

  // Get the widget under the mouse
  Widget* widget = nullptr;
  for (auto mouseWidget : mouse_widgets_list) {
    widget = mouseWidget->pick(mousePos);
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
    if (!widget)
      freeMouse();
    else
      setMouse(widget);
  }

  // Send the mouse movement message
  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  enqueueMessage(
    newMouseMessage(
      kMouseMoveMessage, dst,
      mousePos,
      pointerType,
      mouseButtons,
      modifiers));
}

void Manager::handleMouseDown(const gfx::Point& mousePos,
                              MouseButtons mouseButtons,
                              KeyModifiers modifiers,
                              PointerType pointerType)
{
  handleWindowZOrder();

  enqueueMessage(
    newMouseMessage(
      kMouseDownMessage,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos,
      pointerType,
      mouseButtons,
      modifiers));
}

void Manager::handleMouseUp(const gfx::Point& mousePos,
                            MouseButtons mouseButtons,
                            KeyModifiers modifiers,
                            PointerType pointerType)
{
  enqueueMessage(
    newMouseMessage(
      kMouseUpMessage,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos,
      pointerType,
      mouseButtons,
      modifiers));
}

void Manager::handleMouseDoubleClick(const gfx::Point& mousePos,
                                     MouseButtons mouseButtons,
                                     KeyModifiers modifiers,
                                     PointerType pointerType)
{
  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  if (dst) {
    enqueueMessage(
      newMouseMessage(
        kDoubleClickMessage,
        dst, mousePos, pointerType,
        mouseButtons, modifiers));
  }
}

void Manager::handleMouseWheel(const gfx::Point& mousePos,
                               MouseButtons mouseButtons,
                               KeyModifiers modifiers,
                               PointerType pointerType,
                               const gfx::Point& wheelDelta,
                               bool preciseWheel)
{
  enqueueMessage(newMouseMessage(
      kMouseWheelMessage,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos, pointerType, mouseButtons, modifiers,
      wheelDelta, preciseWheel));
}

void Manager::handleTouchMagnify(const gfx::Point& mousePos,
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

    msg->setRecipient(widget);

    enqueueMessage(msg);
  }
}

// Handles Z order: Send the window to top (only when you click in a
// window that aren't the desktop).
void Manager::handleWindowZOrder()
{
  if (capture_widget || !mouse_widget)
    return;

  // The clicked window
  Window* window = mouse_widget->window();
  Manager* win_manager = (window ? window->manager(): NULL);

  if ((window) &&
    // We cannot change Z-order of desktop windows
    (!window->isDesktop()) &&
    // We cannot change Z order of foreground windows because a
    // foreground window can launch other background windows
    // which should be kept on top of the foreground one.
    (!window->isForeground()) &&
    // If the window is not already the top window of the manager.
    (window != win_manager->getTopWindow())) {
    base::ScopedValue<Widget*> scoped(m_lockedWindow, window, NULL);

    // Put it in the top of the list
    win_manager->removeChild(window);

    if (window->isOnTop())
      win_manager->insertChild(0, window);
    else {
      int pos = (int)win_manager->children().size();
      UI_FOREACH_WIDGET_BACKWARD(win_manager->children(), it) {
        if (static_cast<Window*>(*it)->isOnTop())
          break;

        --pos;
      }
      win_manager->insertChild(pos, window);
    }

    window->invalidate();
  }

  // Put the focus
  setFocus(mouse_widget);
}

void Manager::dispatchMessages()
{
  // Send messages in the queue (mouse/key/timer/etc. events) This
  // might change the state of widgets, etc. In case pumpQueue()
  // returns a number greater than 0, it means that we've processed
  // some messages, so we've to redraw the screen.
  if (pumpQueue() > 0 || redrawState == RedrawState::RedrawDelayed) {
    // If a window has just been closed with Manager::_closeWindow()
    // after processing messages, we'll wait the next event generation
    // to process painting events (so the manager doesn't lost the
    // DIRTY flag right now).
    if (redrawState == RedrawState::AWindowHasJustBeenClosed) {
      redrawState = RedrawState::RedrawDelayed;
    }
    else {
      if (redrawState == RedrawState::RedrawDelayed)
        redrawState = RedrawState::Normal;

      // Generate and send just kPaintMessages with the latest UI state.
      flushRedraw();
      pumpQueue();

      // Flip the back-buffer to the real display.
      flipDisplay();
    }
  }
}

void Manager::addToGarbage(Widget* widget)
{
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
  std::cout << "Manager::setMouse ";
  if (widget) {
    std::cout << typeid(*widget).name();
    if (!widget->id().empty())
      std::cout << " (" << widget->id() << ")";
  }
  else {
    std::cout << "null";
  }
  std::cout << std::endl;
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
      auto msg = newMouseMessage(
        kMouseEnterMessage, NULL,
        get_mouse_position(),
        PointerType::Unknown,
        _internal_get_mouse_buttons(),
        kKeyUninitializedModifier);

      msg->setRecipient(widget);
      msg->setPropagateToParent(true);
      msg->setCommonAncestor(commonAncestor);
      enqueueMessage(msg);
      generateSetCursorMessage(get_mouse_position(),
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

  m_display->captureMouse();
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
  setFocus(NULL);
}

void Manager::freeMouse()
{
  setMouse(NULL);
}

void Manager::freeCapture()
{
  if (capture_widget) {
    capture_widget->disableFlags(HAS_CAPTURE);
    capture_widget = NULL;

    m_display->releaseMouse();
  }
}

void Manager::freeWidget(Widget* widget)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == base::this_thread::native_handle());
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
  ASSERT(manager_thread == base::this_thread::native_handle());
#endif

  for (Message* msg : msg_queue)
    msg->removeRecipient(widget);

  for (Message* msg : used_msg_queue)
    msg->removeRecipient(widget);
}

void Manager::removeMessagesFor(Widget* widget, MessageType type)
{
#ifdef DEBUG_UI_THREADS
  ASSERT(manager_thread == base::this_thread::native_handle());
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
  ASSERT(manager_thread == base::this_thread::native_handle());
#endif

  for (auto it=msg_queue.begin(); it != msg_queue.end(); ) {
    Message* msg = *it;
    if (msg->type() == kTimerMessage &&
        static_cast<TimerMessage*>(msg)->timer() == timer) {
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
  ASSERT(manager_thread == base::this_thread::native_handle());
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
  ASSERT(manager_thread == base::this_thread::native_handle());
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
  ASSERT(manager_thread == base::this_thread::native_handle());
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

void Manager::dirtyRect(const gfx::Rect& bounds)
{
  m_dirtyRegion.createUnion(m_dirtyRegion, gfx::Region(bounds));
}

// Configures the window for begin the loop
void Manager::_openWindow(Window* window)
{
  // Free all widgets of special states.
  if (window->isWantFocus()) {
    freeCapture();
    freeMouse();
    freeFocus();
  }

  // Add the window to manager.
  insertChild(0, window);

  // Broadcast the open message.
  {
    std::unique_ptr<Message> msg(new Message(kOpenMessage));
    window->sendMessage(msg.get());
  }

  // Relayout
  window->layout();

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
  Widget* widget = pick(ui::get_mouse_position());
  if (widget)
    setMouse(widget);
}

void Manager::_closeWindow(Window* window, bool redraw_background)
{
  if (!hasChild(window))
    return;

  gfx::Region reg1;
  if (redraw_background)
    window->getRegion(reg1);

  // Close all windows to this desktop
  if (window->isDesktop()) {
    while (!children().empty()) {
      Window* child = static_cast<Window*>(children().front());
      if (child == window)
        break;
      else {
        gfx::Region reg2;
        window->getRegion(reg2);
        reg1.createUnion(reg1, reg2);

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
    std::unique_ptr<Message> msg(new Message(kCloseMessage));
    window->sendMessage(msg.get());
  }

  // Update manager list stuff.
  removeChild(window);

  // Redraw background.
  invalidateRegion(reg1);

  // Update mouse widget (as it can be a widget below the
  // recently closed window).
  Widget* widget = pick(ui::get_mouse_position());
  if (widget)
    setMouse(widget);

  redrawState = RedrawState::AWindowHasJustBeenClosed;
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

    case kResizeDisplayMessage:
      onNewDisplayConfiguration();
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
  m_invalidRegion = gfx::Region(new_pos);

  const int dx = new_pos.x - old_pos.x;
  const int dy = new_pos.y - old_pos.y;
  const int dw = new_pos.w - old_pos.w;
  const int dh = new_pos.h - old_pos.h;

  for (auto child : children()) {
    Window* window = static_cast<Window*>(child);
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

void Manager::onBroadcastMouseMessage(WidgetsList& targets)
{
  // Ask to the first window in the "children" list to know how to
  // propagate mouse messages.
  Widget* widget = UI_FIRST_WIDGET(children());
  if (widget)
    widget->broadcastMouseMessage(targets);
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
        gfx::Rect bounds = window->bounds();
        bounds *= newUIScale;
        bounds /= oldUIScale;
        bounds.x = MID(0, bounds.x, m_display->width() - bounds.w);
        bounds.y = MID(0, bounds.y, m_display->height() - bounds.h);
        window->setBounds(bounds);
      }
    }
  }
}

LayoutIO* Manager::onGetLayoutIO()
{
  return NULL;
}

void Manager::onNewDisplayConfiguration()
{
  if (m_display) {
    int w = m_display->width() / m_display->scale();
    int h = m_display->height() / m_display->scale();
    if ((bounds().w != w ||
         bounds().h != h)) {
      setBounds(gfx::Rect(0, 0, w, h));
    }
  }

  _internal_set_mouse_display(m_display);
  invalidate();
  flushRedraw();
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

    // Move the message from msg_queue to used_msg_queue
    msg_queue.erase(it);
    auto eraseIt = used_msg_queue.insert(used_msg_queue.end(), msg);

    // Call Timer::tick() if this is a tick message.
    if (msg->type() == kTimerMessage) {
      ASSERT(static_cast<TimerMessage*>(msg)->timer() != NULL);
      static_cast<TimerMessage*>(msg)->timer()->tick();
    }

    bool done = false;

    // Send this message to filters
    {
      Filters& msg_filter = msg_filters[MIN(msg->type(), kFirstRegisteredMessage)];
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
  ASSERT(manager_thread == base::this_thread::native_handle());
#endif

  if (!widget)
    return false;

#ifdef REPORT_EVENTS
  {
    static const char* msg_name[] = {
      "kFunctionMessage",

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
    static_assert(kFunctionMessage == 0 &&
                  kTouchMagnifyMessage == sizeof(msg_name)/sizeof(const char*)-1,
                  "MessageType enum has changed");
    const char* string =
      (msg->type() >= 0 &&
       msg->type() < sizeof(msg_name)/sizeof(const char*)) ?
      msg_name[msg->type()]: "Unknown";

    std::cout << "Event " << msg->type() << " (" << string << ") "
              << "for " << ((void*)widget) << std::flush;
    std::cout << " (" << typeid(*widget).name() << ")";
    if (!widget->id().empty())
      std::cout << " (" << widget->id() << ")";
    std::cout << std::endl;
  }
#endif

  bool used = false;

  // We need to configure the clip region for paint messages
  // before we call Widget::sendMessage().
  if (msg->type() == kPaintMessage) {
    if (widget->hasFlags(HIDDEN))
      return false;

    PaintMessage* paintMsg = static_cast<PaintMessage*>(msg);

    // Restore overlays in the region that we're going to paint.
    OverlayManager::instance()->restoreOverlappedAreas(paintMsg->rect());

    os::Surface* surface = m_display->getSurface();
    surface->saveClip();

    if (surface->clipRect(paintMsg->rect())) {
#ifdef REPORT_EVENTS
      std::cout << " - clip("
                << paintMsg->rect().x << ", "
                << paintMsg->rect().y << ", "
                << paintMsg->rect().w << ", "
                << paintMsg->rect().h << ")"
                << std::endl;
#endif

#ifdef DEBUG_PAINT_EVENTS
      {
        os::SurfaceLock lock(surface);
        surface->fillRect(gfx::rgba(0, 0, 255), paintMsg->rect());
      }

      if (m_display)
        m_display->flip(gfx::Rect(0, 0, display_w(), display_h()));

      base::this_thread::sleep_for(0.002);
#endif

      if (surface) {
        // Call the message handler
        used = widget->sendMessage(msg);

        // Restore clip region for paint messages.
        surface->restoreClip();
      }
    }

    // As this kPaintMessage's rectangle was updated, we can
    // remove it from "m_invalidRegion".
    m_invalidRegion -= gfx::Region(paintMsg->rect());
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
    return NULL;
}

// static
Message* Manager::newMouseMessage(
  MessageType type,
  Widget* widget,
  const gfx::Point& mousePos,
  PointerType pointerType,
  MouseButtons buttons,
  KeyModifiers modifiers,
  const gfx::Point& wheelDelta,
  bool preciseWheel)
{
#ifdef __APPLE__
  // Convert Ctrl+left click -> right-click
  if (widget &&
      widget->isVisible() &&
      widget->isEnabled() &&
      widget->hasFlags(CTRL_RIGHT_CLICK) &&
      (modifiers & kKeyCtrlModifier) &&
      (buttons == kButtonLeft)) {
    modifiers = KeyModifiers(int(modifiers) & ~int(kKeyCtrlModifier));
    buttons = kButtonRight;
  }
#endif

  Message* msg = new MouseMessage(
    type, pointerType, buttons, modifiers, mousePos,
    wheelDelta, preciseWheel);

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
  int (*cmp)(Widget*, int, int) = NULL;
  Widget* focus = NULL;
  Widget* it;
  bool ret = false;
  Window* window = NULL;
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
      case kKeyLeft:  if (!cmp) cmp = cmp_left;
      case kKeyRight: if (!cmp) cmp = cmp_right;
      case kKeyUp:    if (!cmp) cmp = cmp_up;
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

  return NULL;
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
