// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_H_INCLUDED
#define DOC_LAYER_H_INCLUDED
#pragma once

#include "doc/blend_mode.h"
#include "doc/cel_list.h"
#include "doc/frame.h"
#include "doc/layer_list.h"
#include "doc/object.h"

#include <string>

namespace doc {

  class Cel;
  class Image;
  class Sprite;
  class Layer;
  class LayerImage;
  class LayerFolder;

  //////////////////////////////////////////////////////////////////////
  // Layer class

  enum class LayerFlags {
    Visible    = 1,             // Can be read
    Editable   = 2,             // Can be written
    LockMove   = 4,             // Cannot be moved
    Background = 8,             // Stack order cannot be changed
    Continuous = 16,            // Prefer to link cels when the user copy them

    BackgroundLayerFlags = LockMove | Background,
  };

  class Layer : public Object {
  protected:
    Layer(ObjectType type, Sprite* sprite);

  public:
    virtual ~Layer();

    virtual int getMemSize() const override;

    std::string name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    Sprite* sprite() const { return m_sprite; }
    LayerFolder* parent() const { return m_parent; }
    void setParent(LayerFolder* folder) { m_parent = folder; }

    // Gets the previous sibling of this layer.
    Layer* getPrevious() const;
    Layer* getNext() const;

    bool isImage() const { return type() == ObjectType::LayerImage; }
    bool isFolder() const { return type() == ObjectType::LayerFolder; }

    bool isBackground() const { return hasFlags(LayerFlags::Background); }
    bool isVisible() const    { return hasFlags(LayerFlags::Visible); }
    bool isEditable() const   { return hasFlags(LayerFlags::Editable); }
    bool isMovable() const    { return !hasFlags(LayerFlags::LockMove); }
    bool isContinuous() const { return hasFlags(LayerFlags::Continuous); }

    void setBackground(bool state) { switchFlags(LayerFlags::Background, state); }
    void setVisible   (bool state) { switchFlags(LayerFlags::Visible, state); }
    void setEditable  (bool state) { switchFlags(LayerFlags::Editable, state); }
    void setMovable   (bool state) { switchFlags(LayerFlags::LockMove, !state); }
    void setContinuous(bool state) { switchFlags(LayerFlags::Continuous, state); }

    LayerFlags flags() const {
      return m_flags;
    }

    bool hasFlags(LayerFlags flags) const {
      return (int(m_flags) & int(flags)) == int(flags);
    }

    void setFlags(LayerFlags flags) {
      m_flags = flags;
    }

    void switchFlags(LayerFlags flags, bool state) {
      if (state)
        m_flags = LayerFlags(int(m_flags) | int(flags));
      else
        m_flags = LayerFlags(int(m_flags) & ~int(flags));
    }

    virtual Cel* cel(frame_t frame) const;
    virtual void getCels(CelList& cels) const = 0;
    virtual void displaceFrames(frame_t fromThis, frame_t delta) = 0;

  private:
    std::string m_name;           // layer name
    Sprite* m_sprite;             // owner of the layer
    LayerFolder* m_parent;        // parent layer
    LayerFlags m_flags;           // stack order cannot be changed

    // Disable assigment
    Layer& operator=(const Layer& other);
  };

  //////////////////////////////////////////////////////////////////////
  // LayerImage class

  class LayerImage : public Layer {
  public:
    explicit LayerImage(Sprite* sprite);
    virtual ~LayerImage();

    virtual int getMemSize() const override;

    BlendMode blendMode() const { return m_blendmode; }
    void setBlendMode(BlendMode blendmode) { m_blendmode = blendmode; }

    int opacity() const { return m_opacity; }
    void setOpacity(int opacity) { m_opacity = opacity; }

    void addCel(Cel *cel);
    void removeCel(Cel *cel);
    void moveCel(Cel *cel, frame_t frame);

    Cel* cel(frame_t frame) const override;
    void getCels(CelList& cels) const override;
    void displaceFrames(frame_t fromThis, frame_t delta) override;

    Cel* getLastCel() const;
    CelConstIterator findCelIterator(frame_t frame) const;
    CelIterator findCelIterator(frame_t frame);
    CelIterator findFirstCelIteratorAfter(frame_t firstAfterFrame);

    void configureAsBackground();

    CelIterator getCelBegin() { return m_cels.begin(); }
    CelIterator getCelEnd() { return m_cels.end(); }
    CelConstIterator getCelBegin() const { return m_cels.begin(); }
    CelConstIterator getCelEnd() const { return m_cels.end(); }
    int getCelsCount() const { return (int)m_cels.size(); }

  private:
    void destroyAllCels();

    BlendMode m_blendmode;
    int m_opacity;
    CelList m_cels;   // List of all cels inside this layer used by frames.
  };

  //////////////////////////////////////////////////////////////////////
  // LayerFolder class

  class LayerFolder : public Layer {
  public:
    explicit LayerFolder(Sprite* sprite);
    virtual ~LayerFolder();

    virtual int getMemSize() const override;

    const LayerList& getLayersList() { return m_layers; }
    LayerIterator getLayerBegin() { return m_layers.begin(); }
    LayerIterator getLayerEnd() { return m_layers.end(); }
    LayerConstIterator getLayerBegin() const { return m_layers.begin(); }
    LayerConstIterator getLayerEnd() const { return m_layers.end(); }
    int getLayersCount() const { return (int)m_layers.size(); }

    void addLayer(Layer* layer);
    void removeLayer(Layer* layer);
    void stackLayer(Layer* layer, Layer* after);

    Layer* getFirstLayer() { return (m_layers.empty() ? NULL: m_layers.front()); }
    Layer* getLastLayer() { return (m_layers.empty() ? NULL: m_layers.back()); }

    void getCels(CelList& cels) const override;
    void displaceFrames(frame_t fromThis, frame_t delta) override;

  private:
    void destroyAllLayers();

    LayerList m_layers;
  };

} // namespace doc

#endif
