// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_MANAGER_H_INCLUDED
#define UI_MANAGER_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/message_type.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

namespace she {
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
    void setDisplay(she::Display* display);

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
    void pumpQueue();
    void generateSetCursorMessage();
    static void removeWidgetFromRecipients(Widget* widget, Message* msg);
    static bool someParentIsFocusStop(Widget* widget);
    static Widget* findMagneticWidget(Widget* widget);
    static Message* newMouseMessage(MessageType type, Widget* destination);
    static Message* newMouseMessage(MessageType type, Widget* destination,
                                    MouseButtons mouseButtons);
    static MouseButtons currentMouseButtons(int antique);
    void broadcastKeyMsg(Message* msg);

    static Manager* m_defaultManager;

    WidgetsList m_garbage;
    she::Display* m_display;
    she::EventQueue* m_eventQueue;
  };

} // namespace ui

#endif
