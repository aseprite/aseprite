// Aseprite Document Library
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
#include "doc/image_buffer.h"
#include "doc/image_ref.h"
#include "doc/image_spec.h"
#include "doc/layer_list.h"
#include "doc/object.h"
#include "doc/pixel_format.h"
#include "doc/pixel_ratio.h"
#include "doc/slices.h"
#include "doc/tags.h"
#include "gfx/rect.h"

#include <vector>

#define DOC_SPRITE_MAX_WIDTH  65535
#define DOC_SPRITE_MAX_HEIGHT 65535

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
  class SelectedFrames;

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

    Sprite(const ImageSpec& spec, int ncolors = 256);
    virtual ~Sprite();

    // Creates a new sprite with one transparent layer and one cel
    // with an image of the size of the sprite.
    static Sprite* MakeStdSprite(const ImageSpec& spec,
                                 const int ncolors = 256,
                                 const ImageBufferPtr& imageBuf = ImageBufferPtr());

    ////////////////////////////////////////
    // Main properties

    const ImageSpec& spec() const { return m_spec; }

    Document* document() const { return m_document; }
    void setDocument(Document* doc) { m_document = doc; }

    PixelFormat pixelFormat() const { return (PixelFormat)m_spec.colorMode(); }
    ColorMode colorMode() const { return m_spec.colorMode(); }
    const PixelRatio& pixelRatio() const { return m_pixelRatio; }
    gfx::Size size() const { return m_spec.size(); }
    gfx::Rect bounds() const { return m_spec.bounds(); }
    int width() const { return m_spec.width(); }
    int height() const { return m_spec.height(); }
    const gfx::ColorSpacePtr& colorSpace() const { return m_spec.colorSpace(); }

    void setPixelFormat(PixelFormat format);
    void setPixelRatio(const PixelRatio& pixelRatio);
    void setSize(int width, int height);
    void setColorSpace(const gfx::ColorSpacePtr& colorSpace);

    // Returns true if the sprite has a background layer and it's visible
    bool isOpaque() const;

    // Returns true if the rendered images will contain alpha values less
    // than 255. Only RGBA and Grayscale images without background needs
    // alpha channel in the render.
    bool needAlpha() const;
    bool supportAlpha() const;

    color_t transparentColor() const { return m_spec.maskColor(); }
    void setTransparentColor(color_t color);

    static gfx::Rect DefaultGridBounds();
    static void SetDefaultGridBounds(const gfx::Rect& defGridBounds);

    const gfx::Rect& gridBounds() const { return m_gridBounds; }
    void setGridBounds(const gfx::Rect& rc) { m_gridBounds = rc; }

    virtual int getMemSize() const override;

    ////////////////////////////////////////
    // Layers

    LayerGroup* root() const { return m_root; }
    LayerImage* backgroundLayer() const;
    Layer* firstBrowsableLayer() const;
    Layer* firstLayer() const;
    layer_t allLayersCount() const;
    bool hasVisibleReferenceLayers() const;

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
    int totalAnimationDuration() const;
    void setFrameDuration(frame_t frame, int msecs);
    void setFrameRangeDuration(frame_t from, frame_t to, int msecs);
    void setDurationForAllFrames(int msecs);

    const Tags& tags() const { return m_tags; }
    Tags& tags() { return m_tags; }

    const Slices& slices() const { return m_slices; }
    Slices& slices() { return m_slices; }

    ////////////////////////////////////////
    // Shared Images and CelData (for linked Cels)

    ImageRef getImageRef(ObjectId imageId);
    CelDataRef getCelDataRef(ObjectId celDataId);

    ////////////////////////////////////////
    // Images

    void replaceImage(ObjectId curImageId, const ImageRef& newImage);
    void getImages(std::vector<Image*>& images) const;
    void remapImages(frame_t frameFrom, frame_t frameTo, const Remap& remap);
    void pickCels(const double x,
                  const double y,
                  const frame_t frame,
                  const int opacityThreshold,
                  const LayerList& layers,
                  CelList& cels) const;

    ////////////////////////////////////////
    // Iterators

    LayerList allLayers() const;
    LayerList allVisibleLayers() const;
    LayerList allVisibleReferenceLayers() const;
    LayerList allBrowsableLayers() const;

    CelsRange cels() const;
    CelsRange cels(frame_t frame) const;
    CelsRange uniqueCels() const;
    CelsRange uniqueCels(const SelectedFrames& selFrames) const;

  private:
    Document* m_document;
    ImageSpec m_spec;
    PixelRatio m_pixelRatio;
    frame_t m_frames;                      // how many frames has this sprite
    std::vector<int> m_frlens;             // duration per frame
    PalettesList m_palettes;               // list of palettes
    LayerGroup* m_root;                    // main group of layers
    gfx::Rect m_gridBounds;                // grid settings

    // Current rgb map
    mutable RgbMap* m_rgbMap;

    Tags m_tags;
    Slices m_slices;

    // Disable default constructor and copying
    Sprite();
    DISABLE_COPYING(Sprite);
  };

} // namespace doc

#endif
