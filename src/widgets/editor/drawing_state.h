/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef WIDGETS_EDITOR_DRAWING_STATE_H_INCLUDED
#define WIDGETS_EDITOR_DRAWING_STATE_H_INCLUDED

#include "base/compiler_specific.h"
#include "widgets/editor/standby_state.h"

namespace tools {
  class ToolLoop;
  class ToolLoopManager;
}

class DrawingState : public StandbyState
{
public:
  DrawingState(tools::ToolLoop* loop, Editor* editor, Message* msg);
  virtual ~DrawingState();
  virtual bool onMouseDown(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onMouseUp(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onMouseMove(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onSetCursor(Editor* editor) OVERRIDE;
  virtual bool onKeyDown(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onKeyUp(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onUpdateStatusBar(Editor* editor) OVERRIDE;

  // Drawing state doesn't require the pen-preview because we are
  // already drawing (viewing the real trace).
  virtual bool requirePenPreview() OVERRIDE { return false; }

private:
  // The tool-loop.
  tools::ToolLoop* m_toolLoop;

  // Tool-loop manager
  tools::ToolLoopManager* m_toolLoopManager;
};

#endif  // WIDGETS_EDITOR_DRAWING_STATE_H_INCLUDED
