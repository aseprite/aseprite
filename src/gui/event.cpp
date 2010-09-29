// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "gui/event.h"

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
