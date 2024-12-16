// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CLOSE_EVENT_H_INCLUDED
#define UI_CLOSE_EVENT_H_INCLUDED
#pragma once

#include "ui/event.h"

namespace ui {

class CloseEvent : public Event {
public:
  CloseEvent(Component* source) : Event(source), m_canceled(false) {}
  void cancel() { m_canceled = true; }
  bool canceled() const { return m_canceled; }

private:
  bool m_canceled;
};

} // namespace ui

#endif // UI_CLOSE_EVENT_H_INCLUDED
