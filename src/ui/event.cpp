// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/event.h"

namespace ui {

Event::Event(Component* source) : m_source(source)
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
