// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/x11/window.h"

#include "gfx/rect.h"
#include "she/event.h"

#include <map>

namespace she {

namespace {

// See https://bugs.freedesktop.org/show_bug.cgi?id=12871 for more
// information, it looks like the official way to convert a X Window
// into our own user data pointer (X11Window instance) is using a map.
std::map<::Window, X11Window*> g_activeWindows;

KeyModifiers get_modifiers_from_xevent(int state)
{
  int modifiers = kKeyNoneModifier;
  if (state & ShiftMask) modifiers |= kKeyShiftModifier;
  if (state & LockMask) modifiers |= kKeyAltModifier;
  if (state & ControlMask) modifiers |= kKeyCtrlModifier;
  return (KeyModifiers)modifiers;
}

} // anonymous namespace

// static
X11Window* X11Window::getPointerFromHandle(Window handle)
{
  auto it = g_activeWindows.find(handle);
  if (it != g_activeWindows.end())
    return it->second;
  else
    return nullptr;
}

// static
void X11Window::addWindow(X11Window* window)
{
  ASSERT(g_activeWindows.find(window->handle()) == g_activeWindows.end());
  g_activeWindows[window->handle()] = window;
}

// static
void X11Window::removeWindow(X11Window* window)
{
  auto it = g_activeWindows.find(window->handle());
  ASSERT(it != g_activeWindows.end());
  if (it != g_activeWindows.end()) {
    ASSERT(it->second == window);
    g_activeWindows.erase(it);
  }
}

X11Window::X11Window(::Display* display, int width, int height, int scale)
  : m_display(display)
  , m_scale(scale)
  , m_clientSize(1, 1)
{
  ::Window root = XDefaultRootWindow(m_display);

  XSetWindowAttributes swa;
  swa.event_mask = (StructureNotifyMask | ExposureMask | PropertyChangeMask |
                    EnterWindowMask | LeaveWindowMask | FocusChangeMask |
                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                    KeyPressMask | KeyReleaseMask);

  m_window = XCreateWindow(
    m_display, root,
    0, 0, width, height, 0,
    CopyFromParent,
    InputOutput,
    CopyFromParent,
    CWEventMask,
    &swa);

  XMapWindow(m_display, m_window);

  m_gc = XCreateGC(m_display, m_window, 0, nullptr);

  X11Window::addWindow(this);
}

X11Window::~X11Window()
{
  XFreeGC(m_display, m_gc);
  XDestroyWindow(m_display, m_window);

  X11Window::removeWindow(this);
}

void X11Window::setScale(const int scale)
{
  m_scale = scale;
  resizeDisplay(m_clientSize);
}

void X11Window::setTitle(const std::string& title)
{
  XTextProperty prop;
  prop.value = (unsigned char*)title.c_str();
  prop.encoding = XA_STRING;
  prop.format = 8;
  prop.nitems = std::strlen((char*)title.c_str());
  XSetWMName(m_display, m_window, &prop);
}

void X11Window::captureMouse()
{
}

void X11Window::releaseMouse()
{
}

void X11Window::setMousePosition(const gfx::Point& position)
{
}

void X11Window::updateWindow(const gfx::Rect& unscaledBounds)
{
  XEvent ev;
  memset(&ev, 0, sizeof(ev));
  ev.xexpose.type = Expose;
  ev.xexpose.display = x11display();
  ev.xexpose.window = handle();
  ev.xexpose.x = unscaledBounds.x*m_scale;
  ev.xexpose.y = unscaledBounds.y*m_scale;
  ev.xexpose.width = unscaledBounds.w*m_scale;
  ev.xexpose.height = unscaledBounds.h*m_scale;
  XSendEvent(m_display, m_window, False,
             ExposureMask, (XEvent*)&ev);
}

bool X11Window::setNativeMouseCursor(NativeCursor cursor)
{
  return false;
}

bool X11Window::setNativeMouseCursor(const she::Surface* surface,
                                     const gfx::Point& focus,
                                     const int scale)
{
  return false;
}

void X11Window::processX11Event(XEvent& event)
{
  switch (event.type) {

    case ConfigureNotify: {
      gfx::Size newSize(event.xconfigure.width,
                        event.xconfigure.height);

      if (newSize.w > 0 &&
          newSize.h > 0 &&
          m_clientSize != newSize) {
        m_clientSize = newSize;
        resizeDisplay(m_clientSize);
      }
      break;
    }

    case Expose: {
      gfx::Rect rc(event.xexpose.x, event.xexpose.y,
                   event.xexpose.width, event.xexpose.height);
      paintGC(rc);
      break;
    }

    case ButtonPress: {
      Event ev;
      ev.setType(Event::MouseDown);
      ev.setModifiers(get_modifiers_from_xevent(event.xbutton.state));
      ev.setPosition(gfx::Point(event.xbutton.x / m_scale,
                                event.xbutton.y / m_scale));
      ev.setButton(
        event.xbutton.button == 1 ? Event::LeftButton:
        event.xbutton.button == 2 ? Event::MiddleButton:
        event.xbutton.button == 3 ? Event::RightButton:
        event.xbutton.button == 4 ? Event::X1Button:
        event.xbutton.button == 5 ? Event::X2Button: Event::NoneButton);

      queueEvent(ev);
      break;
    }

    case ButtonRelease: {
      Event ev;
      ev.setType(Event::MouseUp);
      ev.setModifiers(get_modifiers_from_xevent(event.xbutton.state));
      ev.setPosition(gfx::Point(event.xbutton.x / m_scale,
                                event.xbutton.y / m_scale));
      ev.setButton(
        event.xbutton.button == 1 ? Event::LeftButton:
        event.xbutton.button == 2 ? Event::MiddleButton:
        event.xbutton.button == 3 ? Event::RightButton:
        event.xbutton.button == 4 ? Event::X1Button:
        event.xbutton.button == 5 ? Event::X2Button: Event::NoneButton);

      queueEvent(ev);
      break;
    }

    case MotionNotify: {
      Event ev;
      ev.setType(Event::MouseMove);
      ev.setModifiers(get_modifiers_from_xevent(event.xmotion.state));
      ev.setPosition(gfx::Point(event.xmotion.x / m_scale,
                                event.xmotion.y / m_scale));
      queueEvent(ev);
      break;
    }

  }
}

} // namespace she
