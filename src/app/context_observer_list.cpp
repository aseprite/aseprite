/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context_observer_list.h"

#include "app/context_observer.h"

#include <algorithm>
#include <functional>

namespace app {

ContextObserverList::ContextObserverList()
{
}

void ContextObserverList::addObserver(ContextObserver* observer)
{
  m_observer.push_back(observer);
}

void ContextObserverList::removeObserver(ContextObserver* observer)
{
  iterator it = std::find(m_observer.begin(), m_observer.end(), observer);
  ASSERT(it != m_observer.end());
  m_observer.erase(it);
}

void ContextObserverList::notifyCommandBeforeExecution(Context* context)
{
  list_type copy = m_observer;
  std::for_each(copy.begin(), copy.end(),
                std::bind2nd(std::mem_fun(&ContextObserver::onCommandBeforeExecution), context));
}

void ContextObserverList::notifyCommandAfterExecution(Context* context)
{
  list_type copy = m_observer;
  std::for_each(copy.begin(), copy.end(),
                std::bind2nd(std::mem_fun(&ContextObserver::onCommandAfterExecution), context));
}

void ContextObserverList::notifyAddDocument(Context* context, Document* document)
{
  list_type copy = m_observer;
  for (iterator it=copy.begin(), end=copy.end(); it!=end; ++it)
    (*it)->onAddDocument(context, document);
}

void ContextObserverList::notifyRemoveDocument(Context* context, Document* document)
{
  list_type copy = m_observer;
  for (iterator it=copy.begin(), end=copy.end(); it!=end; ++it)
    (*it)->onRemoveDocument(context, document);
}

} // namespace app
