// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MANAGER_H_INCLUDED
#define UI_MANAGER_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "ui/message_type.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

namespace she {
  class Clipboard;
  class Display;
  class EventQueue;
}

namespace ui {

  class LayoutIO;
  class Timer;
  class Window;

  class Manager : public Widget
  {
  public:
    static Manager* getDefault() {
      return m_defaultManager;
    }

    Manager();
    ~Manager();

    she::Display* getDisplay() { return m_display; }
    she::Clipboard* getClipboard() { return m_clipboard; }

    void setDisplay(she::Display* display);
    void setClipboard(she::Clipboard* clipboard);

    void run();

    // Returns true if there are messages in the queue to be
    // distpatched through jmanager_dispatch_messages().
    bool generateMessages();
    void dispatchMessages();
    void enqueueMessage(Message* msg);

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
    void removeMessage(Message* msg);
    void removeMessagesFor(Widget* widget);
    void removeMessagesForTimer(Timer* timer);

    void addMessageFilter(int message, Widget* widget);
    void removeMessageFilter(int message, Widget* widget);
    void removeMessageFilterFor(Widget* widget);

    void invalidateDisplayRegion(const gfx::Region& region);

    LayoutIO* getLayoutIO();

    void _openWindow(Window* window);
    void _closeWindow(Window* window, bool redraw_background);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onResize(ResizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onBroadcastMouseMessage(WidgetsList& targets) OVERRIDE;
    virtual LayoutIO* onGetLayoutIO();

  private:
    void generateMouseMessages();
    void generateSetCursorMessage(const gfx::Point& mousePos);
    void generateKeyMessages();
    void generateMessagesFromSheEvents();
    void handleMouseMove(const gfx::Point& mousePos, MouseButtons mouseButtons);
    void handleMouseDown(const gfx::Point& mousePos, MouseButtons mouseButtons);
    void handleMouseUp(const gfx::Point& mousePos, MouseButtons mouseButtons);
    void handleMouseDoubleClick(const gfx::Point& mousePos, MouseButtons mouseButtons);
    void handleMouseWheel(const gfx::Point& mousePos, MouseButtons mouseButtons, const gfx::Point& wheelDelta);
    void handleWindowZOrder();

    void pumpQueue();
    static void removeWidgetFromRecipients(Widget* widget, Message* msg);
    static bool someParentIsFocusStop(Widget* widget);
    static Widget* findMagneticWidget(Widget* widget);
    static Message* newMouseMessage(MessageType type,
      Widget* widget, const gfx::Point& mousePos,
      MouseButtons buttons, const gfx::Point& wheelDelta = gfx::Point(0, 0));
    static MouseButtons currentMouseButtons(int antique);
    void broadcastKeyMsg(Message* msg);

    static Manager* m_defaultManager;

    WidgetsList m_garbage;
    she::Display* m_display;
    she::Clipboard* m_clipboard;
    she::EventQueue* m_eventQueue;

    // This member is used to make freeWidget() a no-op when we
    // restack a window if the user clicks on it.
    Widget* m_lockedWindow;

    // Current pressed buttons.
    MouseButtons m_mouseButtons;
  };

} // namespace ui

#endif
