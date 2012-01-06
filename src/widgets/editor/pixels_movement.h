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

#ifndef WIDGETS_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#define WIDGETS_EDITOR_PIXELS_MOVEMENT_H_INCLUDED

#include "document_wrappers.h"
#include "undo_transaction.h"
#include "widgets/editor/handle_type.h"

class Document;
class Image;
class Sprite;

// Helper class to move pixels interactively and control undo history
// correctly.  The extra cel of the sprite is temporally used to show
// feedback, drag, and drop the specified image in the constructor
// (which generally would be the selected region or the clipboard
// content).
class PixelsMovement
{
public:
  // The "moveThis" image specifies the chunk of pixels to be moved.
  // The "x" and "y" parameters specify the initial position of the image.
  PixelsMovement(Document* document, Sprite* sprite, const Image* moveThis, int x, int y, int opacity,
                 const char* operationName);
  ~PixelsMovement();

  void cutMask();
  void copyMask();
  void catchImage(int x, int y, HandleType handle);
  void catchImageAgain(int x, int y, HandleType handle);

  // Creates a mask for the given image. Useful when the user paste a
  // image from the clipboard.
  void maskImage(const Image* image, int x, int y);

  // Moves the image to the new position (relative to the start
  // position given in the ctor). Returns the rectangle that should be
  // redrawn.
  gfx::Rect moveImage(int x, int y);

  void dropImageTemporarily();
  void dropImage();
  bool isDragging() const;

  gfx::Rect getImageBounds();

  void setMaskColor(uint32_t mask_color);

  const gfx::Transformation& getTransformation() const { return m_currentData; }

private:
  const DocumentReader m_documentReader;
  Sprite* m_sprite;
  UndoTransaction m_undoTransaction;
  bool m_firstDrop;
  bool m_isDragging;
  bool m_adjustPivot;
  HandleType m_handle;
  Image* m_originalImage;
  int m_catchX, m_catchY;
  gfx::Transformation m_initialData;
  gfx::Transformation m_currentData;
  Mask* m_initialMask;
  Mask* m_currentMask;
};

#endif
