/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "app.h"		// TODO avoid to include this file
#include "raster/undo.h"

#include "tools/ink_processing.h"


// Ink used for tools which paint with primary/secondary
// (or foreground/background colors)
class PaintInk : public ToolInk
{
public:
  enum Type { Normal, WithFg, WithBg };

private:
  AlgoHLine m_proc;
  Type m_type;

public:
  PaintInk(Type type) : m_type(type) { }

  bool isPaint() const { return true; }

  void prepareInk(IToolLoop* loop)
  {
    switch (m_type) {

      case Normal:
	// Do nothing, use the default colors
	break;

      case WithFg:
      case WithBg:
	{
	  int color = get_color_for_layer(loop->getLayer(),
					  m_type == WithFg ? 
					    loop->getContext()->getSettings()->getFgColor():
					    loop->getContext()->getSettings()->getBgColor());
	  loop->setPrimaryColor(color);
	  loop->setSecondaryColor(color);
	}
	break;
    }

    m_proc = loop->getOpacity() == 255 ?
      ink_processing[INK_OPAQUE][MID(0, loop->getSprite()->getImgType(), 2)]:
      ink_processing[INK_TRANSPARENT][MID(0, loop->getSprite()->getImgType(), 2)];
  }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


class PickInk : public ToolInk
{
public:
  enum Target { Fg, Bg };

private:
  Target m_target;

public:
  PickInk(Target target) : m_target(target) { }

  bool isEyedropper() const { return true; }

  void prepareInk(IToolLoop* loop)
  {
    // Do nothing
  }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    // Do nothing
  }

};


class ScrollInk : public ToolInk
{
public:

  bool isScrollMovement() const { return true; }

  void prepareInk(IToolLoop* loop)
  {
    // Do nothing
  }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    // Do nothing
  }

};


class MoveInk : public ToolInk
{
public:
  bool isCelMovement() const { return true; }

  void prepareInk(IToolLoop* loop)
  {
    // Do nothing
  }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    // Do nothing
  }
};


class EraserInk : public ToolInk
{
public:
  enum Type { Eraser, ReplaceFgWithBg, ReplaceBgWithFg };

private:
  AlgoHLine m_proc;
  Type m_type;

public:
  EraserInk(Type type) : m_type(type) { }

  bool isPaint() const { return true; }
  bool isEffect() const { return true; }

  void prepareInk(IToolLoop* loop)
  {
    switch (m_type) {

      case Eraser:
	m_proc = ink_processing[INK_OPAQUE][MID(0, loop->getSprite()->getImgType(), 2)];

	// TODO app_get_color_to_clear_layer should receive the context as parameter
	loop->setPrimaryColor(app_get_color_to_clear_layer(loop->getLayer()));
	loop->setSecondaryColor(app_get_color_to_clear_layer(loop->getLayer()));
	break;

      case ReplaceFgWithBg:
	m_proc = ink_processing[INK_REPLACE][MID(0, loop->getSprite()->getImgType(), 2)];

	loop->setPrimaryColor(get_color_for_layer(loop->getLayer(),
						  loop->getContext()->getSettings()->getFgColor()));
	loop->setSecondaryColor(get_color_for_layer(loop->getLayer(),
						    loop->getContext()->getSettings()->getBgColor()));
	break;

      case ReplaceBgWithFg:
	m_proc = ink_processing[INK_REPLACE][MID(0, loop->getSprite()->getImgType(), 2)];

	loop->setPrimaryColor(get_color_for_layer(loop->getLayer(),
						  loop->getContext()->getSettings()->getBgColor()));
	loop->setSecondaryColor(get_color_for_layer(loop->getLayer(),
						    loop->getContext()->getSettings()->getFgColor()));
	break;
    }
  }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


class BlurInk : public ToolInk
{
  AlgoHLine m_proc;

public:
  bool isPaint() const { return true; }
  bool isEffect() const { return true; }

  void prepareInk(IToolLoop* loop)
  {
    m_proc = ink_processing[INK_BLUR][MID(0, loop->getSprite()->getImgType(), 2)];
  }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


class JumbleInk : public ToolInk
{
  AlgoHLine m_proc;

public:
  bool isPaint() const { return true; }
  bool isEffect() const { return true; }

  void prepareInk(IToolLoop* loop)
  {
    m_proc = ink_processing[INK_JUMBLE][MID(0, loop->getSprite()->getImgType(), 2)];
  }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


// Ink used for selection tools (like Rectangle Marquee, Lasso, Magic Wand, etc.)
class SelectionInk : public ToolInk
{
  bool m_modify_selection;

public:
  SelectionInk() { m_modify_selection = false; }
  
  bool isSelection() const { return true; }

  void inkHline(int x1, int y, int x2, IToolLoop* loop)
  {
    if (m_modify_selection) {
      Point offset = loop->getOffset();

      if (loop->getMouseButton() == 0)
	loop->getMask()->add(x1-offset.x, y-offset.y, x2-x1+1, 1);
      else if (loop->getMouseButton() == 1)
	mask_subtract(loop->getMask(), x1-offset.x, y-offset.y, x2-x1+1, 1);
    }
    else
      image_hline(loop->getDstImage(), x1, y, x2, loop->getPrimaryColor());
  }

  void setFinalStep(IToolLoop* loop, bool state)
  {
    m_modify_selection = state;

    if (state) {
      if (undo_is_enabled(loop->getSprite()->getUndo()))
	undo_set_mask(loop->getSprite()->getUndo(), loop->getSprite());

      loop->getMask()->freeze();
      loop->getMask()->reserve(0, 0, loop->getSprite()->getWidth(), loop->getSprite()->getHeight());
    }
    else {
      loop->getMask()->unfreeze();
    }
  }

};
