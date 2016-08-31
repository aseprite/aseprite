// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGES_REF_H_INCLUDED
#define DOC_IMAGES_REF_H_INCLUDED
#pragma once

#include "doc/frame.h"

#include <list>

namespace doc {

  class Sprite;
  class Image;
  class Layer;
  class Cel;

  // Collects images from a sprite
  class ImagesCollector {
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
    // * allFrames: True if you want to collect images from all frames
    //              or false if you need images from the given "frame" param.
    // * forEdit:   True if you will modify the images (it is used to avoid
    //              returning images from layers which are read-only/write-locked).
    ImagesCollector(Layer* layer,
                    frame_t frame,
                    bool allFrames,
                    bool forEdit);

    ItemsIterator begin() { return m_items.begin(); }
    ItemsIterator end() { return m_items.end(); }

    std::size_t size() const { return m_items.size(); }
    bool empty() const { return m_items.empty(); }

  private:
    void collectFromLayer(Layer* layer, frame_t frame);
    void collectImage(Layer* layer, Cel* cel);

    Items m_items;
    bool m_allFrames;
    bool m_forEdit;
  };

} // namespace doc

#endif
