// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CEL_H_INCLUDED
#define DOC_CEL_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/object.h"
#include "gfx/fwd.h"
#include "gfx/point.h"

namespace doc {

  class LayerImage;
  class Sprite;

  class Cel : public Object {
  public:
    Cel(frame_t frame, const ImageRef& image);
    Cel(const Cel& cel);
    virtual ~Cel();

    frame_t frame() const { return m_frame; }
    int x() const { return m_position.x; }
    int y() const { return m_position.y; }
    gfx::Point position() const { return m_position; }
    int opacity() const { return m_opacity; }

    LayerImage* layer() const { return m_layer; }
    Image* image() const { return const_cast<Image*>(m_image.get()); };
    ImageRef imageRef() const { return m_image; }
    Sprite* sprite() const;
    Cel* link() const;
    gfx::Rect bounds() const;

    // You should change the frame only if the cel isn't member of a
    // layer.  If the cel is already in a layer, you should use
    // LayerImage::moveCel() member function.
    void setFrame(frame_t frame);
    void setImage(const ImageRef& image);
    void setPosition(int x, int y) {
      m_position.x = x;
      m_position.y = y;
    }
    void setPosition(const gfx::Point& pos) { m_position = pos; }
    void setOpacity(int opacity) { m_opacity = opacity; }

    virtual int getMemSize() const override {
      return sizeof(Cel);
    }

    void setParentLayer(LayerImage* layer);

  private:
    void fixupImage();

    LayerImage* m_layer;
    frame_t m_frame;            // Frame position
    ImageRef m_image;
    gfx::Point m_position;      // X/Y screen position
    int m_opacity;              // Opacity level
  };

} // namespace doc

#endif
