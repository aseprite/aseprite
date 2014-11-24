// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CEL_H_INCLUDED
#define DOC_CEL_H_INCLUDED
#pragma once

#include "doc/frame_number.h"
#include "doc/object.h"
#include "gfx/fwd.h"
#include "gfx/point.h"

namespace doc {

  class Image;
  class LayerImage;
  class Sprite;

  class Cel : public Object {
  public:
    Cel(FrameNumber frame, int image);
    Cel(const Cel& cel);
    virtual ~Cel();

    FrameNumber frame() const { return m_frame; }
    int imageIndex() const { return m_image; }
    int x() const { return m_position.x; }
    int y() const { return m_position.y; }
    gfx::Point position() const { return m_position; }
    int opacity() const { return m_opacity; }

    LayerImage* layer() const { return m_layer; }
    Image* image() const;
    Sprite* sprite() const;
    gfx::Rect bounds() const;

    // You should change the frame only if the cel isn't member of a
    // layer.  If the cel is already in a layer, you should use
    // LayerImage::moveCel() member function.
    void setFrame(FrameNumber frame) { m_frame = frame; }
    void setImage(int image) { m_image = image; }
    void setPosition(int x, int y) {
      m_position.x = x;
      m_position.y = y;
    }
    void setPosition(const gfx::Point& pos) { m_position = pos; }
    void setOpacity(int opacity) { m_opacity = opacity; }

    virtual int getMemSize() const override {
      return sizeof(Cel);
    }

    void setParentLayer(LayerImage* layer) {
      m_layer = layer;
    }

  private:
    LayerImage* m_layer;
    FrameNumber m_frame;          // Frame position
    int m_image;                  // Image index of stock
    gfx::Point m_position;        // X/Y screen position
    int m_opacity;                // Opacity level
  };

} // namespace doc

#endif
