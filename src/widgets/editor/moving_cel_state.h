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

#ifndef WIDGETS_EDITOR_MOVING_CEL_STATE_H_INCLUDED
#define WIDGETS_EDITOR_MOVING_CEL_STATE_H_INCLUDED

#include "base/compiler_specific.h"
#include "widgets/editor/standby_state.h"

class Cel;
class Editor;

class MovingCelState : public StandbyState
{
public:
  MovingCelState(Editor* editor, ui::Message* msg);
  virtual ~MovingCelState();

  virtual bool onMouseUp(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onMouseMove(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onUpdateStatusBar(Editor* editor) OVERRIDE;

private:
  Cel* m_cel;
  int m_celStartX;
  int m_celStartY;
  int m_mouseStartX;
  int m_mouseStartY;
  int m_celNewX;
  int m_celNewY;
  bool m_canceled;
  bool m_maskVisible;
};

#endif  // WIDGETS_EDITOR_MOVING_CEL_STATE_H_INCLUDED
