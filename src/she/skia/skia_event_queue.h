// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

namespace she {

class SkiaEventQueue : public EventQueue {
public:
  SkiaEventQueue() : m_stop(false) {
  }

  void getEvent(Event& ev, bool canWait) override {
    MSG msg;

    while (!m_stop && m_events.empty()) {
      BOOL res;

      if (canWait) {
        res = GetMessage(&msg, nullptr, 0, 0);
      }
      else {
        res = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
      }

      if (res) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      else if (!canWait)
        break;
    }

    if (m_events.empty()) {
      ev.setType(Event::None);
    }
    else {
      ev = m_events.front();
      m_events.pop();
    }
  }

  void queueEvent(Event& ev) {
    if (ev.type() == Event::CloseDisplay)
      m_stop = true;

    m_events.push(ev);
  }

private:
  std::queue<Event> m_events;
  bool m_stop;
};

} // namespace she
