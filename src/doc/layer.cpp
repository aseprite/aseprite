// Aseprite Document Library
// Copyright (C) 2019-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/layer.h"

#include "doc/cel.h"
#include "doc/grid.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/sprite.h"

#include <algorithm>
#include <cstring>

namespace doc {

Layer::Layer(ObjectType type, Sprite* sprite)
  : WithUserData(type)
  , m_sprite(sprite)
  , m_parent(NULL)
  , m_flags(LayerFlags(
      int(LayerFlags::Visible) |
      int(LayerFlags::Editable)))
{
  ASSERT(type == ObjectType::LayerImage ||
         type == ObjectType::LayerGroup ||
         type == ObjectType::LayerTilemap);

  setName("Layer");
}

Layer::~Layer()
{
}

int Layer::getMemSize() const
{
  return sizeof(Layer);
}

Layer* Layer::getPrevious() const
{
  if (m_parent) {
    auto it =
      std::find(m_parent->layers().begin(),
                m_parent->layers().end(), this);

    if (it != m_parent->layers().end() &&
        it != m_parent->layers().begin()) {
      it--;
      return *it;
    }
  }
  return nullptr;
}

Layer* Layer::getNext() const
{
  if (m_parent) {
    auto it =
      std::find(m_parent->layers().begin(),
                m_parent->layers().end(), this);

    if (it != m_parent->layers().end()) {
      it++;
      if (it != m_parent->layers().end())
        return *it;
    }
  }
  return nullptr;
}

Layer* Layer::getPreviousBrowsable() const
{
  // Go to children
  if (isBrowsable())
    return static_cast<const LayerGroup*>(this)->lastLayer();

  // Go to previous layer
  if (Layer* prev = getPrevious())
    return prev;

  // Go to previous layer in the parent
  LayerGroup* parent = this->parent();
  while (parent != sprite()->root() &&
         !parent->getPrevious()) {
    parent = parent->parent();
  }
  return parent->getPrevious();
}

Layer* Layer::getNextBrowsable() const
{
  // Go to next layer
  if (Layer* next = getNext()) {
    // Go to children
    while (next->isBrowsable()) {
      Layer* firstChild = static_cast<const LayerGroup*>(next)->firstLayer();
      if (!firstChild)
        break;
      next = firstChild;
    }
    return next;
  }

  // Go to parent
  if (m_sprite && parent() != m_sprite->root())
    return m_parent;

  return nullptr;
}

Layer* Layer::getPreviousInWholeHierarchy() const
{
  // Go to children
  if (isGroup() && static_cast<const LayerGroup*>(this)->layersCount() > 0)
    return static_cast<const LayerGroup*>(this)->lastLayer();

  // Go to previous layer
  if (Layer* prev = getPrevious())
    return prev;

  // Go to previous layer in the parent
  LayerGroup* parent = this->parent();
  while (parent != sprite()->root() &&
         !parent->getPrevious()) {
    parent = parent->parent();
  }
  return parent->getPrevious();
}

Layer* Layer::getNextInWholeHierarchy() const
{
  // Go to next layer
  if (Layer* next = getNext()) {
    // Go to children
    while (next->isGroup() && static_cast<const LayerGroup*>(next)->layersCount() > 0) {
      Layer* firstChild = static_cast<const LayerGroup*>(next)->firstLayer();
      if (!firstChild)
        break;
      next = firstChild;
    }
    return next;
  }

  // Go to parent
  if (m_sprite && parent() != m_sprite->root())
    return m_parent;

  return nullptr;
}

bool Layer::isVisibleHierarchy() const
{
  const Layer* layer = this;
  while (layer) {
    if (!layer->isVisible())
      return false;
    layer = layer->parent();
  }
  return true;
}

bool Layer::isEditableHierarchy() const
{
  const Layer* layer = this;
  while (layer) {
    if (!layer->isEditable())
      return false;
    layer = layer->parent();
  }
  return true;
}

// It's like isVisibleHierarchy + isEditableHierarchy. Returns true if
// the whole layer hierarchy is unlocked and visible, so the user can
// edit its pixels without unexpected side-effects (e.g. editing
// hidden layers).
bool Layer::canEditPixels() const
{
  const Layer* layer = this;
  while (layer) {
    if (!layer->isVisible() ||
        !layer->isEditable() ||
        layer->isReference()) { // Cannot edit pixels from reference layers
      return false;
    }
    layer = layer->parent();
  }
  return true;
}

bool Layer::hasAncestor(const Layer* ancestor) const
{
  Layer* it = parent();
  while (it) {
    if (it == ancestor)
      return true;
    it = it->parent();
  }
  return false;
}

Grid Layer::grid() const
{
  gfx::Rect rc = (m_sprite ? m_sprite->gridBounds():
                             doc::Sprite::DefaultGridBounds());
  doc::Grid grid = Grid(rc.size());
  grid.origin(gfx::Point(rc.x % rc.w, rc.y % rc.h));
  return grid;
}

Cel* Layer::cel(frame_t frame) const
{
  return nullptr;
}

//////////////////////////////////////////////////////////////////////
// LayerImage class

LayerImage::LayerImage(ObjectType type, Sprite* sprite)
  : Layer(type, sprite)
  , m_blendmode(BlendMode::NORMAL)
  , m_opacity(255)
{
}

LayerImage::LayerImage(Sprite* sprite)
  : LayerImage(ObjectType::LayerImage, sprite)
{
}

LayerImage::~LayerImage()
{
  destroyAllCels();
}

int LayerImage::getMemSize() const
{
  int size = sizeof(LayerImage);
  CelConstIterator it = getCelBegin();
  CelConstIterator end = getCelEnd();

  for (; it != end; ++it) {
    const Cel* cel = *it;
    size += cel->getMemSize();

    const Image* image = cel->image();
    size += image->getMemSize();
  }

  return size;
}

void LayerImage::destroyAllCels()
{
  CelIterator it = getCelBegin();
  CelIterator end = getCelEnd();

  for (; it != end; ++it) {
    Cel* cel = *it;
    delete cel;
  }
  m_cels.clear();
}

Cel* LayerImage::cel(frame_t frame) const
{
  CelConstIterator it = findCelIterator(frame);
  if (it != getCelEnd())
    return *it;
  else
    return nullptr;
}

void LayerImage::getCels(CelList& cels) const
{
  CelConstIterator it = getCelBegin();
  CelConstIterator end = getCelEnd();

  for (; it != end; ++it)
    cels.push_back(*it);
}

Cel* LayerImage::getLastCel() const
{
  if (!m_cels.empty())
    return m_cels.back();
  else
    return NULL;
}

CelConstIterator LayerImage::findCelIterator(frame_t frame) const
{
  CelIterator it = const_cast<LayerImage*>(this)->findCelIterator(frame);
  return CelConstIterator(it);
}

CelIterator LayerImage::findCelIterator(frame_t frame)
{
  auto first = getCelBegin();
  auto end = getCelEnd();

  // Here we use a binary search to find the first cel equal to "frame" (or after frame)
  first = std::lower_bound(
    first, end, nullptr,
    [frame](Cel* cel, Cel*) -> bool {
      return cel->frame() < frame;
    });

  // We return the iterator only if it's an exact match
  if (first != end && (*first)->frame() == frame)
    return first;
  else
    return end;
}

CelIterator LayerImage::findFirstCelIteratorAfter(frame_t firstAfterFrame)
{
  auto first = getCelBegin();
  auto end = getCelEnd();

  // Here we use a binary search to find the first cel after the given frame
  first = std::lower_bound(
    first, end, nullptr,
    [firstAfterFrame](Cel* cel, Cel*) -> bool {
      return cel->frame() <= firstAfterFrame;
    });

  return first;
}

void LayerImage::addCel(Cel* cel)
{
  ASSERT(cel);
  ASSERT(cel->data() && "The cel doesn't contain CelData");
  ASSERT(cel->image());
  ASSERT(sprite());
  ASSERT(cel->image()->pixelFormat() == sprite()->pixelFormat() ||
         cel->image()->pixelFormat() == IMAGE_TILEMAP);

  CelIterator it = findFirstCelIteratorAfter(cel->frame());
  m_cels.insert(it, cel);

  cel->setParentLayer(this);
}

/**
 * Removes the cel from the layer.
 *
 * It doesn't destroy the cel, you have to delete it after calling
 * this routine.
 */
void LayerImage::removeCel(Cel* cel)
{
  ASSERT(cel);
  CelIterator it = findCelIterator(cel->frame());
  ASSERT(it != m_cels.end());

  m_cels.erase(it);

  cel->setParentLayer(NULL);
}

void LayerImage::moveCel(Cel* cel, frame_t frame)
{
  removeCel(cel);
  cel->setFrame(frame);
  cel->incrementVersion();      // TODO this should be in app::cmd module
  addCel(cel);
}

/**
 * Configures some properties of the specified layer to make it as the
 * "Background" of the sprite.
 *
 * You can't use this routine if the sprite already has a background
 * layer.
 */
void LayerImage::configureAsBackground()
{
  ASSERT(sprite() != NULL);
  ASSERT(sprite()->backgroundLayer() == NULL);

  switchFlags(LayerFlags::BackgroundLayerFlags, true);
  setName("Background");

  sprite()->root()->stackLayer(this, NULL);
}

void LayerImage::displaceFrames(frame_t fromThis, frame_t delta)
{
  Sprite* sprite = this->sprite();

  if (delta > 0) {
    for (frame_t c=sprite->lastFrame(); c>=fromThis; --c) {
      if (Cel* cel = this->cel(c))
        moveCel(cel, c+delta);
    }
  }
  else {
    for (frame_t c=fromThis; c<=sprite->lastFrame(); ++c) {
      if (Cel* cel = this->cel(c))
        moveCel(cel, c+delta);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// LayerGroup class

LayerGroup::LayerGroup(Sprite* sprite)
  : Layer(ObjectType::LayerGroup, sprite)
{
  setName("Group");
}

LayerGroup::~LayerGroup()
{
  destroyAllLayers();
}

void LayerGroup::destroyAllLayers()
{
  for (Layer* layer : m_layers)
    delete layer;
  m_layers.clear();
}

int LayerGroup::getMemSize() const
{
  int size = sizeof(LayerGroup);

  for (const Layer* layer : m_layers) {
    size += layer->getMemSize();
  }

  return size;
}

Layer* LayerGroup::firstLayerInWholeHierarchy() const
{
  Layer* layer = firstLayer();
  if (layer) {
    while (layer->isGroup() &&
           static_cast<LayerGroup*>(layer)->layersCount() > 0) {
      layer = static_cast<LayerGroup*>(layer)->firstLayer();
    }
  }
  return layer;
}

void LayerGroup::allLayers(LayerList& list) const
{
  for (Layer* child : m_layers) {
    if (child->isGroup())
      static_cast<LayerGroup*>(child)->allLayers(list);

    list.push_back(child);
  }
}

layer_t LayerGroup::allLayersCount() const
{
  layer_t count = 0;
  for (Layer* child : m_layers) {
    if (child->isGroup())
      count += static_cast<LayerGroup*>(child)->allLayersCount();
    ++count;
  }
  return count;
}

bool LayerGroup::hasVisibleReferenceLayers() const
{
  for (Layer* child : m_layers) {
    if ((child->isReference() && child->isVisible())
        || (child->isGroup()
            && static_cast<LayerGroup*>(child)->hasVisibleReferenceLayers()))
      return true;
  }
  return false;
}

void LayerGroup::allVisibleLayers(LayerList& list) const
{
  for (Layer* child : m_layers) {
    if (!child->isVisible())
      continue;

    if (child->isGroup())
      static_cast<LayerGroup*>(child)->allVisibleLayers(list);

    list.push_back(child);
  }
}

void LayerGroup::allVisibleReferenceLayers(LayerList& list) const
{
  for (Layer* child : m_layers) {
    if (!child->isVisible())
      continue;

    if (child->isGroup())
      static_cast<LayerGroup*>(child)->allVisibleReferenceLayers(list);

    if (!child->isReference())
      continue;

    list.push_back(child);
  }
}

void LayerGroup::allBrowsableLayers(LayerList& list) const
{
  for (Layer* child : m_layers) {
    if (child->isBrowsable())
      static_cast<LayerGroup*>(child)->allBrowsableLayers(list);

    list.push_back(child);
  }
}

void LayerGroup::allTilemaps(LayerList& list) const
{
  for (Layer* child : m_layers) {
    if (child->isGroup())
      static_cast<LayerGroup*>(child)->allTilemaps(list);

    if (child->isTilemap())
      list.push_back(child);
  }
}

void LayerGroup::getCels(CelList& cels) const
{
  for (const Layer* layer : m_layers)
    layer->getCels(cels);
}

void LayerGroup::addLayer(Layer* layer)
{
  m_layers.push_back(layer);
  layer->setParent(this);
}

void LayerGroup::removeLayer(Layer* layer)
{
  auto it = std::find(m_layers.begin(), m_layers.end(), layer);
  ASSERT(it != m_layers.end());
  m_layers.erase(it);

  layer->setParent(nullptr);
}

void LayerGroup::insertLayer(Layer* layer, Layer* after)
{
  auto after_it = m_layers.begin();
  if (after) {
    after_it = std::find(m_layers.begin(), m_layers.end(), after);
    if (after_it != m_layers.end())
      ++after_it;
  }
  m_layers.insert(after_it, layer);

  layer->setParent(this);
}

void LayerGroup::stackLayer(Layer* layer, Layer* after)
{
  ASSERT(layer != after);
  if (layer == after)
    return;

  removeLayer(layer);
  insertLayer(layer, after);
}

void LayerGroup::displaceFrames(frame_t fromThis, frame_t delta)
{
  for (Layer* layer : m_layers)
    layer->displaceFrames(fromThis, delta);
}

} // namespace doc
