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

#ifndef DOCUMENT_OBSERVER_H_INCLUDED
#define DOCUMENT_OBSERVER_H_INCLUDED

#include "raster/frame_number.h"

class DocumentEvent;

// Observer of document events. The default implementation does
// nothing in each handler, so you can override the required events.
class DocumentObserver {
public:
  virtual ~DocumentObserver() { }

  // General update. If an observer receives this event, it's because
  // anything in the document could be changed.
  virtual void onGeneralUpdate(DocumentEvent& ev) { }

  virtual void onAddSprite(DocumentEvent& ev) { }
  virtual void onAddLayer(DocumentEvent& ev) { }
  virtual void onAddFrame(DocumentEvent& ev) { }
  virtual void onAddCel(DocumentEvent& ev) { }

  virtual void onRemoveSprite(DocumentEvent& ev) { }
  virtual void onRemoveLayer(DocumentEvent& ev) { }
  virtual void onRemoveFrame(DocumentEvent& ev) { }
  virtual void onRemoveCel(DocumentEvent& ev) { }

  virtual void onSpriteSizeChanged(DocumentEvent& ev) { }

  virtual void onLayerRestacked(DocumentEvent& ev) { }

  virtual void onCelFrameChanged(DocumentEvent& ev) { }
  virtual void onCelPositionChanged(DocumentEvent& ev) { }
  virtual void onCelOpacityChanged(DocumentEvent& ev) { }

  virtual void onFrameDurationChanged(DocumentEvent& ev) { }

  virtual void onImageReplaced(DocumentEvent& ev) { }
  virtual void onImagePixelsModified(DocumentEvent& ev) { }
  virtual void onSpritePixelsModified(DocumentEvent& ev) { }

  // Called to destroy the observer. (Here you could call "delete this".)
  virtual void dispose() { }
};

#endif
