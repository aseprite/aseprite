// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc.h"

#include "app/app.h"
#include "app/color_target.h"
#include "app/color_utils.h"
#include "app/context.h"
#include "app/context.h"
#include "app/doc_api.h"
#include "app/doc_event.h"
#include "app/doc_observer.h"
#include "app/doc_undo.h"
#include "app/file/format_options.h"
#include "app/flatten.h"
#include "app/pref/preferences.h"
#include "app/util/create_cel_copy.h"
#include "base/memory.h"
#include "doc/cel.h"
#include "doc/frame_tag.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/mask_boundaries.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "os/display.h"
#include "os/system.h"
#include "ui/system.h"

#include <limits>
#include <map>

namespace app {

using namespace base;
using namespace doc;

Doc::Doc(Sprite* sprite)
  : m_ctx(nullptr)
  , m_flags(kMaskVisible)
  , m_undo(new DocUndo)
  // Information about the file format used to load/save this document
  , m_format_options(nullptr)
  // Mask
  , m_mask(new Mask())
  , m_lastDrawingPoint(Doc::NoLastDrawingPoint())
{
  setFilename("Sprite");

  if (sprite)
    sprites().add(sprite);

  updateOSColorSpace(false);
}

Doc::~Doc()
{
  removeFromContext();
}

void Doc::setContext(Context* ctx)
{
  if (ctx == m_ctx)
    return;

  removeFromContext();

  m_ctx = ctx;
  if (ctx)
    ctx->documents().add(this);

  onContextChanged();
}

DocApi Doc::getApi(Transaction& transaction)
{
  return DocApi(this, transaction);
}

//////////////////////////////////////////////////////////////////////
// Main properties

color_t Doc::bgColor() const
{
  return color_utils::color_for_target(
    Preferences::instance().colorBar.bgColor(),
    ColorTarget(ColorTarget::BackgroundLayer,
                sprite()->pixelFormat(),
                sprite()->transparentColor()));
}

color_t Doc::bgColor(Layer* layer) const
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

void Doc::notifyGeneralUpdate()
{
  DocEvent ev(this);
  notify_observers<DocEvent&>(&DocObserver::onGeneralUpdate, ev);
}

void Doc::notifyColorSpaceChanged()
{
  updateOSColorSpace(true);

  DocEvent ev(this);
  ev.sprite(sprite());
  notify_observers<DocEvent&>(&DocObserver::onColorSpaceChanged, ev);
}

void Doc::notifySpritePixelsModified(Sprite* sprite, const gfx::Region& region, frame_t frame)
{
  DocEvent ev(this);
  ev.sprite(sprite);
  ev.region(region);
  ev.frame(frame);
  notify_observers<DocEvent&>(&DocObserver::onSpritePixelsModified, ev);
}

void Doc::notifyExposeSpritePixels(Sprite* sprite, const gfx::Region& region)
{
  DocEvent ev(this);
  ev.sprite(sprite);
  ev.region(region);
  notify_observers<DocEvent&>(&DocObserver::onExposeSpritePixels, ev);
}

void Doc::notifyLayerMergedDown(Layer* srcLayer, Layer* targetLayer)
{
  DocEvent ev(this);
  ev.sprite(srcLayer->sprite());
  ev.layer(srcLayer);
  ev.targetLayer(targetLayer);
  notify_observers<DocEvent&>(&DocObserver::onLayerMergedDown, ev);
}

void Doc::notifyCelMoved(Layer* fromLayer, frame_t fromFrame, Layer* toLayer, frame_t toFrame)
{
  DocEvent ev(this);
  ev.sprite(fromLayer->sprite());
  ev.layer(fromLayer);
  ev.frame(fromFrame);
  ev.targetLayer(toLayer);
  ev.targetFrame(toFrame);
  notify_observers<DocEvent&>(&DocObserver::onCelMoved, ev);
}

void Doc::notifyCelCopied(Layer* fromLayer, frame_t fromFrame, Layer* toLayer, frame_t toFrame)
{
  DocEvent ev(this);
  ev.sprite(fromLayer->sprite());
  ev.layer(fromLayer);
  ev.frame(fromFrame);
  ev.targetLayer(toLayer);
  ev.targetFrame(toFrame);
  notify_observers<DocEvent&>(&DocObserver::onCelCopied, ev);
}

void Doc::notifySelectionChanged()
{
  DocEvent ev(this);
  notify_observers<DocEvent&>(&DocObserver::onSelectionChanged, ev);
}

bool Doc::isModified() const
{
  return !m_undo->isSavedState();
}

bool Doc::isAssociatedToFile() const
{
  return (m_flags & kAssociatedToFile) == kAssociatedToFile;
}

void Doc::markAsSaved()
{
  m_undo->markSavedState();
  m_flags |= kAssociatedToFile;
}

void Doc::impossibleToBackToSavedState()
{
  m_undo->impossibleToBackToSavedState();
}

bool Doc::needsBackup() const
{
  // If the undo history isn't empty, the user has modified the
  // document, so we need to backup those changes.
  return m_undo->canUndo() || m_undo->canRedo();
}

bool Doc::inhibitBackup() const
{
  return (m_flags & kInhibitBackup) == kInhibitBackup;
}

void Doc::setInhibitBackup(const bool inhibitBackup)
{
  if (inhibitBackup)
    m_flags |= kInhibitBackup;
  else
    m_flags &= ~kInhibitBackup;
}

//////////////////////////////////////////////////////////////////////
// Loaded options from file

void Doc::setFormatOptions(const base::SharedPtr<FormatOptions>& format_options)
{
  m_format_options = format_options;
}

//////////////////////////////////////////////////////////////////////
// Boundaries

void Doc::generateMaskBoundaries(const Mask* mask)
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

void Doc::setMask(const Mask* mask)
{
  m_mask.reset(new Mask(*mask));
  m_flags |= kMaskVisible;

  resetTransformation();
}

bool Doc::isMaskVisible() const
{
  return
    (m_flags & kMaskVisible) && // The mask was not hidden by the user explicitly
    m_mask &&                   // The mask does exist
    !m_mask->isEmpty();         // The mask is not empty
}

void Doc::setMaskVisible(bool visible)
{
  if (visible)
    m_flags |= kMaskVisible;
  else
    m_flags &= ~kMaskVisible;
}

//////////////////////////////////////////////////////////////////////
// Transformation

Transformation Doc::getTransformation() const
{
  return m_transformation;
}

void Doc::setTransformation(const Transformation& transform)
{
  m_transformation = transform;
}

void Doc::resetTransformation()
{
  if (m_mask)
    m_transformation = Transformation(gfx::RectF(m_mask->bounds()));
  else
    m_transformation = Transformation();
}

//////////////////////////////////////////////////////////////////////
// Copying

void Doc::copyLayerContent(const Layer* sourceLayer0, Doc* destDoc, Layer* destLayer0) const
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

      std::unique_ptr<Cel> newCel(nullptr);

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

      destLayer->addCel(newCel.get());
      newCel.release();
    }
  }
  else if (sourceLayer0->isGroup() && destLayer0->isGroup()) {
    const LayerGroup* sourceLayer = static_cast<const LayerGroup*>(sourceLayer0);
    LayerGroup* destLayer = static_cast<LayerGroup*>(destLayer0);

    for (Layer* sourceChild : sourceLayer->layers()) {
      std::unique_ptr<Layer> destChild(nullptr);

      if (sourceChild->isImage()) {
        destChild.reset(new LayerImage(destLayer->sprite()));
        copyLayerContent(sourceChild, destDoc, destChild.get());
      }
      else if (sourceChild->isGroup()) {
        destChild.reset(new LayerGroup(destLayer->sprite()));
        copyLayerContent(sourceChild, destDoc, destChild.get());
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

Doc* Doc::duplicate(DuplicateType type) const
{
  const Sprite* sourceSprite = sprite();
  std::unique_ptr<Sprite> spriteCopyPtr(new Sprite(
      sourceSprite->spec(),
      sourceSprite->palette(frame_t(0))->size()));

  std::unique_ptr<Doc> documentCopy(new Doc(spriteCopyPtr.get()));
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
      copyLayerContent(sourceSprite->root(),
                       documentCopy.get(),
                       spriteCopy->root());

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

void Doc::close()
{
  removeFromContext();
}

void Doc::onFileNameChange()
{
  notify_observers(&DocObserver::onFileNameChanged, this);
}

void Doc::onContextChanged()
{
  m_undo->setContext(context());
}

void Doc::removeFromContext()
{
  if (m_ctx) {
    m_ctx->documents().remove(this);
    m_ctx = nullptr;

    onContextChanged();
  }
}

void Doc::updateOSColorSpace(bool appWideSignal)
{
  auto system = os::instance();
  if (system) {
    m_osColorSpace = system->createColorSpace(sprite()->colorSpace());
    if (!m_osColorSpace && system->defaultDisplay())
      m_osColorSpace = system->defaultDisplay()->colorSpace();
  }

  if (appWideSignal &&
      context() &&
      context()->activeDocument() == this) {
    App::instance()->ColorSpaceChange();
  }

  if (ui::is_ui_thread()) {
    // As the color space has changed, we might need to upate the
    // current palette (because the color space conversion might be
    // came from a cmd::ConvertColorProfile, so the palette might be
    // changed). This might generate a PaletteChange() signal.
    app_update_current_palette();
  }
}

// static
gfx::Point Doc::NoLastDrawingPoint()
{
  return gfx::Point(std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::min());
}

} // namespace app
