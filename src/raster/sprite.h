/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "raster/gfxobj.h"

#include <vector>

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
class Sprite : public GfxObj
{
public:

  ////////////////////////////////////////
  // Constructors/Destructor

  Sprite(int imgtype, int width, int height, int ncolors);
  virtual ~Sprite();

  ////////////////////////////////////////
  // Main properties

  int getImgType() const { return m_imgtype; }
  void setImgType(int imgtype);

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
  Layer* getCurrentLayer() const;
  void setCurrentLayer(Layer* layer);

  int countLayers() const;

  Layer* indexToLayer(int index) const;
  int layerToIndex(const Layer* layer) const;

  ////////////////////////////////////////
  // Palettes

  Palette* getPalette(int frame) const;
  const PalettesList& getPalettes() const;

  void setPalette(const Palette* pal, bool truncate);

  // Removes all palettes from the sprites except the first one.
  void resetPalettes();

  void deletePalette(Palette* pal);

  Palette* getCurrentPalette() const;

  RgbMap* getRgbMap();
  RgbMap* getRgbMap(int frame);

  ////////////////////////////////////////
  // Frames

  int getTotalFrames() const { return m_frames; }

  // Changes the quantity of frames
  void setTotalFrames(int frames);

  int getFrameDuration(int frame) const;
  void setFrameDuration(int frame, int msecs);

  // Sets a constant frame-rate.
  void setDurationForAllFrames(int msecs);

  int getCurrentFrame() const { return m_frame; }
  void setCurrentFrame(int frame);

  ////////////////////////////////////////
  // Images

  Stock* getStock() const;

  Image* getCurrentImage(int* x = NULL, int* y = NULL, int* opacity = NULL) const;

  void getCels(CelList& cels);

  void remapImages(int frame_from, int frame_to, const std::vector<uint8_t>& mapping);

  // Draws the sprite in the given image at the given position. Before
  // drawing the sprite, this function clears (with the sprite's
  // background color) the rectangle area that will occupy the drawn
  // sprite.
  void render(Image* image, int x, int y) const;

  // Gets a pixel from the sprite in the specified position. If in the
  // specified coordinates there're background this routine will
  // return the 0 color (the mask-color).
  int getPixel(int x, int y) const;

private:
  Sprite* m_self;			 // pointer to the Sprite
  int m_imgtype;			 // image type
  int m_width;				 // image width (in pixels)
  int m_height;				 // image height (in pixels)
  int m_frames;				 // how many frames has this sprite
  std::vector<int> m_frlens;		 // duration per frame
  int m_frame;				 // current frame, range [0,frames)
  PalettesList m_palettes;		 // list of palettes
  Stock* m_stock;			 // stock to get images
  LayerFolder* m_folder;		 // main folder of layers
  Layer* m_layer;			 // current layer

  // Current rgb map
  RgbMap* m_rgbMap;

  // Transparent color used in indexed images
  uint32_t m_transparentColor;

  // Disable default constructor and copying
  Sprite();
  DISABLE_COPYING(Sprite);
};

#endif
