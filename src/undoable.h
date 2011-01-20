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

#ifndef UNDOABLE_H_INCLUDED
#define UNDOABLE_H_INCLUDED

#include "raster/dithering_method.h"

class Cel;
class Mask;
class Layer;
class LayerImage;
class Sprite;
class SpriteWriter;

/**
 * Class with high-level set of routines to modify a sprite.
 *
 * In this class, all modifications in the sprite have an undo
 * counterpart (if the sprite's undo is enabled).
 */
class Undoable
{
public:
  Undoable(SpriteWriter& sprite, const char* label);
  virtual ~Undoable();

  inline Sprite* getSprite() const { return m_sprite;  }
  inline bool isEnabled() const { return m_enabledFlag; }

  void commit();

  // for sprite
  void setNumberOfFrames(int frames);
  void setCurrentFrame(int frame);
  void setCurrentLayer(Layer* layer);
  void setSpriteSize(int w, int h);
  void cropSprite(int x, int y, int w, int h, int bgcolor);
  void autocropSprite(int bgcolor);
  void setImgType(int new_imgtype, DitheringMethod dithering_method);

  // for images in stock
  int addImageInStock(Image* image);
  void removeImageFromStock(int image_index);
  void replaceStockImage(int image_index, Image* new_image);

  // for layers
  Layer* newLayer();
  void removeLayer(Layer* layer);
  void moveLayerAfter(Layer *layer, Layer *after_this);
  void cropLayer(Layer* layer, int x, int y, int w, int h, int bgcolor);
  void displaceLayers(Layer* layer, int dx, int dy);
  void backgroundFromLayer(LayerImage* layer, int bgcolor);
  void layerFromBackground();
  void flattenLayers(int bgcolor);
private:
  void configureLayerAsBackground(LayerImage* layer);

public:
  // for frames
  void newFrame();
  void newFrameForLayer(Layer* layer, int frame);
  void removeFrame(int frame);
  void removeFrameOfLayer(Layer* layer, int frame);
  void copyPreviousFrame(Layer* layer, int frame);
  void setFrameDuration(int frame, int msecs);
  void setConstantFrameRate(int msecs);
  void moveFrameBefore(int frame, int before_frame);
  void moveFrameBeforeLayer(Layer* layer, int frame, int before_frame);

  // for cels
  void addCel(LayerImage* layer, Cel* cel);
  void removeCel(LayerImage* layer, Cel* cel);
  void setCelFramePosition(Cel* cel, int frame);
  void setCelPosition(Cel* cel, int x, int y);
  Cel* getCurrentCel();
  void cropCel(Cel* cel, int x, int y, int w, int h, int bgcolor);

  // for image
  Image* getCelImage(Cel* cel);
  void clearMask(int bgcolor);
  void flipImage(Image* image, int x1, int y1, int x2, int y2,
		 bool flip_horizontal, bool flip_vertical);
  void pasteImage(const Image* src_image, int x, int y, int opacity);

  // for mask
  void copyToCurrentMask(Mask* mask);
  void setMaskPosition(int x, int y);
  void deselectMask();

private:
  Sprite* m_sprite;
  bool m_committed;
  bool m_enabledFlag;
};

#endif
