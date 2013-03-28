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

#ifndef UTIL_EXPAND_CEL_CANVAS_H_INCLUDED
#define UTIL_EXPAND_CEL_CANVAS_H_INCLUDED

#include "filters/tiled_mode.h"

class Cel;
class Context;
class Document;
class Image;
class Layer;
class Sprite;
class UndoTransaction;

// This class can be used to expand the canvas of the current cel to
// the visible portion of sprite. If the user cancels the operation,
// you've a rollback() method to restore the cel to its original
// state.  If all changes are committed, some undo information is
// stored in the document's UndoHistory to go back to the original
// state using "Undo" command.
class ExpandCelCanvas
{
public:
  ExpandCelCanvas(Context* context, TiledMode tiledMode, UndoTransaction& undo);
  ~ExpandCelCanvas();

  // Commit changes made in getDestCanvas() in the cel's image. Adds
  // information in the undo history so the user can undo the
  // modifications in the canvas.
  void commit();

  // Restore the cel as its original state as when ExpandCelCanvas()
  // was created.
  void rollback();

  // You can read pixels from here
  Image* getSourceCanvas() {    // TODO this should be "const"
    return m_srcImage;
  }

  // You can write pixels right here
  Image* getDestCanvas() {
    return m_dstImage;
  }

  const Cel* getCel() const {
    return m_cel;
  }

private:
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  Cel* m_cel;
  Image* m_celImage;
  bool m_celCreated;
  int m_originalCelX;
  int m_originalCelY;
  Image* m_srcImage;
  Image* m_dstImage;
  bool m_closed;
  bool m_committed;
  UndoTransaction& m_undo;
};

#endif
