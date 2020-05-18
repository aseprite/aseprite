// Aseprite UI Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MANAGER_H_INCLUDED
#define UI_MANAGER_H_INCLUDED
#pragma once

#include "gfx/region.h"
#include "ui/keys.h"
#include "ui/message_type.h"
#include "ui/mouse_button.h"
#include "ui/pointer_type.h"
#include "ui/widget.h"

namespace os {
  class Display;
  class EventQueue;
}

namespace ui {

  class LayoutIO;
  class Timer;
  class Window;

  class Manager : public Widget {
  public:
    static Manager* getDefault() { return m_defaultManager; }
    static bool widgetAssociatedToManager(Widget* widget);

    Manager();
    ~Manager();

    os::Display* getDisplay() { return m_display; }

    void setDisplay(os::Display* display);

    // Executes the main message loop.
    void run();

    // Refreshes the real display with the UI content.
    void flipDisplay();

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
    Window* getForegroundWindow();

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

    void addMessageFilter(int message, Widget* widget);
    void removeMessageFilter(int message, Widget* widget);
    void removeMessageFilterFor(Widget* widget);

    LayoutIO* getLayoutIO();

    bool isFocusMovementMessage(Message* msg);
    bool processFocusMovementMessage(Message* msg);

    // Returns the invalid region in the screen to being updated with
    // PaintMessages. This region is cleared when each widget receives
    // a paint message.
    const gfx::Region& getInvalidRegion() const {
      return m_invalidRegion;
    }

    void addInvalidRegion(const gfx::Region& b) {
      m_invalidRegion |= b;
    }

    // Mark the given rectangle as a area to be flipped to the real
    // screen
    void dirtyRect(const gfx::Rect& bounds);

    void _openWindow(Window* window);
    void _closeWindow(Window* window, bool redraw_background);

  protected:
    bool onProcessMessage(Message* msg) override;
    void onInvalidateRegion(const gfx::Region& region) override;
    void onResize(ResizeEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onBroadcastMouseMessage(WidgetsList& targets) override;
    void onInitTheme(InitThemeEvent& ev) override;
    virtual LayoutIO* onGetLayoutIO();
    virtual void onNewDisplayConfiguration();

  private:
    void generateSetCursorMessage(const gfx::Point& mousePos,
                                  KeyModifiers modifiers,
                                  PointerType pointerType);
    void generateMessagesFromOSEvents();
    void handleMouseMove(const gfx::Point& mousePos,
                         const KeyModifiers modifiers,
                         const PointerType pointerType,
                         const float pressure);
    void handleMouseDown(const gfx::Point& mousePos,
                         MouseButton mouseButton,
                         KeyModifiers modifiers,
                         PointerType pointerType);
    void handleMouseUp(const gfx::Point& mousePos,
                       MouseButton mouseButton,
                       KeyModifiers modifiers,
                       PointerType pointerType);
    void handleMouseDoubleClick(const gfx::Point& mousePos,
                                MouseButton mouseButton,
                                KeyModifiers modifiers,
                                PointerType pointerType);
    void handleMouseWheel(const gfx::Point& mousePos,
                          KeyModifiers modifiers,
                          PointerType pointerType,
                          const gfx::Point& wheelDelta,
                          bool preciseWheel);
    void handleTouchMagnify(const gfx::Point& mousePos,
                            const KeyModifiers modifiers,
                            const double magnification);
    void handleWindowZOrder();

    int pumpQueue();
    bool sendMessageToWidget(Message* msg, Widget* widget);

    static Widget* findLowestCommonAncestor(Widget* a, Widget* b);
    static bool someParentIsFocusStop(Widget* widget);
    static Widget* findMagneticWidget(Widget* widget);
    static Message* newMouseMessage(
      MessageType type,
      Widget* widget, const gfx::Point& mousePos,
      PointerType pointerType,
      MouseButton button,
      KeyModifiers modifiers,
      const gfx::Point& wheelDelta = gfx::Point(0, 0),
      bool preciseWheel = false,
      float pressure = 0.0f);
    void broadcastKeyMsg(Message* msg);

    static Manager* m_defaultManager;
    static gfx::Region m_dirtyRegion;

    WidgetsList m_garbage;
    os::Display* m_display;
    os::EventQueue* m_eventQueue;
    gfx::Region m_invalidRegion;  // Invalid region (we didn't receive paint messages yet for this).

    // This member is used to make freeWidget() a no-op when we
    // restack a window if the user clicks on it.
    Widget* m_lockedWindow;

    // Last pressed mouse button.
    MouseButton m_mouseButton;
  };

} // namespace ui

#endif
