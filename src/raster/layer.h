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

#ifndef RASTER_LAYER_H_INCLUDED
#define RASTER_LAYER_H_INCLUDED

#include <string>
#include "raster/gfxobj.h"
#include "raster/blend.h"

class Cel;
class Image;
class Sprite;
class Layer;
class LayerImage;
class LayerFolder;

//////////////////////////////////////////////////////////////////////
// Layer class

enum {
  LAYER_IS_READABLE = 0x0001,
  LAYER_IS_WRITABLE = 0x0002,
  LAYER_IS_LOCKMOVE = 0x0004,
  LAYER_IS_BACKGROUND = 0x0008,
};

class Layer : public GfxObj
{
protected:
  Layer(GfxObjType type, Sprite* sprite);

public:
  virtual ~Layer();

  int getMemSize() const;

  std::string getName() const { return m_name; }
  void setName(const std::string& name) { m_name = name; }

  Sprite* getSprite() const { return m_sprite; }
  LayerFolder* get_parent() const { return m_parent; }
  void set_parent(LayerFolder* folder) { m_parent = folder; }
  Layer* get_prev() const;
  Layer* get_next() const;

  bool is_image() const { return getType() == GFXOBJ_LAYER_IMAGE; }
  bool is_folder() const { return getType() == GFXOBJ_LAYER_FOLDER; }
  bool is_background() const { return (m_flags & LAYER_IS_BACKGROUND) == LAYER_IS_BACKGROUND; }
  bool is_moveable() const { return (m_flags & LAYER_IS_LOCKMOVE) == 0; }
  bool is_readable() const { return (m_flags & LAYER_IS_READABLE) == LAYER_IS_READABLE; }
  bool is_writable() const { return (m_flags & LAYER_IS_WRITABLE) == LAYER_IS_WRITABLE; }

  void set_background(bool b) { if (b) m_flags |= LAYER_IS_BACKGROUND; else m_flags &= ~LAYER_IS_BACKGROUND; }
  void set_moveable(bool b) { if (b) m_flags &= ~LAYER_IS_LOCKMOVE; else m_flags |= LAYER_IS_LOCKMOVE; }
  void set_readable(bool b) { if (b) m_flags |= LAYER_IS_READABLE; else m_flags &= ~LAYER_IS_READABLE; }
  void set_writable(bool b) { if (b) m_flags |= LAYER_IS_WRITABLE; else m_flags &= ~LAYER_IS_WRITABLE; }

  uint32_t getFlags() const { return m_flags; }
  void setFlags(uint32_t flags) { m_flags = flags; }

  virtual void getCels(CelList& cels) = 0;

private:
  std::string m_name;		// layer name
  Sprite* m_sprite;		// owner of the layer
  LayerFolder* m_parent;	// parent layer
  unsigned short m_flags;

  // Disable assigment
  Layer& operator=(const Layer& other);
};

//////////////////////////////////////////////////////////////////////
// LayerImage class

class LayerImage : public Layer
{
public:
  explicit LayerImage(Sprite* sprite);
  virtual ~LayerImage();

  int getMemSize() const;

  int getBlendMode() const { return BLEND_MODE_NORMAL; }

  void addCel(Cel *cel);
  void removeCel(Cel *cel);
  const Cel* getCel(int frame) const;
  Cel* getCel(int frame);

  void getCels(CelList& cels);

  void configureAsBackground();

  CelIterator getCelBegin() { return m_cels.begin(); }
  CelIterator getCelEnd() { return m_cels.end(); }
  CelConstIterator getCelBegin() const { return m_cels.begin(); }
  CelConstIterator getCelEnd() const { return m_cels.end(); }
  int getCelsCount() const { return m_cels.size(); }

private:
  void destroyAllCels();

  CelList m_cels;   // List of all cels inside this layer used by frames.
};

//////////////////////////////////////////////////////////////////////
// LayerFolder class

class LayerFolder : public Layer
{
public:
  explicit LayerFolder(Sprite* sprite);
  virtual ~LayerFolder();

  int getMemSize() const;

  LayerList get_layers_list() { return m_layers; }
  LayerIterator get_layer_begin() { return m_layers.begin(); }
  LayerIterator get_layer_end() { return m_layers.end(); }
  LayerConstIterator get_layer_begin() const { return m_layers.begin(); }
  LayerConstIterator get_layer_end() const { return m_layers.end(); }
  int get_layers_count() const { return m_layers.size(); }

  void add_layer(Layer* layer);
  void remove_layer(Layer* layer);
  void move_layer(Layer* layer, Layer* after);

  void getCels(CelList& cels);

private:
  void destroyAllLayers();

  LayerList m_layers;
};

Layer* layer_new_flatten_copy(Sprite* dst_sprite, const Layer* src_layer,
			      int x, int y, int w, int h, int frmin, int frmax);

void layer_render(const Layer* layer, Image *image, int x, int y, int frame);

#endif
