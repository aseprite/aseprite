// Aseprite Document Library
// Copyright (C) 2019-2026  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_H_INCLUDED
#define DOC_LAYER_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "base/enum_flags.h"
#include "base/uuid.h"
#include "doc/blend_mode.h"
#include "doc/cel_list.h"
#include "doc/frame.h"
#include "doc/layer_list.h"
#include "doc/object.h"
#include "doc/with_user_data.h"

#include <string>

namespace doc {

class Cel;
class Grid;
class Image;
class Layer;
class LayerGroup;
class LayerImage;
class Sprite;

//////////////////////////////////////////////////////////////////////
// Layer class

enum class LayerFlags {
  None = 0,
  Visible = 1,     // Can be read
  Editable = 2,    // Can be written
  LockMove = 4,    // Cannot be moved
  Background = 8,  // Stack order cannot be changed
  Continuous = 16, // Prefer to link cels when the user copy them
  Collapsed = 32,  // Prefer to show this group layer collapsed
  Reference = 64,  // Is a reference layer

  PersistentFlagsMask = 0xffff,
  Internal_WasVisible = 0x10000, // Was visible in the alternative state (Alt+click)

  BackgroundLayerFlags = LockMove | Background,

  // Flags that change the modified flag of the document
  // (e.g. created by undoable actions).
  StructuralFlagsMask = Background | Reference,
};
LAF_ENUM_FLAGS(LayerFlags);

class Layer : public WithUserData {
protected:
  // Only constructured by derived classes
  Layer(ObjectType type, Sprite* sprite);

public:
  // Disable assigment
  Layer& operator=(const Layer& other) = delete;

  virtual ~Layer();

  int getMemSize() const override;
  void suspendObject() override;
  void restoreObject() override;

  const std::string& name() const { return m_name; }
  void setName(const std::string& name) { m_name = name; }

  Sprite* sprite() const { return m_sprite; }
  Layer* parent() const { return m_parent; }
  void setParent(Layer* parent) { m_parent = parent; }

  // Gets the previous sibling of this layer.
  Layer* getPrevious() const;
  Layer* getNext() const;

  Layer* getPreviousBrowsable() const;
  Layer* getNextBrowsable() const;

  Layer* getPreviousInWholeHierarchy() const;
  Layer* getNextInWholeHierarchy() const;

  bool isImage() const
  {
    return (type() == ObjectType::LayerImage || type() == ObjectType::LayerTilemap);
  }
  bool hasSublayers() const { return !m_layers.empty(); }
  bool isGroup() const { return type() == ObjectType::LayerGroup; }
  bool isTilemap() const { return type() == ObjectType::LayerTilemap; }
  bool isBrowsable() const { return isExpanded() && !m_layers.empty(); }
  // If this layer is a Mask or FX
  bool isMaskOrFx() const
  {
    return type() == ObjectType::LayerMask || type() == ObjectType::LayerFx;
  }
  // In case we can drop a Mask or FX inside this kind of layer
  bool acceptMaskOrFx() const
  {
    return type() == ObjectType::LayerImage || type() == ObjectType::LayerTilemap ||
           type() == ObjectType::LayerText || type() == ObjectType::LayerVector ||
           type() == ObjectType::LayerSubsprite;
  }

  bool isBackground() const { return hasFlags(LayerFlags::Background); }
  bool isTransparent() const { return !hasFlags(LayerFlags::Background); }
  bool isVisible() const { return hasFlags(LayerFlags::Visible); }
  bool isEditable() const { return hasFlags(LayerFlags::Editable); }
  bool isMovable() const { return !hasFlags(LayerFlags::LockMove); }
  bool isContinuous() const { return hasFlags(LayerFlags::Continuous); }
  bool isCollapsed() const { return hasFlags(LayerFlags::Collapsed); }
  bool isExpanded() const { return !hasFlags(LayerFlags::Collapsed); }
  bool isReference() const { return hasFlags(LayerFlags::Reference); }

  bool isVisibleHierarchy() const;
  bool isEditableHierarchy() const;
  bool canEditPixels() const;
  bool hasAncestor(const Layer* ancestor) const;

  void setBackground(bool state) { switchFlags(LayerFlags::Background, state); }
  void setVisible(bool state) { switchFlags(LayerFlags::Visible, state); }
  void setEditable(bool state) { switchFlags(LayerFlags::Editable, state); }
  void setMovable(bool state) { switchFlags(LayerFlags::LockMove, !state); }
  void setContinuous(bool state) { switchFlags(LayerFlags::Continuous, state); }
  void setCollapsed(bool state) { switchFlags(LayerFlags::Collapsed, state); }
  void setReference(bool state) { switchFlags(LayerFlags::Reference, state); }

  LayerFlags flags() const { return m_flags; }

  bool hasFlags(LayerFlags flags) const { return (int(m_flags) & int(flags)) == int(flags); }

  void setFlags(LayerFlags flags) { m_flags = flags; }

  void switchFlags(LayerFlags flags, bool state)
  {
    if (state)
      m_flags = LayerFlags(int(m_flags) | int(flags));
    else
      m_flags = LayerFlags(int(m_flags) & ~int(flags));
  }

  BlendMode blendMode() const { return m_blendmode; }
  void setBlendMode(BlendMode blendmode) { m_blendmode = blendmode; }

  int opacity() const { return m_opacity; }
  void setOpacity(int opacity) { m_opacity = opacity; }

  const base::Uuid& uuid() const
  {
    if (m_uuid == base::Uuid())
      m_uuid = base::Uuid::Generate();
    return m_uuid;
  }
  void setUuid(const base::Uuid& uuid)
  {
    ASSERT(m_uuid == base::Uuid());
    m_uuid = uuid;
  }

  virtual Grid grid() const;

  // Cels management

  void addCel(Cel* cel);
  void removeCel(Cel* cel);
  void moveCel(Cel* cel, frame_t frame);

  Cel* cel(frame_t frame) const;
  const CelList& cels() const { return m_cels; }
  virtual void getCels(CelList& cels) const;
  virtual void displaceFrames(frame_t fromThis, frame_t delta);

  Cel* getLastCel() const;
  CelConstIterator findCelIterator(frame_t frame) const;
  CelIterator findCelIterator(frame_t frame);
  CelIterator findFirstCelIteratorAfter(frame_t firstAfterFrame);

  CelIterator getCelBegin() { return m_cels.begin(); }
  CelIterator getCelEnd() { return m_cels.end(); }
  CelConstIterator getCelBegin() const { return m_cels.begin(); }
  CelConstIterator getCelEnd() const { return m_cels.end(); }
  int getCelsCount() const { return (int)m_cels.size(); }

  const LayerList& layers() const { return m_layers; }
  int layersCount() const { return (int)m_layers.size(); }

  void addLayer(Layer* layer);
  void removeLayer(Layer* layer);
  void insertLayer(Layer* layer, Layer* after);
  void insertLayerBefore(Layer* layer, Layer* before);
  void stackLayer(Layer* layer, Layer* after);

  Layer* firstLayer() const { return (m_layers.empty() ? nullptr : m_layers.front()); }
  Layer* firstLayerInWholeHierarchy() const;
  Layer* lastLayer() const { return (m_layers.empty() ? nullptr : m_layers.back()); }

  void allLayers(LayerList& list) const;
  layer_t allLayersCount() const;
  bool hasVisibleReferenceLayers() const;
  void allVisibleLayers(LayerList& list) const;
  void allVisibleReferenceLayers(LayerList& list) const;
  void allBrowsableLayers(LayerList& list) const;
  void allTilemaps(LayerList& list) const;
  std::string visibleLayerHierarchyAsString(const std::string& indent) const;

  layer_t getLayerIndex(const Layer* layer) const;

private:
  layer_t getLayerIndex(const Layer* layer, layer_t& index) const;
  void destroyAllCels();
  void destroyAllLayers();

  std::string m_name;        // layer name
  Sprite* m_sprite;          // owner of the layer
  Layer* m_parent;           // parent layer
  LayerFlags m_flags;        // stack order cannot be changed
  mutable base::Uuid m_uuid; // lazily generated layer's UUID

  // Some of these fields might not be used depending on the layer
  // kind (e.g. the blend mode and opacity don't make sense for audio
  // layers).
  BlendMode m_blendmode;
  int m_opacity;

  // List of all cels inside this layer used by frames.
  CelList m_cels;

  // List of all layers inside this layer. For a group, this can be a
  // list of any kind. For other layers, they might be limited to
  // Masks and FXs.
  LayerList m_layers;
};

//////////////////////////////////////////////////////////////////////
// LayerImage class

class LayerImage : public Layer {
public:
  LayerImage(ObjectType type, Sprite* sprite);
  explicit LayerImage(Sprite* sprite);
  virtual ~LayerImage();

  void configureAsBackground();
};

//////////////////////////////////////////////////////////////////////
// LayerGroup class

class LayerGroup final : public Layer {
public:
  explicit LayerGroup(Sprite* sprite);
  virtual ~LayerGroup();
};

} // namespace doc

#endif
