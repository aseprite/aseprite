/* Aseprite
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

#ifndef APP_DOCUMENT_EVENT_H_INCLUDED
#define APP_DOCUMENT_EVENT_H_INCLUDED
#pragma once

#include "gfx/region.h"
#include "raster/frame_number.h"

namespace raster {
  class Cel;
  class Image;
  class Layer;
  class LayerImage;
  class Sprite;
}

namespace app {
  class Document;

  using namespace raster;

  class DocumentEvent {
  public:
    DocumentEvent(Document* document)
      : m_document(document)
      , m_sprite(NULL)
      , m_layer(NULL)
      , m_cel(NULL)
      , m_image(NULL)
      , m_imageIndex(-1)
      , m_frame(0)
      , m_targetLayer(NULL)
      , m_targetFrame(0) {
    }

    // Source of the event.
    Document* document() const { return m_document; }
    Sprite* sprite() const { return m_sprite; }
    Layer* layer() const { return m_layer; }
    Cel* cel() const { return m_cel; }
    Image* image() const { return m_image; }
    int imageIndex() const { return m_imageIndex; }
    FrameNumber frame() const { return m_frame; }
    const gfx::Region& region() const { return m_region; }

    void sprite(Sprite* sprite) { m_sprite = sprite; }
    void layer(Layer* layer) { m_layer = layer; }
    void cel(Cel* cel) { m_cel = cel; }
    void image(Image* image) { m_image = image; }
    void imageIndex(int imageIndex) { m_imageIndex = imageIndex; }
    void frame(FrameNumber frame) { m_frame = frame; }
    void region(const gfx::Region& rgn) { m_region = rgn; }

    // Destination of the operation.
    Layer* targetLayer() const { return m_targetLayer; }
    FrameNumber targetFrame() const { return m_targetFrame; }

    void targetLayer(Layer* layer) { m_targetLayer = layer; }
    void targetFrame(FrameNumber frame) { m_targetFrame = frame; }

  private:
    Document* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    Cel* m_cel;
    Image* m_image;
    int m_imageIndex;
    FrameNumber m_frame;
    gfx::Region m_region;

    // For copy/move commands, the m_layer/m_frame are source of the
    // operation, and these are the destination of the operation.
    Layer* m_targetLayer;
    FrameNumber m_targetFrame;
  };

} // namespace app

#endif
