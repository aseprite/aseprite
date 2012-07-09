// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_MANAGER_H_INCLUDED
#define UI_MANAGER_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class Window;
  class Timer;

  class Manager : public Widget
  {
  public:
    static Manager* getDefault();

    Manager();
    ~Manager();

    void run();

    // Returns true if there are messages in the queue to be
    // distpatched through jmanager_dispatch_messages().
    bool generateMessages();

    void dispatchMessages();

    void addToGarbage(Widget* widget);

    void enqueueMessage(Message* msg);

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

    void invalidateDisplayRegion(const JRegion region);

    void _openWindow(Window* window);
    void _closeWindow(Window* window, bool redraw_background);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onBroadcastMouseMessage(WidgetsList& targets) OVERRIDE;

  private:
    void layoutManager(JRect rect);
    void pumpQueue();
    void collectGarbage();
    void generateSetCursorMessage();
    static void removeWidgetFromDests(Widget* widget, Message* msg);
    static bool someParentIsFocusStop(Widget* widget);
    static Widget* findMagneticWidget(Widget* widget);
    static Message* newMouseMessage(int type, Widget* destination);
    void broadcastKeyMsg(Message* msg);

    static Manager* m_defaultManager;

    WidgetsList m_garbage;
  };

} // namespace ui

#endif
