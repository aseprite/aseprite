// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SPRITE_H_INCLUDED
#define DOC_SPRITE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/cel_data.h"
#include "doc/cel_list.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/frame_tags.h"
#include "doc/image_ref.h"
#include "doc/image_spec.h"
#include "doc/layer_index.h"
#include "doc/object.h"
#include "doc/pixel_format.h"
#include "doc/pixel_ratio.h"
#include "doc/sprite_position.h"
#include "gfx/rect.h"

#include <vector>

namespace doc {

  class CelsRange;
  class Document;
  class Image;
  class Layer;
  class LayerGroup;
  class LayerImage;
  class Mask;
  class Palette;
  class Remap;
  class RgbMap;

  typedef std::vector<Palette*> PalettesList;

  // The main structure used in the whole program to handle a sprite.
  class Sprite : public Object {
  public:
    enum class RgbMapFor {
      OpaqueLayer,
      TransparentLayer
    };

    ////////////////////////////////////////
    // Constructors/Destructor

    Sprite(PixelFormat format, int width, int height, int ncolors);
    Sprite(const ImageSpec& spec, int ncolors);
    virtual ~Sprite();

    static Sprite* createBasicSprite(PixelFormat format, int width, int height, int ncolors);

    ////////////////////////////////////////
    // Main properties

    const ImageSpec& spec() const { return m_spec; }

    Document* document() { return m_document; }
    void setDocument(Document* doc) { m_document = doc; }

    PixelFormat pixelFormat() const { return (PixelFormat)m_spec.colorMode(); }
    const PixelRatio& pixelRatio() const { return m_pixelRatio; }
    gfx::Rect bounds() const { return m_spec.bounds(); }
    int width() const { return m_spec.width(); }
    int height() const { return m_spec.height(); }

    void setPixelFormat(PixelFormat format);
    void setPixelRatio(const PixelRatio& pixelRatio);
    void setSize(int width, int height);

    // Returns true if the rendered images will contain alpha values less
    // than 255. Only RGBA and Grayscale images without background needs
    // alpha channel in the render.
    bool needAlpha() const;
    bool supportAlpha() const;

    color_t transparentColor() const { return m_spec.maskColor(); }
    void setTransparentColor(color_t color);

    virtual int getMemSize() const override;

    ////////////////////////////////////////
    // Layers

    LayerGroup* root() const { return m_root; }
    LayerImage* backgroundLayer() const;

    LayerIndex countLayers() const;
    LayerIndex firstLayer() const;
    Layer* firstBrowsableLayer() const;
    LayerIndex lastLayer() const;

    Layer* layer(int layerIndex) const;
    Layer* indexToLayer(LayerIndex index) const;
    LayerIndex layerToIndex(const Layer* layer) const;

    ////////////////////////////////////////
    // Palettes

    Palette* palette(frame_t frame) const;
    const PalettesList& getPalettes() const;

    void setPalette(const Palette* pal, bool truncate);

    // Removes all palettes from the sprites except the first one.
    void resetPalettes();

    void deletePalette(frame_t frame);

    RgbMap* rgbMap(frame_t frame) const;
    RgbMap* rgbMap(frame_t frame, RgbMapFor forLayer) const;

    ////////////////////////////////////////
    // Frames

    frame_t totalFrames() const { return m_frames; }
    frame_t lastFrame() const { return m_frames-1; }

    void addFrame(frame_t newFrame);
    void removeFrame(frame_t frame);
    void setTotalFrames(frame_t frames);

    int frameDuration(frame_t frame) const;
    void setFrameDuration(frame_t frame, int msecs);
    void setFrameRangeDuration(frame_t from, frame_t to, int msecs);
    void setDurationForAllFrames(int msecs);

    const FrameTags& frameTags() const { return m_frameTags; }
    FrameTags& frameTags() { return m_frameTags; }

    ////////////////////////////////////////
    // Shared Images and CelData (for linked Cels)

    ImageRef getImageRef(ObjectId imageId);
    CelDataRef getCelDataRef(ObjectId celDataId);

    ////////////////////////////////////////
    // Images

    void replaceImage(ObjectId curImageId, const ImageRef& newImage);
    void getImages(std::vector<Image*>& images) const;
    void remapImages(frame_t frameFrom, frame_t frameTo, const Remap& remap);
    void pickCels(int x, int y, frame_t frame, int opacityThreshold, CelList& cels) const;

    ////////////////////////////////////////
    // Iterators

    LayerList allLayers() const;
    LayerList allVisibleLayers() const;
    LayerList allBrowsableLayers() const;

    CelsRange cels() const;
    CelsRange cels(frame_t frame) const;
    CelsRange uniqueCels() const;
    CelsRange uniqueCels(frame_t from, frame_t to) const;

  private:
    Document* m_document;
    ImageSpec m_spec;
    PixelRatio m_pixelRatio;
    frame_t m_frames;                      // how many frames has this sprite
    std::vector<int> m_frlens;             // duration per frame
    PalettesList m_palettes;               // list of palettes
    LayerGroup* m_root;                    // main group of layers

    // Current rgb map
    mutable RgbMap* m_rgbMap;

    FrameTags m_frameTags;

    // Disable default constructor and copying
    Sprite();
    DISABLE_COPYING(Sprite);
  };

} // namespace doc

#endif
