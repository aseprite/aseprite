// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "gui/event.h"

namespace ui {

Event::Event(Component* source)
  : m_source(source)
{
}

Event::~Event()
{
}

Component* Event::getSource()
{
  return m_source;
}

} // namespace ui
