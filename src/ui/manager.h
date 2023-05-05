// Aseprite UI Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MANAGER_H_INCLUDED
#define UI_MANAGER_H_INCLUDED
#pragma once

#include "gfx/region.h"
#include "ui/display.h"
#include "ui/keys.h"
#include "ui/message_type.h"
#include "ui/mouse_button.h"
#include "ui/pointer_type.h"
#include "ui/widget.h"

namespace os {
  class EventQueue;
  class Window;
}

namespace ui {

  class LayoutIO;
  class Timer;
  class Window;

  class Manager : public Widget {
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

    void updateAllDisplaysWithNewScale(int scale);

    // Adds the given "msg" message to the queue of messages to be
    // dispached. "msg" cannot be used after this function, it'll be
    // automatically deleted.
    void enqueueMessage(Message* msg);

    // Returns true if there are messages in the queue to be
    // dispatched through dispatchMessages().
    bool generateMessages();
    void dispatchMessages();

    void addToGarbage(Widget* widget);
    void collectGarbage();

    Window* getTopWindow();
    Window* getDesktopWindow();
    Window* getForegroundWindow();
    Display* getForegroundDisplay();

    Widget* getFocus();
    Widget* getMouse();
    Widget* getCapture();

    void setFocus(Widget* widget);
    void setMouse(Widget* widget);
    void setCapture(Widget* widget);
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
    void onBroadcastMouseMessage(const gfx::Point& screenPos,
                                 WidgetsList& targets) override;
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
    void updateMouseWidgets(const gfx::Point& mousePos,
                            Display* display);

    int pumpQueue();
    bool sendMessageToWidget(Message* msg, Widget* widget);

    static Widget* findLowestCommonAncestor(Widget* a, Widget* b);
    static bool someParentIsFocusStop(Widget* widget);
    static Widget* findMagneticWidget(Widget* widget);
    static Message* newMouseMessage(
      MessageType type,
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
  };

} // namespace ui

#endif
