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

#ifndef RASTER_SPRITE_H_INCLUDED
#define RASTER_SPRITE_H_INCLUDED

#include "base/disable_copying.h"
#include "raster/frame_number.h"
#include "raster/layer_index.h"
#include "raster/gfxobj.h"
#include "raster/pixel_format.h"
#include "raster/sprite_position.h"

#include <vector>

namespace raster {

  class Image;
  class Layer;
  class LayerFolder;
  class LayerImage;
  class Mask;
  class Palette;
  class Path;
  class Stock;
  class Sprite;
  class RgbMap;

  typedef std::vector<Palette*> PalettesList;

  // The main structure used in the whole program to handle a sprite.
  class Sprite : public GfxObj {
  public:

    ////////////////////////////////////////
    // Constructors/Destructor

    Sprite(PixelFormat format, int width, int height, int ncolors);
    virtual ~Sprite();

    ////////////////////////////////////////
    // Main properties

    PixelFormat getPixelFormat() const { return m_format; }
    void setPixelFormat(PixelFormat format);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    void setSize(int width, int height);

    // Returns true if the rendered images will contain alpha values less
    // than 255. Only RGBA and Grayscale images without background needs
    // alpha channel in the render.
    bool needAlpha() const;

    uint32_t getTransparentColor() const { return m_transparentColor; }
    void setTransparentColor(uint32_t color);

    int getMemSize() const;

    ////////////////////////////////////////
    // Layers

    LayerFolder* getFolder() const;
    LayerImage* getBackgroundLayer() const;

    LayerIndex countLayers() const;

    Layer* indexToLayer(LayerIndex index) const;
    LayerIndex layerToIndex(const Layer* layer) const;

    ////////////////////////////////////////
    // Palettes

    Palette* getPalette(FrameNumber frame) const;
    const PalettesList& getPalettes() const;

    void setPalette(const Palette* pal, bool truncate);

    // Removes all palettes from the sprites except the first one.
    void resetPalettes();

    void deletePalette(Palette* pal);

    RgbMap* getRgbMap(FrameNumber frame);

    ////////////////////////////////////////
    // Frames

    FrameNumber getTotalFrames() const { return m_frames; }
    FrameNumber getLastFrame() const { return m_frames.previous(); }

    void addFrame(FrameNumber newFrame);
    void removeFrame(FrameNumber newFrame);
    void setTotalFrames(FrameNumber frames);

    int getFrameDuration(FrameNumber frame) const;
    void setFrameDuration(FrameNumber frame, int msecs);

    // Sets a constant frame-rate.
    void setDurationForAllFrames(int msecs);

    ////////////////////////////////////////
    // Images

    Stock* getStock() const;

    void getCels(CelList& cels);

    void remapImages(FrameNumber frameFrom, FrameNumber frameTo, const std::vector<uint8_t>& mapping);

    // Draws the sprite in the given image at the given position. Before
    // drawing the sprite, this function clears (with the sprite's
    // background color) the rectangle area that will occupy the drawn
    // sprite.
    void render(Image* image, int x, int y, FrameNumber frame) const;

    // Gets a pixel from the sprite in the specified position. If in the
    // specified coordinates there're background this routine will
    // return the 0 color (the mask-color).
    int getPixel(int x, int y, FrameNumber frame) const;

  private:
    Sprite* m_self;                        // pointer to the Sprite
    PixelFormat m_format;                  // pixel format
    int m_width;                           // image width (in pixels)
    int m_height;                          // image height (in pixels)
    FrameNumber m_frames;                  // how many frames has this sprite
    std::vector<int> m_frlens;             // duration per frame
    PalettesList m_palettes;               // list of palettes
    Stock* m_stock;                        // stock to get images
    LayerFolder* m_folder;                 // main folder of layers

    // Current rgb map
    RgbMap* m_rgbMap;

    // Transparent color used in indexed images
    uint32_t m_transparentColor;

    // Disable default constructor and copying
    Sprite();
    DISABLE_COPYING(Sprite);
  };

} // namespace raster

#endif
