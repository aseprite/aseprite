// Aseprite UI Library
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MANAGER_H_INCLUDED
#define UI_MANAGER_H_INCLUDED
#pragma once

#include "gfx/region.h"
#include "os/dnd.h"
#include "ui/display.h"
#include "ui/keys.h"
#include "ui/layer.h"
#include "ui/message_type.h"
#include "ui/mouse_button.h"
#include "ui/pointer_type.h"
#include "ui/widget.h"

namespace os {
class EventQueue;
class Window;
} // namespace os

namespace ui {

class LayoutIO;
class Timer;
class Window;

class Manager : public Widget,
                public os::DragTarget {
public:
  static Manager* getDefault() { return m_defaultManager; }
  static bool widgetAssociatedToManager(Widget* widget);

  Manager(const os::WindowRef& nativeWindow);
  ~Manager();

  Display* display() const { return &const_cast<Manager*>(this)->m_display; }

  static Display* getDisplayFromNativeWindow(os::Window* window);

  // Executes the main message loop.
  void run();

  // Refreshes all real displays with the UI content.
  void flipAllDisplays();

  // Updates the scale and GPU acceleration flag of all native
  // windows.
  void updateAllDisplays(int scale, bool gpu);

  // Adds the given "msg" message to the queue of messages to be
  // dispached. "msg" cannot be used after this function, it'll be
  // automatically deleted.
  void enqueueMessage(Message* msg);

  // Returns true if there are messages in the queue to be
  // dispatched through dispatchMessages().
  bool generateMessages();
  void dispatchMessages();

  // Makes the generateMessages() function to return immediately if
  // there is no user events in the OS queue. Useful only for tests
  // or benchmarks where we don't wait the user (or we don't even
  // have an user). In this case we want to use 100% of the CPU.
  void dontWaitEvents() { m_waitEvents = false; }

  void addToGarbage(Widget* widget);
  void collectGarbage();

  Window* getTopWindow() const;
  Window* getDesktopWindow() const;
  Window* getForegroundWindow() const;
  Display* getForegroundDisplay();

  Widget* getFocus();
  Widget* getMouse();
  Widget* getCapture();

  void setFocus(Widget* widget);
  void setMouse(Widget* widget);
  void setCapture(Widget* widget, bool force = false);
  void attractFocus(Widget* widget);
  void focusFirstChild(Widget* widget);
  void freeFocus();
  void freeMouse();
  void freeCapture();
  void freeWidget(Widget* widget);
  void removeMessagesFor(Widget* widget);
  void removeMessagesFor(Widget* widget, MessageType type);
  void removeMessagesForTimer(Timer* timer);
  void removeMessagesForDisplay(Display* display);
  void removePaintMessagesForDisplay(Display* display);

  void addMessageFilter(int message, Widget* widget);
  void removeMessageFilter(int message, Widget* widget);
  void removeMessageFilterFor(Widget* widget);

  LayoutIO* getLayoutIO();

  bool isFocusMovementMessage(Message* msg);
  bool processFocusMovementMessage(Message* msg);

  Widget* pickFromScreenPos(const gfx::Point& screenPos) const override;

  // Transfers the given MouseMessage received "from" (generally a
  // kMouseMoveMessage) to the given "to" widget, sending a copy of
  // the message but changing its type to kMouseDownMessage. By
  // default it enqueues the message.
  //
  // If "from" has the mouse capture, it will be released as it is
  // highly probable that the "to" widget will recapture the mouse
  // again.
  //
  // This is used in cases were the user presses the mouse button on
  // one widget, and then drags the mouse to another widget. With this
  // we can transfer the mouse capture between widgets (from -> to)
  // simulating a kMouseDownMessage for the new widget "to".
  void transferAsMouseDownMessage(Widget* from,
                                  Widget* to,
                                  const MouseMessage* mouseMsg,
                                  bool sendNow = false);

  // Returns true if the widget is accessible with the mouse, i.e. the
  // widget is in the current foreground window (or a top window above
  // the foreground, e.g. a combobox popup), or there is no foreground
  // window and the widget is in the desktop window.
  bool isWidgetClickable(const Widget* widget) const;

  void _openWindow(Window* window, bool center);
  void _closeWindow(Window* window, bool redraw_background);
  void _runModalWindow(Window* window);
  void _updateMouseWidgets();
  void _closingAppWithException();

protected:
  bool onProcessMessage(Message* msg) override;
  void onInvalidateRegion(const gfx::Region& region) override;
  void onResize(ResizeEvent& ev) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void onBroadcastMouseMessage(const gfx::Point& screenPos, WidgetsList& targets) override;
  void onInitTheme(InitThemeEvent& ev) override;
  virtual LayoutIO* onGetLayoutIO();
  virtual void onNewDisplayConfiguration(Display* display);

private:
  void generateSetCursorMessage(Display* display,
                                const gfx::Point& mousePos,
                                KeyModifiers modifiers,
                                PointerType pointerType);
  void generateMessagesFromOSEvents();

  void handleMouseMove(Display* display,
                       const gfx::Point& mousePos,
                       const KeyModifiers modifiers,
                       const PointerType pointerType,
                       const float pressure);
  void handleMouseDown(Display* display,
                       const gfx::Point& mousePos,
                       MouseButton mouseButton,
                       KeyModifiers modifiers,
                       PointerType pointerType,
                       const float pressure);
  void handleMouseUp(Display* display,
                     const gfx::Point& mousePos,
                     MouseButton mouseButton,
                     KeyModifiers modifiers,
                     PointerType pointerType);
  void handleMouseDoubleClick(Display* display,
                              const gfx::Point& mousePos,
                              MouseButton mouseButton,
                              KeyModifiers modifiers,
                              PointerType pointerType,
                              const float pressure);
  void handleMouseWheel(Display* display,
                        const gfx::Point& mousePos,
                        KeyModifiers modifiers,
                        PointerType pointerType,
                        const gfx::Point& wheelDelta,
                        bool preciseWheel);
  void handleTouchMagnify(Display* display,
                          const gfx::Point& mousePos,
                          const KeyModifiers modifiers,
                          const double magnification);
  bool handleWindowZOrder();
  void updateMouseWidgets(const gfx::Point& mousePos, Display* display);
  void allowCapture(Widget* widget);

  int pumpQueue();
  bool sendMessageToWidget(Message* msg, Widget* widget);

  Widget* findForDragAndDrop(Widget* widget);
  void dragEnter(os::DragEvent& ev) override;
  void dragLeave(os::DragEvent& ev) override;
  void drag(os::DragEvent& ev) override;
  void drop(os::DragEvent& ev) override;

  static Widget* findLowestCommonAncestor(Widget* a, Widget* b);
  static bool someParentIsFocusStop(Widget* widget);
  static Widget* findMagneticWidget(Widget* widget);
  static Message* newMouseMessage(MessageType type,
                                  Display* display,
                                  Widget* widget,
                                  const gfx::Point& mousePos,
                                  PointerType pointerType,
                                  MouseButton button,
                                  KeyModifiers modifiers,
                                  const gfx::Point& wheelDelta = gfx::Point(0, 0),
                                  bool preciseWheel = false,
                                  float pressure = 0.0f);
  void broadcastKeyMsg(Message* msg);

  static Manager* m_defaultManager;

  WidgetsList m_garbage;
  Display m_display;
  os::EventQueue* m_eventQueue;

  // This member is used to make freeWidget() a no-op when we
  // restack a window if the user clicks on it.
  Widget* m_lockedWindow;

  // Last pressed mouse button.
  MouseButton m_mouseButton;

  // Widget over which the drag is being hovered in a drag & drop operation.
  Widget* m_dragOverWidget = nullptr;

  // False if we want to continue in case that there is no user
  // event in the OS queue. Useful when there is no need to wait for
  // user interaction (tests/benchmarks).
  bool m_waitEvents = true;
};

} // namespace ui

#endif
