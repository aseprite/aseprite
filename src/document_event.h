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

#ifndef DOCUMENT_EVENT_H_INCLUDED
#define DOCUMENT_EVENT_H_INCLUDED

#include "gfx/region.h"
#include "raster/frame_number.h"

class Cel;
class Document;
class Image;
class Layer;
class LayerImage;
class Sprite;

class DocumentEvent {
public:
  DocumentEvent(Document* document,
                Sprite* sprite = NULL,
                Layer* layer = NULL,
                Cel* cel = NULL,
                Image* image = NULL,
                int imageIndex = -1,
                FrameNumber frame = FrameNumber(),
                const gfx::Region& region = gfx::Region())
    : m_document(document)
    , m_sprite(sprite)
    , m_layer(layer)
    , m_cel(cel)
    , m_image(image)
    , m_imageIndex(imageIndex)
    , m_frame(frame)
    , m_region(region) {
  }

  Document* document() const { return m_document; }
  Sprite* sprite() const { return m_sprite; }
  Layer* layer() const { return m_layer; }
  Cel* cel() const { return m_cel; }
  Image* image() const { return m_image; }
  int imageIndex() const { return m_imageIndex; }
  FrameNumber frame() const { return m_frame; }
  const gfx::Region& region() const { return m_region; }

private:
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  Cel* m_cel;
  Image* m_image;
  int m_imageIndex;
  FrameNumber m_frame;
  gfx::Region m_region;
};

#endif
