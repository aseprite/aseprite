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

#ifndef WIDGETS_EDITOR_EDITOR_DECORATOR_H_INCLUDED
#define WIDGETS_EDITOR_EDITOR_DECORATOR_H_INCLUDED

#include "gfx/rect.h"

class Editor;
class EditorDecorator;
class Graphics;
class Image;

// EditorPreRender and EditorPostRender are two interfaces used to
// draw elements in the editor's area. They are implemented by the
// editor and used by a EditorDecorator.
class EditorPreRender
{
public:
  virtual ~EditorPreRender() { }
  virtual Editor* getEditor() = 0;
  virtual Image* getImage() = 0;
  virtual void fillRect(const gfx::Rect& rect, uint32_t rgbaColor, int opacity) = 0;
};

class EditorPostRender
{
public:
  virtual ~EditorPostRender() { }
  virtual Editor* getEditor() = 0;
  virtual void drawLine(int x1, int y1, int x2, int y2, int screenColor) = 0;
};

// Used by editor's states to pre- and post-render customized
// decorations depending of the state (e.g. SelectBoxState draws the
// selected bounds/canvas area).
class EditorDecorator
{
public:
  virtual ~EditorDecorator() { }
  virtual void preRenderDecorator(EditorPreRender* render) = 0;
  virtual void postRenderDecorator(EditorPostRender* render) = 0;
};

#endif  // WIDGETS_EDITOR_EDITOR_DECORATOR_H_INCLUDED
