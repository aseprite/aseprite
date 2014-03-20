// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

// #define REPORT_EVENTS
// #define LIMIT_DISPATCH_TIME

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"

#include "she/display.h"
#include "she/event.h"
#include "she/event_queue.h"
#include "ui/intern.h"
#include "ui/ui.h"

#ifdef REPORT_EVENTS
#include <cstdio>
#endif
#include <allegro.h>
#include <list>
#include <vector>

namespace ui {

#define ACCEPT_FOCUS(widget)                            \
  (((widget)->flags & (JI_FOCUSSTOP |                   \
                       JI_DISABLED |                    \
                       JI_HIDDEN |                      \
                       JI_DECORATIVE)) == JI_FOCUSSTOP)

#define DOUBLE_CLICK_TIMEOUT_MSECS   400

enum {
  DOUBLE_CLICK_NONE,
  DOUBLE_CLICK_DOWN,
  DOUBLE_CLICK_UP
};

enum {
  STAGE_NORMAL,
  STAGE_WANT_CLOSE,
  STAGE_WAITING_REPLY,
  STAGE_CLOSE_ALL,
};

static const int NFILTERS = (int)(kFirstRegisteredMessage+1);

struct Filter
{
  int message;
  Widget* widget;

  Filter(int message, Widget* widget)
    : message(message)
    , widget(widget) { }
};

typedef std::list<Message*> Messages;
typedef std::list<Filter*> Filters;

static int double_click_level;
static MouseButtons double_click_buttons;
static int double_click_ticks;

static int want_close_stage;       /* variable to handle the external
                                      close button in some Windows
                                      enviroments */

Manager* Manager::m_defaultManager = NULL;

static WidgetsList new_windows; // Windows that we should show
static WidgetsList mouse_widgets_list; // List of widgets to send mouse events
static Messages msg_queue;             // Messages queue
static Filters msg_filters[NFILTERS]; // Filters for every enqueued message

static Widget* focus_widget;    // The widget with the focus
static Widget* mouse_widget;    // The widget with the mouse
static Widget* capture_widget;  // The widget that captures the mouse

static bool first_time_poll;    /* true when we don't enter in poll yet */

static char old_readed_key[KEY_MAX]; /* keyboard status of previous
                                        poll */

static unsigned key_repeated[KEY_MAX];

/* keyboard focus movement stuff */
static bool move_focus(Manager* manager, Message* msg);
static int count_widgets_accept_focus(Widget* widget);
static bool childs_accept_focus(Widget* widget, bool first);
static Widget* next_widget(Widget* widget);
static int cmp_left(Widget* widget, int x, int y);
static int cmp_right(Widget* widget, int x, int y);
static int cmp_up(Widget* widget, int x, int y);
static int cmp_down(Widget* widget, int x, int y);

/* hooks the close-button in some platform with window support */
static void allegro_window_close_hook()
{
  if (want_close_stage == STAGE_NORMAL)
    want_close_stage = STAGE_WANT_CLOSE;
}

Manager::Manager()
  : Widget(kManagerWidget)
  , m_display(NULL)
  , m_eventQueue(NULL)
{
  if (!m_defaultManager) {
    // Hook the window close message
    want_close_stage = STAGE_NORMAL;
    set_close_button_callback(allegro_window_close_hook);

    // Empty lists
    ASSERT(msg_queue.empty());
    ASSERT(new_windows.empty());
    mouse_widgets_list.clear();

    // Reset variables
    focus_widget = NULL;
    mouse_widget = NULL;
    capture_widget = NULL;

    first_time_poll = true;
    double_click_level = DOUBLE_CLICK_NONE;
    double_click_ticks = 0;

    // Reset keyboard
    for (int c=0; c<KEY_MAX; c++) {
      old_readed_key[c] = 0;
      key_repeated[c] = 0;
    }
  }

  setBounds(gfx::Rect(0, 0, JI_SCREEN_W, JI_SCREEN_H));
  setVisible(true);

  // Default manager is the first one (and is always visible).
  if (!m_defaultManager)
    m_defaultManager = this;
}

Manager::~Manager()
{
  // There are some messages in queue? Dispatch everything.
  dispatchMessages();
  collectGarbage();

  // Finish the main manager.
  if (m_defaultManager == this) {
    // No more cursor
    jmouse_set_cursor(kNoCursor);

    // Destroy timers
    Timer::checkNoTimers();

    // Destroy filters
    for (int c=0; c<NFILTERS; ++c) {
      for (Filters::iterator it=msg_filters[c].begin(), end=msg_filters[c].end();
           it != end; ++it)
        delete *it;
      msg_filters[c].clear();
    }

    // No more default manager
    m_defaultManager = NULL;

    // Shutdown system
    ASSERT(msg_queue.empty());
    ASSERT(new_windows.empty());
    mouse_widgets_list.clear();
  }
}

void Manager::setDisplay(she::Display* display)
{
  m_display = display;
  m_eventQueue = m_display->getEventQueue();
}

void Manager::run()
{
  MessageLoop loop(this);

  while (!getChildren().empty())
    loop.pumpMessages();
}

bool Manager::generateMessages()
{
  Widget* widget;
  Widget* window;
  int c;

  // Poll keyboard
  poll_keyboard();

  if (first_time_poll) {
    first_time_poll = false;

    Manager::getDefault()->invalidate();
    jmouse_set_cursor(kArrowCursor);
  }

  // First check: there are windows to manage?
  if (getChildren().empty())
    return false;

  // New windows to show?
  if (!new_windows.empty()) {
    UI_FOREACH_WIDGET(new_windows, it) {
      window = *it;

      // Relayout
      window->layout();

      // Dirty the entire window and show it
      window->setVisible(true);
      window->invalidate();

      // Attract the focus to the magnetic widget...
      // 1) get the magnetic widget
      Widget* magnet = findMagneticWidget(window->getRoot());
      // 2) if magnetic widget exists and it doesn't have the focus
      if (magnet && !magnet->hasFocus())
        setFocus(magnet);
      // 3) if not, put the focus in the first child
      else
        focusFirstChild(window);
    }

    new_windows.clear();
  }

  // Update mouse status
  bool mousemove = jmouse_poll();
  if (mousemove || !mouse_widget) {
    // Get the list of widgets to send mouse messages.
    mouse_widgets_list.clear();
    broadcastMouseMessage(mouse_widgets_list);

    // Get the widget under the mouse
    widget = NULL;

    UI_FOREACH_WIDGET(mouse_widgets_list, it) {
      widget = (*it)->pick(gfx::Point(jmouse_x(0), jmouse_y(0)));
      if (widget)
        break;
    }

    // Fixup "mouse" flag
    if (widget != mouse_widget) {
      if (!widget)
        freeMouse();
      else
        setMouse(widget);
    }

    // Mouse movement
    if (mousemove) {
      Widget* dst;

      // Reset double click status
      double_click_level = DOUBLE_CLICK_NONE;

      if (capture_widget)
        dst = capture_widget;
      else
        dst = mouse_widget;

      // Send the mouse movement message
      enqueueMessage(newMouseMessage(kMouseMoveMessage, dst,
                                     currentMouseButtons(0)));
      generateSetCursorMessage();
    }
  }

  // Mouse wheel
  if (jmouse_z(0) != jmouse_z(1)) {
    enqueueMessage(newMouseMessage(kMouseWheelMessage,
                                   capture_widget ? capture_widget:
                                                    mouse_widget,
                                   currentMouseButtons(0)));
  }

  // Mouse clicks
  if (jmouse_b(0) != jmouse_b(1)) {
    int current_ticks = ji_clock;
    bool pressed =
      ((jmouse_b(1) & 1) == 0 && (jmouse_b(0) & 1) == 1) ||
      ((jmouse_b(1) & 2) == 0 && (jmouse_b(0) & 2) == 2) ||
      ((jmouse_b(1) & 4) == 0 && (jmouse_b(0) & 4) == 4);
    MessageType msgType = (pressed ? kMouseDownMessage: kMouseUpMessage);
    MouseButtons mouseButtons = currentMouseButtons(pressed ? 0: 1);

    //////////////////////////////////////////////////////////////////////
    // Double Click
    if (msgType == kMouseDownMessage) {
      if (double_click_level != DOUBLE_CLICK_NONE) {
        // Time out, back to NONE
        if (current_ticks - double_click_ticks > DOUBLE_CLICK_TIMEOUT_MSECS) {
          double_click_level = DOUBLE_CLICK_NONE;
        }
        else if (double_click_buttons == mouseButtons) {
          if (double_click_level == DOUBLE_CLICK_UP) {
            msgType = kDoubleClickMessage;
          }
          else {
            double_click_level = DOUBLE_CLICK_NONE;
          }
        }
        // Press other button, back to NONE
        else {
          double_click_level = DOUBLE_CLICK_NONE;
        }
      }

      // This could be the beginning of the state
      if (double_click_level == DOUBLE_CLICK_NONE) {
        double_click_level = DOUBLE_CLICK_DOWN;
        double_click_buttons = mouseButtons;
        double_click_ticks = current_ticks;
      }
    }
    else if (msgType == kMouseUpMessage) {
      if (double_click_level != DOUBLE_CLICK_NONE) {
        // Time out, back to NONE
        if (current_ticks - double_click_ticks > DOUBLE_CLICK_TIMEOUT_MSECS) {
          double_click_level = DOUBLE_CLICK_NONE;
        }
        else if (double_click_buttons == mouseButtons) {
          if (double_click_level == DOUBLE_CLICK_DOWN) {
            double_click_level = DOUBLE_CLICK_UP;
            double_click_ticks = current_ticks;
          }
        }
        // Press other button, back to NONE
        else {
          double_click_level = DOUBLE_CLICK_NONE;
        }
      }
    }

    // Handle Z order: Send the window to top (only when you click in
    // a window that aren't the desktop).
    if (msgType == kMouseDownMessage && !capture_widget && mouse_widget) {
      // The clicked window
      Window* window = mouse_widget->getRoot();
      Manager* win_manager = (window ? window->getManager(): NULL);

      if ((window) &&
          // We cannot change Z-order of desktop windows
          (!window->isDesktop()) &&
          // We cannot change Z order of foreground windows because a
          // foreground window can launch other background windows
          // which should be kept on top of the foreground one.
          (!window->isForeground()) &&
          // If the window is not already the top window of the manager.
          (window != win_manager->getTopWindow())) {
        // Put it in the top of the list
        win_manager->removeChild(window);

        if (window->isOnTop())
          win_manager->insertChild(0, window);
        else {
          int pos = (int)win_manager->getChildren().size();
          UI_FOREACH_WIDGET_BACKWARD(win_manager->getChildren(), it) {
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

    enqueueMessage(newMouseMessage(msgType,
                                   capture_widget ? capture_widget:
                                                    mouse_widget,
                                   mouseButtons));
  }

  // Generate Close message when the user press close button on the system window.
  if (want_close_stage == STAGE_WANT_CLOSE) {
    want_close_stage = STAGE_NORMAL;

    Message* msg = new Message(kCloseAppMessage);
    msg->broadcastToChildren(this);
    enqueueMessage(msg);
  }

  // Generate kKeyDownMessage messages.
  while (keypressed()) {
    int scancode;
    int unicode_char = ureadkey(&scancode);
    int repeat = 0;
    {
      c = scancode;
      if (c >= 0 && c < KEY_MAX) {
        old_readed_key[c] = key[c];
        repeat = key_repeated[c]++;
      }
    }

    Message* msg = new KeyMessage(kKeyDownMessage,
                                  static_cast<KeyScancode>(scancode),
                                  unicode_char, repeat);
    broadcastKeyMsg(msg);
    enqueueMessage(msg);
  }

  for (c=0; c<KEY_MAX; c++) {
    if (old_readed_key[c] != key[c]) {
      KeyScancode scancode = static_cast<ui::KeyScancode>(c);

      // Generate kKeyUpMessage messages (old key state is activated,
      // the new one is deactivated).
      if (old_readed_key[c]) {
        // Press/release key interface
        Message* msg = new KeyMessage(kKeyUpMessage,
                                      scancode,
                                      scancode_to_unicode(scancode), 0);
        old_readed_key[c] = key[c];
        key_repeated[c] = 0;

        broadcastKeyMsg(msg);
        enqueueMessage(msg);
      }
      // Generate kKeyDownMessage messages for modifiers
      else if (c >= kKeyFirstModifierScancode) {
        // Press/release key interface
        Message* msg = new KeyMessage(kKeyDownMessage,
                                      scancode,
                                      scancode_to_unicode(scancode),
                                      key_repeated[c]++);
        old_readed_key[c] = key[c];

        broadcastKeyMsg(msg);
        enqueueMessage(msg);
      }
    }
  }

  // Events from "she" layer.
  she::Event sheEvent;
  for (;;) {
    m_eventQueue->getEvent(sheEvent);
    if (sheEvent.type() == she::Event::None)
      break;

    switch (sheEvent.type()) {

      case she::Event::DropFiles:
        {
          Message* msg = new DropFilesMessage(sheEvent.files());
          msg->addRecipient(this);
          enqueueMessage(msg);
        }
        break;

    }
  }

  // Generate messages for timers
  Timer::pollTimers();

  // Generate redraw events.
  flushRedraw();

  if (!msg_queue.empty())
    return true;
  else
    return false;
}

void Manager::dispatchMessages()
{
  // Add the "Queue Processing" message for the manager.
  enqueueMessage(newMouseMessage(kQueueProcessingMessage, this,
                                 currentMouseButtons(0)));

  pumpQueue();
}

void Manager::addToGarbage(Widget* widget)
{
  m_garbage.push_back(widget);
}

/**
 * @param msg You can't use the this message after calling this
 *            routine. The message will be automatically freed through
 *            @ref jmessage_free
 */
void Manager::enqueueMessage(Message* msg)
{
  int c;

  ASSERT(msg != NULL);

  // Check if this message must be filtered by some widget before
  c = msg->type();
  if (c >= kFirstRegisteredMessage)
    c = kFirstRegisteredMessage;

  if (!msg_filters[c].empty()) { // OK, so are filters to add...
    // Add all the filters in the destination list of the message
    for (Filters::reverse_iterator it=msg_filters[c].rbegin(),
           end=msg_filters[c].rend(); it != end; ++it) {
      Filter* filter = *it;
      if (msg->type() == filter->message)
        msg->prependRecipient(filter->widget);
    }
  }

  if (msg->hasRecipients())
    msg_queue.push_back(msg);
  else
    delete msg;
}

Window* Manager::getTopWindow()
{
  return static_cast<Window*>(UI_FIRST_WIDGET(getChildren()));
}

Window* Manager::getForegroundWindow()
{
  UI_FOREACH_WIDGET(getChildren(), it) {
    Window* window = static_cast<Window*>(*it);
    if (window->isForeground() ||
        window->isDesktop())
      return window;
  }
  return NULL;
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
          || (!(widget->flags & JI_DISABLED)
              && !(widget->flags & JI_HIDDEN)
              && !(widget->flags & JI_DECORATIVE)
              && someParentIsFocusStop(widget)))) {
    WidgetsList widget_parents;
    Widget* common_parent = NULL;

    if (widget)
      widget->getParents(false, widget_parents);

    // Fetch the focus
    if (focus_widget) {
      WidgetsList focus_parents;
      focus_widget->getParents(true, focus_parents);

      Message* msg = new Message(kFocusLeaveMessage);

      UI_FOREACH_WIDGET(focus_parents, it) {
        if (widget) {
          UI_FOREACH_WIDGET(widget_parents, it2) {
            if (*it == *it2) {
              common_parent = *it;
              break;
            }
          }
          if (common_parent)
            break;
        }

        if ((*it)->hasFocus()) {
          (*it)->flags &= ~JI_HASFOCUS;
          msg->addRecipient(*it);
        }
      }

      enqueueMessage(msg);
    }

    // Put the focus
    focus_widget = widget;
    if (widget) {
      WidgetsList::iterator it;

      if (common_parent) {
        it = std::find(widget_parents.begin(),
                       widget_parents.end(),
                       common_parent);
        ASSERT(it != widget_parents.end());
        ++it;
      }
      else
        it = widget_parents.begin();

      Message* msg = new Message(kFocusEnterMessage);

      for (; it != widget_parents.end(); ++it) {
        Widget* w = *it;

        if (w->flags & JI_FOCUSSTOP) {
          w->flags |= JI_HASFOCUS;

          msg->addRecipient(w);
        }
      }

      enqueueMessage(msg);
    }
  }
}

void Manager::setMouse(Widget* widget)
{
  if ((mouse_widget != widget) && (!capture_widget)) {
    WidgetsList widget_parents;
    Widget* common_parent = NULL;

    if (widget)
      widget->getParents(false, widget_parents);

    // Fetch the mouse
    if (mouse_widget) {
      WidgetsList mouse_parents;
      mouse_widget->getParents(true, mouse_parents);

      Message* msg = new Message(kMouseLeaveMessage);

      UI_FOREACH_WIDGET(mouse_parents, it) {
        if (widget) {
          UI_FOREACH_WIDGET(widget_parents, it2) {
            if (*it == *it2) {
              common_parent = *it;
              break;
            }
          }
          if (common_parent)
            break;
        }

        if ((*it)->hasMouse()) {
          (*it)->flags &= ~JI_HASMOUSE;
          msg->addRecipient(*it);
        }
      }

      enqueueMessage(msg);
    }

    // Put the mouse
    mouse_widget = widget;
    if (widget) {
      WidgetsList::iterator it;

      if (common_parent) {
        it = std::find(widget_parents.begin(),
                       widget_parents.end(),
                       common_parent);
        ASSERT(it != widget_parents.end());
        ++it;
      }
      else
        it = widget_parents.begin();

      Message* msg = newMouseMessage(kMouseEnterMessage, NULL,
                                     currentMouseButtons(0));

      for (; it != widget_parents.end(); ++it) {
        (*it)->flags |= JI_HASMOUSE;
        msg->addRecipient(*it);
      }

      enqueueMessage(msg);
      generateSetCursorMessage();
    }
  }
}

void Manager::setCapture(Widget* widget)
{
  widget->flags |= JI_HASCAPTURE;
  capture_widget = widget;
}

// Sets the focus to the "magnetic" widget inside the window
void Manager::attractFocus(Widget* widget)
{
  // Get the magnetic widget
  Widget* magnet = findMagneticWidget(widget->getRoot());

  // If magnetic widget exists and it doesn't have the focus
  if (magnet && !magnet->hasFocus())
    setFocus(magnet);
}

void Manager::focusFirstChild(Widget* widget)
{
  for (Widget* it=widget->getRoot(); it; it=next_widget(it)) {
    if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true))) {
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
    capture_widget->flags &= ~JI_HASCAPTURE;
    capture_widget = NULL;
  }
}

void Manager::freeWidget(Widget* widget)
{
  // Break any relationship with the GUI manager

  if (widget->hasCapture() || (widget == capture_widget))
    freeCapture();

  if (widget->hasMouse() || (widget == mouse_widget))
    freeMouse();

  if (widget->hasFocus() || (widget == focus_widget))
    freeFocus();
}

void Manager::removeMessage(Message* msg)
{
  Messages::iterator it = std::find(msg_queue.begin(), msg_queue.end(), msg);
  ASSERT(it != msg_queue.end());
  msg_queue.erase(it);
}

void Manager::removeMessagesFor(Widget* widget)
{
  for (Messages::iterator it=msg_queue.begin(), end=msg_queue.end();
       it != end; ++it)
    removeWidgetFromRecipients(widget, *it);
}

void Manager::removeMessagesForTimer(Timer* timer)
{
  for (Messages::iterator it=msg_queue.begin(); it != msg_queue.end(); ) {
    Message* msg = *it;

    if (!msg->isUsed() &&
        msg->type() == kTimerMessage &&
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
  int c = message;
  if (c >= kFirstRegisteredMessage)
    c = kFirstRegisteredMessage;

  msg_filters[c].push_back(new Filter(message, widget));
}

void Manager::removeMessageFilter(int message, Widget* widget)
{
  int c = message;
  if (c >= kFirstRegisteredMessage)
    c = kFirstRegisteredMessage;

  for (Filters::iterator it=msg_filters[c].begin(); it != msg_filters[c].end(); ) {
    Filter* filter = *it;
    if (filter->widget == widget) {
      delete filter;
      it = msg_filters[c].erase(it);
    }
    else
      ++it;
  }
}

void Manager::removeMessageFilterFor(Widget* widget)
{
  for (int c=0; c<NFILTERS; ++c) {
    for (Filters::iterator it=msg_filters[c].begin(); it != msg_filters[c].end(); ) {
      Filter* filter = *it;
      if (filter->widget == widget) {
        delete filter;
        it = msg_filters[c].erase(it);
      }
      else
        ++it;
    }
  }
}

/* configures the window for begin the loop */
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
  Message* msg = new Message(kOpenMessage);
  msg->addRecipient(window);
  enqueueMessage(msg);

  // Update the new windows list to show.
  new_windows.push_back(window);
}

void Manager::_closeWindow(Window* window, bool redraw_background)
{
  Message* msg;
  gfx::Region reg1;

  if (!hasChild(window))
    return;

  if (redraw_background)
    window->getRegion(reg1);

  // Close all windows to this desktop
  if (window->isDesktop()) {
    while (!getChildren().empty()) {
      Window* child = static_cast<Window*>(getChildren().front());
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
  if (capture_widget != NULL && capture_widget->getRoot() == window)
    freeCapture();

  if (mouse_widget != NULL && mouse_widget->getRoot() == window)
    freeMouse();

  if (focus_widget != NULL && focus_widget->getRoot() == window)
    freeFocus();

  // Hide window.
  window->setVisible(false);

  // Close message.
  msg = new Message(kCloseMessage);
  msg->addRecipient(window);
  enqueueMessage(msg);

  // Update manager list stuff.
  removeChild(window);

  // Redraw background.
  invalidateRegion(reg1);

  // Maybe the window is in the "new_windows" list.
  WidgetsList::iterator it =
    std::find(new_windows.begin(), new_windows.end(), window);
  if (it != new_windows.end())
    new_windows.erase(it);
}

bool Manager::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kKeyDownMessage:
    case kKeyUpMessage: {
      KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
      keymsg->setPropagateToChildren(true);
      keymsg->setPropagateToParent(false);

      // Continue sending the message to the children of all windows
      // (until a desktop or foreground window).
      UI_FOREACH_WIDGET(getChildren(), it) {
        Window* w = static_cast<Window*>(*it);

        // Send to the window.
        UI_FOREACH_WIDGET(w->getChildren(), it2)
          if ((*it2)->sendMessage(msg))
            return true;

        if (w->isForeground() ||
            w->isDesktop())
          break;
      }

      // Check the focus movement.
      if (msg->type() == kKeyDownMessage)
        move_focus(this, msg);

      return true;
    }

  }

  return Widget::onProcessMessage(msg);
}

void Manager::onResize(ResizeEvent& ev)
{
  gfx::Rect old_pos = getBounds();
  gfx::Rect new_pos = ev.getBounds();
  setBoundsQuietly(new_pos);

  // Offset for all windows
  int dx = new_pos.x - old_pos.x;
  int dy = new_pos.y - old_pos.y;

  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
    gfx::Rect cpos = child->getBounds();
    cpos.offset(dx, dy);
    child->setBounds(cpos);
  }
}

void Manager::onPaint(PaintEvent& ev)
{
  getTheme()->paintDesktop(ev);
}

void Manager::onBroadcastMouseMessage(WidgetsList& targets)
{
  // Ask to the first window in the "children" list to know how to
  // propagate mouse messages.
  Widget* widget = UI_FIRST_WIDGET(getChildren());
  if (widget)
    widget->broadcastMouseMessage(targets);
}

LayoutIO* Manager::onGetLayoutIO()
{
  return NULL;
}

void Manager::onPreferredSize(PreferredSizeEvent& ev)
{
  int w = 0, h = 0;

  if (!getParent()) {        // hasn' parent?
    w = getBounds().w;
    h = getBounds().h;
  }
  else {
    gfx::Rect pos = getParent()->getChildrenBounds();

    UI_FOREACH_WIDGET(getChildren(), it) {
      gfx::Rect cpos = (*it)->getBounds();
      pos = pos.createUnion(cpos);
    }

    w = pos.w;
    h = pos.h;
  }

  ev.setPreferredSize(gfx::Size(w, h));
}

void Manager::pumpQueue()
{
#ifdef LIMIT_DISPATCH_TIME
  int t = ji_clock;
#endif

  Messages::iterator it = msg_queue.begin();
  while (it != msg_queue.end()) {
#ifdef LIMIT_DISPATCH_TIME
    if (ji_clock-t > 250)
      break;
#endif

    // The message to process
    Message* msg = *it;

    // Go to next message
    if (msg->isUsed()) {
      ++it;
      continue;
    }

    // This message is in use
    msg->markAsUsed();
    Message* first_msg = msg;

    // Call Timer::tick() if this is a tick message.
    if (msg->type() == kTimerMessage) {
      ASSERT(static_cast<TimerMessage*>(msg)->timer() != NULL);
      static_cast<TimerMessage*>(msg)->timer()->tick();
    }

    bool done = false;
    UI_FOREACH_WIDGET(msg->recipients(), it2) {
      Widget* widget = *it2;
      if (!widget)
        continue;

#ifdef REPORT_EVENTS
      {
        static char *msg_name[] = {
          "kOpenMessage",
          "kCloseMessage",
          "kCloseAppMessage",
          "kPaintMessage",
          "kTimerMessage",
          "kWinMoveMessage",
          "kQueueProcessingMessage",

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
        };
        const char *string =
          (msg->type >= kOpenMessage &&
           msg->type <= kMouseWheelMessage) ? msg_name[msg->type]:
                                              "Unknown";

        printf("Event #%d: %s (%d)\n", msg->type, string, widget->id);
        fflush(stdout);
      }
#endif

      // We need to configure the clip region for paint messages
      // before we call Widget::sendMessage().
      if (msg->type() == kPaintMessage) {
        if (widget->flags & JI_HIDDEN)
          continue;

        PaintMessage* paintMsg = static_cast<PaintMessage*>(msg);

        jmouse_hide();
        acquire_bitmap(ji_screen);

        ASSERT(get_clip_state(ji_screen));
        set_clip_rect(ji_screen,
                      paintMsg->rect().x,
                      paintMsg->rect().y,
                      paintMsg->rect().x2()-1,
                      paintMsg->rect().y2()-1);
#ifdef REPORT_EVENTS
        printf(" - clip(%d, %d, %d, %d)\n",
               paintMsg->rect().x,
               paintMsg->rect().y,
               paintMsg->rect().x2()-1,
               paintMsg->rect().y2()-1);
        fflush(stdout);
#endif

        dirty_display_flag = true;
      }

      // Call the message handler
      done = widget->sendMessage(msg);

      // Restore clip region for paint messages.
      if (msg->type() == kPaintMessage) {
        set_clip_rect(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);

        release_bitmap(ji_screen);
        jmouse_show();
      }

      if (done)
        break;
    }

    // Remove the message from the msg_queue
    it = msg_queue.erase(it);

    // Destroy the message
    delete first_msg;
  }
}

void Manager::invalidateDisplayRegion(const gfx::Region& region)
{
  // TODO intersect with getDrawableRegion()???
  gfx::Region reg1;
  reg1.createIntersection(region, gfx::Region(getBounds()));

  // Redraw windows from top to background.
  bool withDesktop = false;
  UI_FOREACH_WIDGET(getChildren(), it) {
    Window* window = static_cast<Window*>(*it);

    // Invalidate regions of this window
    window->invalidateRegion(reg1);

    // There is desktop?
    if (window->isDesktop()) {
      withDesktop = true;
      break;                                    // Work done
    }

    // Clip this window area for the next window.
    gfx::Region reg3;
    window->getRegion(reg3);
    reg1.createSubtraction(reg1, reg3);
  }

  // Invalidate areas outside windows (only when there are not a
  // desktop window).
  if (!withDesktop)
    Widget::invalidateRegion(reg1);
}

LayoutIO* Manager::getLayoutIO()
{
  return onGetLayoutIO();
}

void Manager::collectGarbage()
{
  if (m_garbage.empty())
    return;

  for (WidgetsList::iterator
         it = m_garbage.begin(),
         end = m_garbage.end(); it != end; ++it) {
    delete *it;
  }
  m_garbage.clear();
}

/**********************************************************************
                        Internal routines
 **********************************************************************/

// static
void Manager::generateSetCursorMessage()
{
  Widget* dst;
  if (capture_widget)
    dst = capture_widget;
  else
    dst = mouse_widget;

  if (dst)
    enqueueMessage(newMouseMessage(kSetCursorMessage, dst,
                                   currentMouseButtons(0)));
  else
    jmouse_set_cursor(kArrowCursor);
}

// static
void Manager::removeWidgetFromRecipients(Widget* widget, Message* msg)
{
  msg->removeRecipient(widget);
}

// static
bool Manager::someParentIsFocusStop(Widget* widget)
{
  if (widget->isFocusStop())
    return true;

  if (widget->getParent())
    return someParentIsFocusStop(widget->getParent());
  else
    return false;
}

// static
Widget* Manager::findMagneticWidget(Widget* widget)
{
  Widget* found;

  UI_FOREACH_WIDGET(widget->getChildren(), it) {
    found = findMagneticWidget(*it);
    if (found)
      return found;
  }

  if (widget->isFocusMagnet())
    return widget;
  else
    return NULL;
}

// static
Message* Manager::newMouseMessage(MessageType type, Widget* widget,
                                  MouseButtons buttons)
{
  Message* msg =
    new MouseMessage(type, buttons,
                     gfx::Point(jmouse_x(0),
                                jmouse_y(0)));

  if (widget != NULL)
    msg->addRecipient(widget);

  return msg;
}

// static
MouseButtons Manager::currentMouseButtons(int antique)
{
  return jmouse_b(antique);
}

// static
void Manager::broadcastKeyMsg(Message* msg)
{
  // Send the message to the widget with capture
  if (capture_widget) {
    msg->addRecipient(capture_widget);
  }
  // Send the msg to the focused widget
  else if (focus_widget) {
    msg->addRecipient(focus_widget);
  }
  // Finally, send the message to the manager, it'll know what to do
  else {
    msg->addRecipient(this);
  }
}

/***********************************************************************
                            Focus Movement
 ***********************************************************************/

static bool move_focus(Manager* manager, Message* msg)
{
  int (*cmp)(Widget*, int, int) = NULL;
  Widget* focus = NULL;
  Widget* it;
  bool ret = false;
  Window* window = NULL;
  int c, count;

  // Who have the focus
  if (focus_widget) {
    window = focus_widget->getRoot();
  }
  else if (!manager->getChildren().empty()) {
    window = manager->getTopWindow();
  }

  if (!window)
    return false;

  // How many child-widget want the focus in this widget?
  count = count_widgets_accept_focus(window);

  // One at least
  if (count > 0) {
    std::vector<Widget*> list(count);

    c = 0;

    /* list's 1st element is the focused widget */
    for (it=focus_widget; it; it=next_widget(it)) {
      if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true)))
        list[c++] = it;
    }

    for (it=window; it != focus_widget; it=next_widget(it)) {
      if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true)))
        list[c++] = it;
    }

    // Depending on the pressed key...
    switch (static_cast<KeyMessage*>(msg)->scancode()) {

      case kKeyTab:
        // Reverse tab
        if ((msg->keyModifiers() & (kKeyShiftModifier | kKeyCtrlModifier | kKeyAltModifier)) != 0) {
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
          int i, j, x, y;

          // Position where the focus come
          x = ((focus_widget) ? focus_widget->getBounds().x+focus_widget->getBounds().x2():
                                window->getBounds().x+window->getBounds().x2())
            / 2;
          y = ((focus_widget) ? focus_widget->getBounds().y+focus_widget->getBounds().y2():
                                window->getBounds().y+window->getBounds().y2())
            / 2;

          c = focus_widget ? 1: 0;

          // Rearrange the list
          for (i=c; i<count-1; i++) {
            for (j=i+1; j<count; j++) {
              // Sort the list in ascending order
              if ((*cmp) (list[i], x, y) > (*cmp) (list[j], x, y)) {
                Widget* tmp = list[i];
                list[i] = list[j];
                list[j] = tmp;
              }
            }
          }

          // Check if the new widget to put the focus is not in the wrong way.
          if ((*cmp) (list[c], x, y) < INT_MAX)
            focus = list[c];
        }
        // If only there are one widget, put the focus in this
        else
          focus = list[0];

        ret = true;
        break;
    }

    if ((focus) && (focus != focus_widget))
      Manager::getDefault()->setFocus(focus);
  }

  return ret;
}

static int count_widgets_accept_focus(Widget* widget)
{
  ASSERT(widget != NULL);

  int count = 0;

  UI_FOREACH_WIDGET(widget->getChildren(), it)
    count += count_widgets_accept_focus(*it);

  if ((count == 0) && (ACCEPT_FOCUS(widget)))
    count++;

  return count;
}

static bool childs_accept_focus(Widget* widget, bool first)
{
  UI_FOREACH_WIDGET(widget->getChildren(), it)
    if (childs_accept_focus(*it, false))
      return true;

  return first ? false: ACCEPT_FOCUS(widget);
}

static Widget* next_widget(Widget* widget)
{
  if (!widget->getChildren().empty())
    return UI_FIRST_WIDGET(widget->getChildren());

  while (widget->getParent()->type != kManagerWidget) {
    WidgetsList::const_iterator begin = widget->getParent()->getChildren().begin();
    WidgetsList::const_iterator end = widget->getParent()->getChildren().end();
    WidgetsList::const_iterator it = std::find(begin, end, widget);

    ASSERT(it != end);

    if ((it+1) != end)
      return *(it+1);
    else
      widget = widget->getParent();
  }

  return NULL;
}

static int cmp_left(Widget* widget, int x, int y)
{
  int z = x - (widget->getBounds().x+widget->getBounds().w/2);
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->getBounds().y+widget->getBounds().h/2) - y) * 8;
}

static int cmp_right(Widget* widget, int x, int y)
{
  int z = (widget->getBounds().x+widget->getBounds().w/2) - x;
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->getBounds().y+widget->getBounds().h/2) - y) * 8;
}

static int cmp_up(Widget* widget, int x, int y)
{
  int z = y - (widget->getBounds().y+widget->getBounds().h/2);
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->getBounds().x+widget->getBounds().w/2) - x) * 8;
}

static int cmp_down(Widget* widget, int x, int y)
{
  int z = (widget->getBounds().y+widget->getBounds().h/2) - y;
  if (z <= 0)
    return INT_MAX;
  return z + ABS((widget->getBounds().x+widget->getBounds().w/2) - x) * 8;
}

} // namespace ui
