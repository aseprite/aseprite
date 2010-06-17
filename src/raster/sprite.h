/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jbase.h"
#include "raster/gfxobj.h"
#include <vector>

struct FormatOptions;
class Image;
class Layer;
class LayerFolder;
class LayerImage;
class Mask;
class Palette;
class Path;
class Stock;
class Undo;
class Sprite;
class RgbMap;
struct _BoundSeg;

struct PreferredEditorSettings
{
  int scroll_x;
  int scroll_y;
  int zoom;
};

/**
 * The main structure used in the whole program to handle a sprite.
 */
class Sprite : public GfxObj
{
public:

  ////////////////////////////////////////
  // Constructors/Destructor

  Sprite(int imgtype, int width, int height, int ncolors);
  Sprite(const Sprite& original);
  virtual ~Sprite();

  ////////////////////////////////////////
  // Special constructors

  static Sprite* createFlattenCopy(const Sprite& original);
  static Sprite* createWithLayer(int imgtype, int width, int height, int ncolors);

  ////////////////////////////////////////
  // Multi-threading ("sprite wrappers" use this)

  bool lock(bool write);
  bool lockToWrite();
  void unlockToRead();
  void unlock();

  ////////////////////////////////////////
  // Main properties

  int getImgType() const;
  void setImgType(int imgtype);

  int getWidth() const;
  int getHeight() const;
  void setSize(int width, int height);

  const char* getFilename() const;
  void setFilename(const char* filename);

  bool isModified() const;
  bool isAssociatedToFile() const;
  void markAsSaved();

  bool needAlpha() const;

  int getMemSize() const;

  ////////////////////////////////////////
  // Layers

  const LayerFolder* getFolder() const;
  LayerFolder* getFolder();

  const LayerImage* getBackgroundLayer() const;
  LayerImage* getBackgroundLayer();

  const Layer* getCurrentLayer() const;
  Layer* getCurrentLayer();
  void setCurrentLayer(Layer* layer);

  int countLayers() const;

  const Layer* indexToLayer(int index) const;
  Layer* indexToLayer(int index);
  int layerToIndex(const Layer* layer) const;

  ////////////////////////////////////////
  // Palettes

  const Palette* getPalette(int frame) const;
  Palette* getPalette(int frame);
  JList getPalettes();

  void setPalette(Palette* pal, bool truncate);
  void resetPalettes();
  void deletePalette(Palette* pal);

  const Palette* getCurrentPalette() const;
  Palette* getCurrentPalette();

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

  const Stock* getStock() const;
  Stock* getStock();

  const Image* getCurrentImage(int* x = NULL, int* y = NULL, int* opacity = NULL) const;
  Image* getCurrentImage(int* x = NULL, int* y = NULL, int* opacity = NULL);

  void getCels(CelList& cels);

  void remapImages(int frame_from, int frame_to, const std::vector<int>& mapping);

  ////////////////////////////////////////
  // Undo

  const Undo* getUndo() const;
  Undo* getUndo();

  ////////////////////////////////////////
  // Mask

  const Mask* getMask() const;
  Mask* getMask();
  void setMask(const Mask* mask);

  void addMask(Mask* mask);
  void removeMask(Mask* mask);
  Mask* requestMask(const char* name) const;

  void generateMaskBoundaries(Mask* mask = NULL);

  JList getMasksRepository();

  ////////////////////////////////////////
  // Path

  void addPath(Path* path);
  void removePath(Path* path);
  void setPath(const Path* path);

  JList getPathsRepository();

  ////////////////////////////////////////
  // Loaded options from file

  void setFormatOptions(FormatOptions* format_options);

  ////////////////////////////////////////
  // Drawing

  void render(Image* image, int x, int y) const;
  int getPixel(int x, int y) const;

  ////////////////////////////////////////
  // Preferred editor settings

  PreferredEditorSettings getPreferredEditorSettings() const;
  void setPreferredEditorSettings(const PreferredEditorSettings& settings);

  ////////////////////////////////////////
  // Boundaries

  int getBoundariesSegmentsCount() const;
  const _BoundSeg* getBoundariesSegments() const;

  ////////////////////////////////////////
  // Extra Cel (it is used to draw pen preview, pixels in movement,
  // etc.)

  void prepareExtraCel(int x, int y, int w, int h, int opacity);
  void destroyExtraCel();
  Cel* getExtraCel() const;
  Image* getExtraCelImage() const;

private:
  Sprite();
  class SpriteImpl* m_impl;
};

#endif
