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

#ifndef RASTER_LAYER_H_INCLUDED
#define RASTER_LAYER_H_INCLUDED
#pragma once

#include "raster/blend.h"
#include "raster/frame_number.h"
#include "raster/object.h"

#include <string>

namespace raster {

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

  class Layer : public Object {
  protected:
    Layer(ObjectType type, Sprite* sprite);

  public:
    virtual ~Layer();

    int getMemSize() const;

    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    Sprite* getSprite() const { return m_sprite; }
    LayerFolder* getParent() const { return m_parent; }
    void setParent(LayerFolder* folder) { m_parent = folder; }

    // Gets the previous sibling of this layer.
    Layer* getPrevious() const;
    Layer* getNext() const;

    bool isImage() const { return type() == OBJECT_LAYER_IMAGE; }
    bool isFolder() const { return type() == OBJECT_LAYER_FOLDER; }
    bool isBackground() const { return (m_flags & LAYER_IS_BACKGROUND) == LAYER_IS_BACKGROUND; }
    bool isMoveable() const { return (m_flags & LAYER_IS_LOCKMOVE) == 0; }
    bool isReadable() const { return (m_flags & LAYER_IS_READABLE) == LAYER_IS_READABLE; }
    bool isWritable() const { return (m_flags & LAYER_IS_WRITABLE) == LAYER_IS_WRITABLE; }

    void setBackground(bool b) { if (b) m_flags |= LAYER_IS_BACKGROUND; else m_flags &= ~LAYER_IS_BACKGROUND; }
    void setMoveable(bool b) { if (b) m_flags &= ~LAYER_IS_LOCKMOVE; else m_flags |= LAYER_IS_LOCKMOVE; }
    void setReadable(bool b) { if (b) m_flags |= LAYER_IS_READABLE; else m_flags &= ~LAYER_IS_READABLE; }
    void setWritable(bool b) { if (b) m_flags |= LAYER_IS_WRITABLE; else m_flags &= ~LAYER_IS_WRITABLE; }

    uint32_t getFlags() const { return m_flags; }
    void setFlags(uint32_t flags) { m_flags = flags; }

    virtual void getCels(CelList& cels) = 0;

  private:
    std::string m_name;           // layer name
    Sprite* m_sprite;             // owner of the layer
    LayerFolder* m_parent;        // parent layer
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
    const Cel* getCel(FrameNumber frame) const;
    Cel* getCel(FrameNumber frame);

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

    const LayerList& getLayersList() { return m_layers; }
    LayerIterator getLayerBegin() { return m_layers.begin(); }
    LayerIterator getLayerEnd() { return m_layers.end(); }
    LayerConstIterator getLayerBegin() const { return m_layers.begin(); }
    LayerConstIterator getLayerEnd() const { return m_layers.end(); }
    int getLayersCount() const { return m_layers.size(); }

    void addLayer(Layer* layer);
    void removeLayer(Layer* layer);
    void stackLayer(Layer* layer, Layer* after);

    Layer* getFirstLayer() { return (m_layers.empty() ? NULL: m_layers.front()); }
    Layer* getLastLayer() { return (m_layers.empty() ? NULL: m_layers.back()); }

    void getCels(CelList& cels);

  private:
    void destroyAllLayers();

    LayerList m_layers;
  };

  void layer_render(const Layer* layer, Image *image, int x, int y, FrameNumber frame);

} // namespace raster

#endif
