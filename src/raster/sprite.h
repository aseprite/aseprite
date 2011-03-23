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
typedef std::vector<Mask*> MasksList;
typedef std::vector<Path*> PathsList;

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

  int getImgType() const;
  void setImgType(int imgtype);

  int getWidth() const;
  int getHeight() const;
  void setSize(int width, int height);

  bool needAlpha() const;

  ase_uint32 getTransparentColor() const;
  void setTransparentColor(ase_uint32 color);

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
  PalettesList getPalettes() const;

  void setPalette(Palette* pal, bool truncate);
  void resetPalettes();
  void deletePalette(Palette* pal);

  Palette* getCurrentPalette() const;

  RgbMap* getRgbMap();
  RgbMap* getRgbMap(int frame);

  ////////////////////////////////////////
  // Frames

  int getTotalFrames() const;
  void setTotalFrames(int frames);

  int getFrameDuration(int frame) const;
  void setFrameDuration(int frame, int msecs);
  void setDurationForAllFrames(int msecs);

  int getCurrentFrame() const;
  void setCurrentFrame(int frame);

  ////////////////////////////////////////
  // Images

  Stock* getStock() const;

  Image* getCurrentImage(int* x = NULL, int* y = NULL, int* opacity = NULL) const;

  void getCels(CelList& cels);

  void remapImages(int frame_from, int frame_to, const std::vector<int>& mapping);

  ////////////////////////////////////////
  // Mask

  Mask* getMask() const;
  void setMask(const Mask* mask);

  void addMask(Mask* mask);
  void removeMask(Mask* mask);
  Mask* requestMask(const char* name) const;

  MasksList getMasksRepository();

  ////////////////////////////////////////
  // Path

  void addPath(Path* path);
  void removePath(Path* path);
  void setPath(const Path* path);

  PathsList getPathsRepository();

  ////////////////////////////////////////
  // Drawing

  void render(Image* image, int x, int y) const;
  int getPixel(int x, int y) const;

private:
  Sprite();
  class SpriteImpl* m_impl;

  DISABLE_COPYING(Sprite);
};

#endif
