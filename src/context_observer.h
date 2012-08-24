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

#ifndef CONTEXT_OBSERVER_H_INCLUDED
#define CONTEXT_OBSERVER_H_INCLUDED

class Context;
class Document;

// Observer of context events. The default implementation does nothing
// in each handler, so you can override the required events.
class ContextObserver
{
public:
  virtual ~ContextObserver() { }
  virtual void onActiveDocumentBeforeChange(Context* context) { }
  virtual void onActiveDocumentAfterChange(Context* context) { }
  virtual void onCommandBeforeExecution(Context* context) { }
  virtual void onCommandAfterExecution(Context* context) { }
  virtual void onAddDocument(Context* context, Document* document) { }
  virtual void onRemoveDocument(Context* context, Document* document) { }
};

#endif
