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

#ifndef WIDGETS_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#define WIDGETS_EDITOR_PIXELS_MOVEMENT_H_INCLUDED

#include "document_wrappers.h"
#include "undo_transaction.h"

class Document;
class Image;
class Sprite;

// Uses the extra cel of the sprite to move/paste the specified image.
class PixelsMovement
{
public:
  PixelsMovement(Document* document, Sprite* sprite, const Image* moveThis, int x, int y, int opacity);
  ~PixelsMovement();

  void cutMask();
  void copyMask();
  void catchImage(int x, int y);
  void catchImageAgain(int x, int y);

  // Moves the image to the new position (relative to the start
  // position given in the ctor). Returns the rectangle that should be
  // redrawn.
  gfx::Rect moveImage(int x, int y);

  void dropImageTemporarily();
  void dropImage();
  bool isDragging();

  gfx::Rect getImageBounds();

  void setMaskColor(uint32_t mask_color);

private:
  const DocumentReader m_documentReader;
  Sprite* m_sprite;
  UndoTransaction m_undoTransaction;
  int m_initial_x, m_initial_y;
  int m_catch_x, m_catch_y;
  bool m_firstDrop;
  bool m_isDragging;
};

#endif
