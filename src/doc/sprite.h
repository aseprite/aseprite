// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SPRITE_H_INCLUDED
#define DOC_SPRITE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/cel_list.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/layer_index.h"
#include "doc/object.h"
#include "doc/pixel_format.h"
#include "doc/sprite_position.h"
#include "gfx/rect.h"

#include <vector>

namespace doc {

  class Image;
  class Layer;
  class LayerFolder;
  class LayerImage;
  class Mask;
  class Palette;
  class Path;
  class RgbMap;
  class Sprite;
  class Stock;

  typedef std::vector<Palette*> PalettesList;

  // The main structure used in the whole program to handle a sprite.
  class Sprite : public Object {
  public:

    ////////////////////////////////////////
    // Constructors/Destructor

    Sprite(PixelFormat format, int width, int height, int ncolors);
    virtual ~Sprite();

    static Sprite* createBasicSprite(PixelFormat format, int width, int height, int ncolors);

    ////////////////////////////////////////
    // Main properties

    PixelFormat pixelFormat() const { return m_format; }
    void setPixelFormat(PixelFormat format);

    gfx::Rect bounds() const { return gfx::Rect(0, 0, m_width, m_height); }
    int width() const { return m_width; }
    int height() const { return m_height; }
    void setSize(int width, int height);

    // Returns true if the rendered images will contain alpha values less
    // than 255. Only RGBA and Grayscale images without background needs
    // alpha channel in the render.
    bool needAlpha() const;
    bool supportAlpha() const;

    color_t transparentColor() const { return m_transparentColor; }
    void setTransparentColor(color_t color);

    virtual int getMemSize() const override;

    ////////////////////////////////////////
    // Layers

    LayerFolder* folder() const;
    LayerImage* backgroundLayer() const;

    LayerIndex countLayers() const;

    Layer* layer(int layerIndex) const;
    Layer* indexToLayer(LayerIndex index) const;
    LayerIndex layerToIndex(const Layer* layer) const;

    void getLayersList(std::vector<Layer*>& layers) const;

    ////////////////////////////////////////
    // Palettes

    Palette* palette(frame_t frame) const;
    const PalettesList& getPalettes() const;

    void setPalette(const Palette* pal, bool truncate);

    // Removes all palettes from the sprites except the first one.
    void resetPalettes();

    void deletePalette(Palette* pal);

    RgbMap* rgbMap(frame_t frame);

    ////////////////////////////////////////
    // Frames

    frame_t totalFrames() const { return m_frames; }
    frame_t lastFrame() const { return m_frames-1; }

    void addFrame(frame_t newFrame);
    void removeFrame(frame_t newFrame);
    void setTotalFrames(frame_t frames);

    int frameDuration(frame_t frame) const;
    void setFrameDuration(frame_t frame, int msecs);
    void setFrameRangeDuration(frame_t from, frame_t to, int msecs);
    void setDurationForAllFrames(int msecs);

    ////////////////////////////////////////
    // Images

    Stock* stock() const;

    void getCels(CelList& cels) const;

    // Returns the how many cels are referencing the given imageIndex.
    size_t getImageRefs(int imageIndex) const;

    void remapImages(frame_t frameFrom, frame_t frameTo, const std::vector<uint8_t>& mapping);

    void pickCels(int x, int y, frame_t frame, int opacityThreshold, CelList& cels) const;

  private:
    Sprite* m_self;                        // pointer to the Sprite
    PixelFormat m_format;                  // pixel format
    int m_width;                           // image width (in pixels)
    int m_height;                          // image height (in pixels)
    frame_t m_frames;                      // how many frames has this sprite
    std::vector<int> m_frlens;             // duration per frame
    PalettesList m_palettes;               // list of palettes
    Stock* m_stock;                        // stock to get images
    LayerFolder* m_folder;                 // main folder of layers

    // Current rgb map
    RgbMap* m_rgbMap;

    // Transparent color used in indexed images
    color_t m_transparentColor;

    // Disable default constructor and copying
    Sprite();
    DISABLE_COPYING(Sprite);
  };

} // namespace doc

#endif
