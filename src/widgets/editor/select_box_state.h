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

#ifndef WIDGETS_EDITOR_SELECT_BOX_STATE_H_INCLUDED
#define WIDGETS_EDITOR_SELECT_BOX_STATE_H_INCLUDED

#include "base/compiler_specific.h"
#include "widgets/editor/editor_decorator.h"
#include "widgets/editor/ruler.h"
#include "widgets/editor/standby_state.h"

#include <vector>

class SelectBoxDelegate
{
public:
  virtual ~SelectBoxDelegate() { }
  virtual void onChangeRectangle(const gfx::Rect& rect) = 0;
};

class SelectBoxState : public StandbyState
		     , public EditorDecorator
{
  enum { H1, H2, V1, V2 };

public:
  typedef int PaintFlags;
  static const int PaintRulers = 1;
  static const int PaintDarkOutside = 2;
  static const int PaintGrid = 4;

  SelectBoxState(SelectBoxDelegate* delegate,
		 const gfx::Rect& rc,
		 PaintFlags paintFlags);

  // Returns the bounding box arranged by the rulers.
  gfx::Rect getBoxBounds() const;
  void setBoxBounds(const gfx::Rect& rc);

  // EditorState overrides
  virtual void onAfterChangeState(Editor* editor) OVERRIDE;
  virtual void onBeforePopState(Editor* editor) OVERRIDE;
  virtual bool onMouseDown(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onMouseUp(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onMouseMove(Editor* editor, Message* msg) OVERRIDE;
  virtual bool onSetCursor(Editor* editor) OVERRIDE;

  // Returns false as it overrides default standby state behavior &
  // look. This state uses normal arrow cursors.
  virtual bool requirePenPreview() OVERRIDE { return false; }

  // EditorDecorator overrides
  virtual void preRenderDecorator(EditorPreRender* render) OVERRIDE;
  virtual void postRenderDecorator(EditorPostRender* render) OVERRIDE;

private:
  typedef std::vector<Ruler> Rulers;

  // Returns true if the position screen position (x, y) is touching
  // the given ruler.
  bool touchRuler(Editor* editor, Ruler& ruler, int x, int y);

  bool hasPaintFlag(PaintFlags flag) const;

  SelectBoxDelegate* m_delegate;
  Rulers m_rulers;
  int m_movingRuler;
  PaintFlags m_paintFlags;
};

#endif	// WIDGETS_EDITOR_SELECT_BOX_STATE_H_INCLUDED
