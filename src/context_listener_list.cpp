/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "context_listener_list.h"

#include "context_listener.h"

#include <algorithm>
#include <functional>

ContextListenerList::ContextListenerList()
{
}

void ContextListenerList::add(ContextListener* listener)
{
  m_listener.push_back(listener);
}

void ContextListenerList::remove(ContextListener* listener)
{
  iterator it = std::find(m_listener.begin(), m_listener.end(), listener);
  ASSERT(it != m_listener.end());
  m_listener.erase(it);
}

void ContextListenerList::notifyActiveDocumentBeforeChange(Context* context)
{
  list_type copy = m_listener;
  std::for_each(copy.begin(), copy.end(),
                std::bind2nd(std::mem_fun(&ContextListener::onActiveDocumentBeforeChange), context));
}

void ContextListenerList::notifyActiveDocumentAfterChange(Context* context)
{
  list_type copy = m_listener;
  std::for_each(copy.begin(), copy.end(),
                std::bind2nd(std::mem_fun(&ContextListener::onActiveDocumentAfterChange), context));
}

void ContextListenerList::notifyCommandBeforeExecution(Context* context)
{
  list_type copy = m_listener;
  std::for_each(copy.begin(), copy.end(),
                std::bind2nd(std::mem_fun(&ContextListener::onCommandBeforeExecution), context));
}

void ContextListenerList::notifyCommandAfterExecution(Context* context)
{
  list_type copy = m_listener;
  std::for_each(copy.begin(), copy.end(),
                std::bind2nd(std::mem_fun(&ContextListener::onCommandAfterExecution), context));
}
