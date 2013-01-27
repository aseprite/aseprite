/* ASEPRITE
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

#ifndef RASTER_IMAGES_REF_H_INCLUDED
#define RASTER_IMAGES_REF_H_INCLUDED

#include <list>

class Sprite;
class Image;
class Layer;
class Cel;

// Collects images from a sprite
class ImagesCollector
{
public:
  class Item {
  public:
    Item(Layer* layer, Cel* cel, Image* image)
      : m_layer(layer)
      , m_cel(cel)
      , m_image(image)
    { }

    Layer* layer() const { return m_layer; }
    Cel*   cel()   const { return m_cel; }
    Image* image() const { return m_image; }

  private:
    Layer* m_layer;
    Cel*   m_cel;
    Image* m_image;
  };

  typedef std::list<Item> Items;
  typedef std::list<Item>::iterator ItemsIterator;

  // Creates a collection of images from the specified sprite.
  // Parameters:
  // * allLayers: True if you want to collect images from all layers or
  //              false if you want to images from the current layer only.
  // * allFrames: True if you want to collect images from all frames
  //              or false if you need images from the current frame.
  // * forWrite: True if you will modify the images (it is used to avoid
  //             returning images from layers which are read-only/write-locked).
  ImagesCollector(const Sprite* sprite, bool allLayers, bool allFrames, bool forWrite);

  ItemsIterator begin() { return m_items.begin(); }
  ItemsIterator end() { return m_items.end(); }

  size_t size() const { return m_items.size(); }
  bool empty() const { return m_items.empty(); }

private:
  void collectFromLayer(Layer* layer);
  void collectImage(Layer* layer, Cel* cel);

  Items m_items;
  bool m_allFrames;
  bool m_forWrite;
};

#endif
