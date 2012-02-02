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

#ifndef CONTEXT_LISTENER_LIST_H_INCLUDED
#define CONTEXT_LISTENER_LIST_H_INCLUDED

#include <vector>

class Context;
class ContextListener;

class ContextListenerList
{
public:
  typedef std::vector<ContextListener*> list_type;
  typedef std::vector<ContextListener*>::iterator iterator;
  typedef std::vector<ContextListener*>::const_iterator const_iterator;

  ContextListenerList();

  void add(ContextListener* listener);
  void remove(ContextListener* listener);

  void notifyActiveDocumentBeforeChange(Context* context);
  void notifyActiveDocumentAfterChange(Context* context);
  void notifyCommandBeforeExecution(Context* context);
  void notifyCommandAfterExecution(Context* context);

private:
  list_type m_listener;
};

#endif
