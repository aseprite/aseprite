// Aseprite Document Library
// Copyright (c) 2019-2023 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CEL_H_INCLUDED
#define DOC_CEL_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/cel_data.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/object.h"
#include "gfx/fwd.h"
#include "gfx/point.h"

namespace doc {

  class Document;
  class Grid;
  class LayerImage;
  class Sprite;

  class Cel : public Object {
  public:
    Cel(frame_t frame, const ImageRef& image);
    Cel(frame_t frame, const CelDataRef& celData);

    static Cel* MakeCopy(const frame_t newFrame, const Cel* other);
    static Cel* MakeLink(const frame_t newFrame, const Cel* other);

    frame_t frame() const { return m_frame; }
    int x() const { return m_data->position().x; }
    int y() const { return m_data->position().y; }
    gfx::Point position() const { return m_data->position(); }
    const gfx::Rect& bounds() const { return m_data->bounds(); }
    const gfx::RectF& boundsF() const { return m_data->boundsF(); }
    int opacity() const { return m_data->opacity(); }
    int zIndex() const { return m_zIndex; }

    gfx::Rect imageBounds() const { return m_data->imageBounds(); }

    LayerImage* layer() const { return m_layer; }
    Image* image() const { return m_data->image(); }
    ImageRef imageRef() const { return m_data->imageRef(); }
    CelData* data() const { return const_cast<CelData*>(m_data.get()); }
    CelDataRef dataRef() const { return m_data; }
    Document* document() const;
    Sprite* sprite() const;
    Cel* link() const;
    std::size_t links() const;

    // You should change the frame only if the cel isn't member of a
    // layer. If the cel is already in a layer, you should use
    // LayerImage::moveCel() member function.
    void setFrame(frame_t frame);
    void setDataRef(const CelDataRef& celData);
    void setPosition(int x, int y);
    void setPosition(const gfx::Point& pos);
    void setBounds(const gfx::Rect& bounds);
    void setBoundsF(const gfx::RectF& bounds);
    void setOpacity(int opacity);
    void setZIndex(int zindex);

    virtual int getMemSize() const override {
      return sizeof(Cel) + m_data->getMemSize();
    }

    void setParentLayer(LayerImage* layer);
    Grid grid() const;

  private:
    void fixupImage();

    LayerImage* m_layer;
    frame_t m_frame;            // Frame position
    CelDataRef m_data;
    int m_zIndex = 0;

    Cel();
    DISABLE_COPYING(Cel);
  };

} // namespace doc

#endif
