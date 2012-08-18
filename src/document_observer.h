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

#ifndef DOCUMENT_OBSERVER_H_INCLUDED
#define DOCUMENT_OBSERVER_H_INCLUDED

#include "raster/frame_number.h"

class Document;
class Layer;
class Sprite;

// Observer of document events. The default implementation does
// nothing in each handler, so you can override the required events.
class DocumentObserver {
public:
  virtual ~DocumentObserver() { }

  virtual void onAddSprite(Document* document, Sprite* sprite) { }

  virtual void onAddLayer(Document* document, Layer* layer) { }

  virtual void onAddFrame(Document* document, FrameNumber frame) { }

  virtual void onRemoveSprite(Document* document, Sprite* sprite) { }

  virtual void onRemoveLayer(Document* document, Layer* layer) { }

  virtual void onRemoveFrame(Document* document, FrameNumber frame) { }

  // Called when the active cel data was modified (position, opacity, etc.).
  virtual void onModifyCel(Document* document) { }

  // Called when the active cel's image pixels were modified.
  virtual void onModifyCelImage(Document* document) { }

  // Called to destroy the observer. (Here you could call "delete this".)
  virtual void dispose() { }
};

#endif
