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

#ifndef CONTEXT_OBSERVER_LIST_H_INCLUDED
#define CONTEXT_OBSERVER_LIST_H_INCLUDED

#include <vector>

class Context;
class ContextObserver;
class Document;

class ContextObserverList
{
public:
  typedef std::vector<ContextObserver*> list_type;
  typedef std::vector<ContextObserver*>::iterator iterator;
  typedef std::vector<ContextObserver*>::const_iterator const_iterator;

  ContextObserverList();

  void addObserver(ContextObserver* observer);
  void removeObserver(ContextObserver* observer);

  void notifyCommandBeforeExecution(Context* context);
  void notifyCommandAfterExecution(Context* context);
  void notifyAddDocument(Context* context, Document* document);
  void notifyRemoveDocument(Context* context, Document* document);

private:
  list_type m_observer;
};

#endif
