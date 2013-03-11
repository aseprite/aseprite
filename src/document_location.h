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

#ifndef DOCUMENT_LOCATION_H_INCLUDED
#define DOCUMENT_LOCATION_H_INCLUDED

#include "raster/frame_number.h"
#include "raster/layer_index.h"

class Cel;
class Document;
class Image;
class Layer;
class Palette;
class Sprite;

// Specifies the current location in a context. If we are in the
// UIContext, it means the location in the current Editor (current
// document, sprite, layer, frame, etc.).
class DocumentLocation
{
public:
  DocumentLocation()
    : m_document(NULL)
    , m_sprite(NULL)
    , m_layer(NULL)
    , m_frame(0) { }

  const Document* document() const { return m_document; }
  const Sprite* sprite() const { return m_sprite; }
  const Layer* layer() const { return m_layer; }
  FrameNumber frame() const { return m_frame; }
  const Cel* cel() const;

  Document* document() { return m_document; }
  Sprite* sprite() { return m_sprite; }
  Layer* layer() { return m_layer; }
  Cel* cel();

  void document(Document* document) { m_document = document; }
  void sprite(Sprite* sprite) { m_sprite = sprite; }
  void layer(Layer* layer) { m_layer = layer; }
  void frame(FrameNumber frame) { m_frame = frame; }

  LayerIndex layerIndex() const;
  void layerIndex(LayerIndex layerIndex);
  Palette* palette();
  Image* image(int* x = NULL, int* y = NULL, int* opacity = NULL) const;

private:
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  FrameNumber m_frame;
};

#endif
