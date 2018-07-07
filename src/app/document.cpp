// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document.h"

#include "app/app.h"
#include "app/color_target.h"
#include "app/color_utils.h"
#include "app/context.h"
#include "app/context.h"
#include "app/doc_event.h"
#include "app/doc_observer.h"
#include "app/doc_undo.h"
#include "app/document_api.h"
#include "app/file/format_options.h"
#include "app/flatten.h"
#include "app/pref/preferences.h"
#include "app/util/create_cel_copy.h"
#include "base/memory.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/frame_tag.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/mask_boundaries.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include <limits>
#include <map>

namespace app {

using namespace base;
using namespace doc;

Document::Document(Sprite* sprite)
  : m_ctx(nullptr)
  , m_flags(kMaskVisible)
  , m_undo(new DocUndo)
  // Information about the file format used to load/save this document
  , m_format_options(nullptr)
  // Mask
  , m_mask(new Mask())
  , m_lastDrawingPoint(Document::NoLastDrawingPoint())
{
  setFilename("Sprite");

  if (sprite)
    sprites().add(sprite);
}

Document::~Document()
{
  removeFromContext();
}

void Document::setContext(Context* ctx)
{
  if (ctx == m_ctx)
    return;

  removeFromContext();

  m_ctx = ctx;
  if (ctx)
    ctx->documents().add(this);

  onContextChanged();
}

DocumentApi Document::getApi(Transaction& transaction)
{
  return DocumentApi(this, transaction);
}

//////////////////////////////////////////////////////////////////////
// Main properties

color_t Document::bgColor() const
{
  return color_utils::color_for_target(
    Preferences::instance().colorBar.bgColor(),
    ColorTarget(ColorTarget::BackgroundLayer,
                sprite()->pixelFormat(),
                sprite()->transparentColor()));
}

color_t Document::bgColor(Layer* layer) const
{
  if (layer->isBackground())
    return color_utils::color_for_layer(
      Preferences::instance().colorBar.bgColor(),
      layer);
  else
    return layer->sprite()->transparentColor();
}

//////////////////////////////////////////////////////////////////////
// Notifications

void Document::notifyGeneralUpdate()
{
  DocEvent ev(this);
  notify_observers<DocEvent&>(&DocObserver::onGeneralUpdate, ev);
}

void Document::notifySpritePixelsModified(Sprite* sprite, const gfx::Region& region, frame_t frame)
{
  DocEvent ev(this);
  ev.sprite(sprite);
  ev.region(region);
  ev.frame(frame);
  notify_observers<DocEvent&>(&DocObserver::onSpritePixelsModified, ev);
}

void Document::notifyExposeSpritePixels(Sprite* sprite, const gfx::Region& region)
{
  DocEvent ev(this);
  ev.sprite(sprite);
  ev.region(region);
  notify_observers<DocEvent&>(&DocObserver::onExposeSpritePixels, ev);
}

void Document::notifyLayerMergedDown(Layer* srcLayer, Layer* targetLayer)
{
  DocEvent ev(this);
  ev.sprite(srcLayer->sprite());
  ev.layer(srcLayer);
  ev.targetLayer(targetLayer);
  notify_observers<DocEvent&>(&DocObserver::onLayerMergedDown, ev);
}

void Document::notifyCelMoved(Layer* fromLayer, frame_t fromFrame, Layer* toLayer, frame_t toFrame)
{
  DocEvent ev(this);
  ev.sprite(fromLayer->sprite());
  ev.layer(fromLayer);
  ev.frame(fromFrame);
  ev.targetLayer(toLayer);
  ev.targetFrame(toFrame);
  notify_observers<DocEvent&>(&DocObserver::onCelMoved, ev);
}

void Document::notifyCelCopied(Layer* fromLayer, frame_t fromFrame, Layer* toLayer, frame_t toFrame)
{
  DocEvent ev(this);
  ev.sprite(fromLayer->sprite());
  ev.layer(fromLayer);
  ev.frame(fromFrame);
  ev.targetLayer(toLayer);
  ev.targetFrame(toFrame);
  notify_observers<DocEvent&>(&DocObserver::onCelCopied, ev);
}

void Document::notifySelectionChanged()
{
  DocEvent ev(this);
  notify_observers<DocEvent&>(&DocObserver::onSelectionChanged, ev);
}

bool Document::isModified() const
{
  return !m_undo->isSavedState();
}

bool Document::isAssociatedToFile() const
{
  return (m_flags & kAssociatedToFile) == kAssociatedToFile;
}

void Document::markAsSaved()
{
  m_undo->markSavedState();
  m_flags |= kAssociatedToFile;
}

void Document::impossibleToBackToSavedState()
{
  m_undo->impossibleToBackToSavedState();
}

bool Document::needsBackup() const
{
  // If the undo history isn't empty, the user has modified the
  // document, so we need to backup those changes.
  return m_undo->canUndo() || m_undo->canRedo();
}

bool Document::inhibitBackup() const
{
  return (m_flags & kInhibitBackup) == kInhibitBackup;
}

void Document::setInhibitBackup(const bool inhibitBackup)
{
  if (inhibitBackup)
    m_flags |= kInhibitBackup;
  else
    m_flags &= ~kInhibitBackup;
}

//////////////////////////////////////////////////////////////////////
// Loaded options from file

void Document::setFormatOptions(const base::SharedPtr<FormatOptions>& format_options)
{
  m_format_options = format_options;
}

//////////////////////////////////////////////////////////////////////
// Boundaries

void Document::generateMaskBoundaries(const Mask* mask)
{
  m_maskBoundaries.reset();

  // No mask specified? Use the current one in the document
  if (!mask) {
    if (!isMaskVisible())       // The mask is hidden
      return;                   // Done, without boundaries
    else
      mask = this->mask();      // Use the document mask
  }

  ASSERT(mask);

  if (!mask->isEmpty()) {
    m_maskBoundaries.reset(new MaskBoundaries(mask->bitmap()));
    m_maskBoundaries->offset(mask->bounds().x,
                             mask->bounds().y);
  }

  // TODO move this to the exact place where selection is modified.
  notifySelectionChanged();
}

//////////////////////////////////////////////////////////////////////
// Mask

void Document::setMask(const Mask* mask)
{
  m_mask.reset(new Mask(*mask));
  m_flags |= kMaskVisible;

  resetTransformation();
}

bool Document::isMaskVisible() const
{
  return
    (m_flags & kMaskVisible) && // The mask was not hidden by the user explicitly
    m_mask &&                   // The mask does exist
    !m_mask->isEmpty();         // The mask is not empty
}

void Document::setMaskVisible(bool visible)
{
  if (visible)
    m_flags |= kMaskVisible;
  else
    m_flags &= ~kMaskVisible;
}

//////////////////////////////////////////////////////////////////////
// Transformation

Transformation Document::getTransformation() const
{
  return m_transformation;
}

void Document::setTransformation(const Transformation& transform)
{
  m_transformation = transform;
}

void Document::resetTransformation()
{
  if (m_mask)
    m_transformation = Transformation(gfx::RectF(m_mask->bounds()));
  else
    m_transformation = Transformation();
}

//////////////////////////////////////////////////////////////////////
// Copying

void Document::copyLayerContent(const Layer* sourceLayer0, Document* destDoc, Layer* destLayer0) const
{
  LayerFlags dstFlags = sourceLayer0->flags();

  // Remove the "background" flag if the destDoc already has a background layer.
  if (((int)dstFlags & (int)LayerFlags::Background) == (int)LayerFlags::Background &&
      (destDoc->sprite()->backgroundLayer())) {
    dstFlags = (LayerFlags)((int)dstFlags & ~(int)(LayerFlags::BackgroundLayerFlags));
  }

  // Copy the layer name/flags/user data
  destLayer0->setName(sourceLayer0->name());
  destLayer0->setFlags(dstFlags);
  destLayer0->setUserData(sourceLayer0->userData());

  if (sourceLayer0->isImage() && destLayer0->isImage()) {
    const LayerImage* sourceLayer = static_cast<const LayerImage*>(sourceLayer0);
    LayerImage* destLayer = static_cast<LayerImage*>(destLayer0);

    // Copy blend mode and opacity
    destLayer->setBlendMode(sourceLayer->blendMode());
    destLayer->setOpacity(sourceLayer->opacity());

    // Copy cels
    CelConstIterator it = sourceLayer->getCelBegin();
    CelConstIterator end = sourceLayer->getCelEnd();

    std::map<ObjectId, Cel*> linked;

    for (; it != end; ++it) {
      const Cel* sourceCel = *it;
      if (sourceCel->frame() > destLayer->sprite()->lastFrame())
        break;

      base::UniquePtr<Cel> newCel;

      auto it = linked.find(sourceCel->data()->id());
      if (it != linked.end()) {
        newCel.reset(Cel::createLink(it->second));
        newCel->setFrame(sourceCel->frame());
      }
      else {
        newCel.reset(create_cel_copy(sourceCel,
                                     destLayer->sprite(),
                                     destLayer,
                                     sourceCel->frame()));
        linked.insert(std::make_pair(sourceCel->data()->id(), newCel.get()));
      }

      destLayer->addCel(newCel);
      newCel.release();
    }
  }
  else if (sourceLayer0->isGroup() && destLayer0->isGroup()) {
    const LayerGroup* sourceLayer = static_cast<const LayerGroup*>(sourceLayer0);
    LayerGroup* destLayer = static_cast<LayerGroup*>(destLayer0);

    for (Layer* sourceChild : sourceLayer->layers()) {
      base::UniquePtr<Layer> destChild(NULL);

      if (sourceChild->isImage()) {
        destChild.reset(new LayerImage(destLayer->sprite()));
        copyLayerContent(sourceChild, destDoc, destChild);
      }
      else if (sourceChild->isGroup()) {
        destChild.reset(new LayerGroup(destLayer->sprite()));
        copyLayerContent(sourceChild, destDoc, destChild);
      }
      else {
        ASSERT(false);
      }

      ASSERT(destChild != NULL);

      // Add the new layer in the sprite.

      Layer* newLayer = destChild.release();
      Layer* afterThis = destLayer->lastLayer();

      destLayer->addLayer(newLayer);
      destChild.release();

      destLayer->stackLayer(newLayer, afterThis);
    }
  }
  else  {
    ASSERT(false && "Trying to copy two incompatible layers");
  }
}

Document* Document::duplicate(DuplicateType type) const
{
  const Sprite* sourceSprite = sprite();
  base::UniquePtr<Sprite> spriteCopyPtr(new Sprite(
      sourceSprite->pixelFormat(),
      sourceSprite->width(),
      sourceSprite->height(),
      sourceSprite->palette(frame_t(0))->size()));

  base::UniquePtr<Document> documentCopy(new Document(spriteCopyPtr));
  Sprite* spriteCopy = spriteCopyPtr.release();

  spriteCopy->setTotalFrames(sourceSprite->totalFrames());

  // Copy frames duration
  for (frame_t i(0); i < sourceSprite->totalFrames(); ++i)
    spriteCopy->setFrameDuration(i, sourceSprite->frameDuration(i));

  // Copy frame tags
  for (const FrameTag* tag : sourceSprite->frameTags())
    spriteCopy->frameTags().add(new FrameTag(*tag));

  // Copy color palettes
  {
    PalettesList::const_iterator it = sourceSprite->getPalettes().begin();
    PalettesList::const_iterator end = sourceSprite->getPalettes().end();
    for (; it != end; ++it) {
      const Palette* pal = *it;
      spriteCopy->setPalette(pal, true);
    }
  }

  switch (type) {

    case DuplicateExactCopy:
      // Copy the layer group
      copyLayerContent(sourceSprite->root(), documentCopy, spriteCopy->root());

      ASSERT((spriteCopy->backgroundLayer() && sourceSprite->backgroundLayer()) ||
             (!spriteCopy->backgroundLayer() && !sourceSprite->backgroundLayer()));
      break;

    case DuplicateWithFlattenLayers:
      {
        // Flatten layers
        ASSERT(sourceSprite->root() != NULL);

        LayerImage* flatLayer = create_flatten_layer_copy
            (spriteCopy,
             sourceSprite->root(),
             gfx::Rect(0, 0, sourceSprite->width(), sourceSprite->height()),
             frame_t(0), sourceSprite->lastFrame());

        // Add and select the new flat layer
        spriteCopy->root()->addLayer(flatLayer);

        // Configure the layer as background only if the original
        // sprite has a background layer.
        if (sourceSprite->backgroundLayer() != NULL)
          flatLayer->configureAsBackground();
      }
      break;
  }

  // Copy only some flags
  documentCopy->m_flags = (m_flags & kMaskVisible);
  documentCopy->setFilename(filename());
  documentCopy->setMask(mask());
  documentCopy->generateMaskBoundaries();

  return documentCopy.release();
}

void Document::close()
{
  removeFromContext();
}

void Document::onFileNameChange()
{
  notify_observers(&DocObserver::onFileNameChanged, this);
}

void Document::onContextChanged()
{
  m_undo->setContext(context());
}

void Document::removeFromContext()
{
  if (m_ctx) {
    m_ctx->documents().remove(this);
    m_ctx = nullptr;

    onContextChanged();
  }
}

// static
gfx::Point Document::NoLastDrawingPoint()
{
  return gfx::Point(std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::min());
}

} // namespace app
