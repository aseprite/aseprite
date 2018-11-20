// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/expand_cel_canvas.h"

#include "app/app.h"
#include "app/cmd/add_cel.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/copy_region.h"
#include "app/cmd/patch_cel.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/site.h"
#include "app/transaction.h"
#include "app/util/range_utils.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"

namespace {

// We cannot have two ExpandCelCanvas instances at the same time
// (because we share ImageBuffers between them).
static app::ExpandCelCanvas* singleton = nullptr;

static doc::ImageBufferPtr src_buffer;
static doc::ImageBufferPtr dst_buffer;

static void destroy_buffers()
{
  src_buffer.reset(NULL);
  dst_buffer.reset(NULL);
}

static void create_buffers()
{
  if (!src_buffer) {
    app::App::instance()->Exit.connect(&destroy_buffers);

    src_buffer.reset(new doc::ImageBuffer(1));
    dst_buffer.reset(new doc::ImageBuffer(1));
  }
}

}

namespace app {

ExpandCelCanvas::ExpandCelCanvas(
  Site site, Layer* layer,
  TiledMode tiledMode, Transaction& transaction, Flags flags)
  : m_document(site.document())
  , m_sprite(site.sprite())
  , m_layer(layer)
  , m_frame(site.frame())
  , m_cel(NULL)
  , m_celImage(NULL)
  , m_celCreated(false)
  , m_flags(flags)
  , m_srcImage(NULL)
  , m_dstImage(NULL)
  , m_closed(false)
  , m_committed(false)
  , m_transaction(transaction)
  , m_canCompareSrcVsDst((m_flags & NeedsSource) == NeedsSource)
{
  ASSERT(!singleton);
  singleton = this;

  create_buffers();

  if (m_layer && m_layer->isImage()) {
    m_cel = m_layer->cel(site.frame());
    if (m_cel)
      m_celImage = m_cel->imageRef();
  }

  // Create a new cel
  if (!m_cel) {
    m_celCreated = true;
    m_cel = new Cel(site.frame(), ImageRef(NULL));
  }

  m_origCelPos = m_cel->position();

  // Region to draw
  gfx::Rect celBounds(
    m_cel->x(),
    m_cel->y(),
    m_celImage ? m_celImage->width(): m_sprite->width(),
    m_celImage ? m_celImage->height(): m_sprite->height());

  gfx::Rect spriteBounds(0, 0,
    m_sprite->width(),
    m_sprite->height());

  if (tiledMode == TiledMode::NONE) { // Non-tiled
    m_bounds = celBounds.createUnion(spriteBounds);
  }
  else {                         // Tiled
    m_bounds = spriteBounds;
  }

  // We have to adjust the cel position to match the m_dstImage
  // position (the new m_dstImage will be used in RenderEngine to
  // draw this cel).
  m_cel->setPosition(m_bounds.x, m_bounds.y);

  if (m_celCreated) {
    getDestCanvas();
    m_cel->data()->setImage(m_dstImage);

    if (m_layer && m_layer->isImage())
      static_cast<LayerImage*>(m_layer)->addCel(m_cel);
  }
}

ExpandCelCanvas::~ExpandCelCanvas()
{
  ASSERT(singleton == this);
  singleton = nullptr;

  try {
    if (!m_committed && !m_closed)
      rollback();
  }
  catch (...) {
    // Do nothing
  }
}

void ExpandCelCanvas::commit()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  if (!m_layer) {
    m_committed = true;
    return;
  }

  // Was the cel created in the start of the tool-loop?
  if (m_celCreated) {
    ASSERT(m_cel);
    ASSERT(!m_celImage);

    // Validate the whole m_dstImage (invalid areas are cleared, as we
    // don't have a m_celImage)
    validateDestCanvas(gfx::Region(m_bounds));

    // We can temporary remove the cel.
    if (m_layer->isImage()) {
      static_cast<LayerImage*>(m_layer)->removeCel(m_cel);

      // Add a copy of m_dstImage in the sprite's image stock
      gfx::Rect trimBounds = getTrimDstImageBounds();
      if (!trimBounds.isEmpty()) {
        ImageRef newImage(trimDstImage(trimBounds));
        ASSERT(newImage);

        m_cel->data()->setImage(newImage);
        m_cel->setPosition(m_cel->position() + trimBounds.origin());

        // And finally we add the cel again in the layer.
        m_transaction.execute(new cmd::AddCel(m_layer, m_cel));
      }
    }
    // We are selecting inside a layer group...
    else {
      // Just delete the created layer
      delete m_cel;
      m_cel = nullptr;
    }
  }
  else if (m_celImage) {
    // Restore cel position to its original position
    m_cel->setPosition(m_origCelPos);

    ASSERT(m_cel->image() == m_celImage.get());

    gfx::Region* regionToPatch = &m_validDstRegion;
    gfx::Region reduced;

    if (m_canCompareSrcVsDst) {
      ASSERT(gfx::Region().createSubtraction(m_validDstRegion, m_validSrcRegion).isEmpty());

      for (gfx::Rect rc : m_validDstRegion) {
        if (algorithm::shrink_bounds2(getSourceCanvas(),
                                      getDestCanvas(), rc, rc)) {
          reduced |= gfx::Region(rc);
        }
      }

      regionToPatch = &reduced;
    }

    if (m_layer->isBackground()) {
      m_transaction.execute(
        new cmd::CopyRegion(
          m_cel->image(),
          m_dstImage.get(),
          *regionToPatch,
          m_bounds.origin()));
    }
    else {
      m_transaction.execute(
        new cmd::PatchCel(
          m_cel,
          m_dstImage.get(),
          *regionToPatch,
          m_bounds.origin()));
    }
  }
  else {
    ASSERT(false);
  }

  m_committed = true;
}

void ExpandCelCanvas::rollback()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  // Here we destroy the temporary 'cel' created and restore all as it was before
  m_cel->setPosition(m_origCelPos);

  if (m_celCreated) {
    if (m_layer && m_layer->isImage())
      static_cast<LayerImage*>(m_layer)->removeCel(m_cel);

    delete m_cel;
    m_celImage.reset();
  }

  m_closed = true;
}

Image* ExpandCelCanvas::getSourceCanvas()
{
  ASSERT((m_flags & NeedsSource) == NeedsSource);

  if (!m_srcImage) {
    m_srcImage.reset(Image::create(m_sprite->pixelFormat(),
        m_bounds.w, m_bounds.h, src_buffer));

    m_srcImage->setMaskColor(m_sprite->transparentColor());
  }
  return m_srcImage.get();
}

Image* ExpandCelCanvas::getDestCanvas()
{
  if (!m_dstImage) {
    m_dstImage.reset(Image::create(m_sprite->pixelFormat(),
        m_bounds.w, m_bounds.h, dst_buffer));

    m_dstImage->setMaskColor(m_sprite->transparentColor());
  }
  return m_dstImage.get();
}

void ExpandCelCanvas::validateSourceCanvas(const gfx::Region& rgn)
{
  getSourceCanvas();

  gfx::Region rgnToValidate(rgn);
  rgnToValidate.offset(-m_bounds.origin());
  rgnToValidate.createSubtraction(rgnToValidate, m_validSrcRegion);
  rgnToValidate.createIntersection(rgnToValidate, gfx::Region(m_srcImage->bounds()));

  if (m_celImage) {
    gfx::Region rgnToClear;
    rgnToClear.createSubtraction(rgnToValidate,
      gfx::Region(m_celImage->bounds()
        .offset(m_origCelPos)
        .offset(-m_bounds.origin())));
    for (const auto& rc : rgnToClear)
      fill_rect(m_srcImage.get(), rc, m_srcImage->maskColor());

    for (const auto& rc : rgnToValidate)
      m_srcImage->copy(m_celImage.get(),
        gfx::Clip(rc.x, rc.y,
          rc.x+m_bounds.x-m_origCelPos.x,
          rc.y+m_bounds.y-m_origCelPos.y, rc.w, rc.h));
  }
  else {
    for (const auto& rc : rgnToValidate)
      fill_rect(m_srcImage.get(), rc, m_srcImage->maskColor());
  }

  m_validSrcRegion.createUnion(m_validSrcRegion, rgnToValidate);
}

void ExpandCelCanvas::validateDestCanvas(const gfx::Region& rgn)
{
  Image* src;
  int src_x, src_y;
  if ((m_flags & NeedsSource) == NeedsSource) {
    validateSourceCanvas(rgn);
    src = m_srcImage.get();
    src_x = m_bounds.x;
    src_y = m_bounds.y;
  }
  else {
    src = m_celImage.get();
    src_x = m_origCelPos.x;
    src_y = m_origCelPos.y;
  }

  getDestCanvas();

  gfx::Region rgnToValidate(rgn);
  rgnToValidate.offset(-m_bounds.origin());
  rgnToValidate.createSubtraction(rgnToValidate, m_validDstRegion);
  rgnToValidate.createIntersection(rgnToValidate, gfx::Region(m_dstImage->bounds()));

  if (src) {
    gfx::Region rgnToClear;
    rgnToClear.createSubtraction(rgnToValidate,
      gfx::Region(src->bounds()
        .offset(src_x, src_y)
        .offset(-m_bounds.origin())));
    for (const auto& rc : rgnToClear)
      fill_rect(m_dstImage.get(), rc, m_dstImage->maskColor());

    for (const auto& rc : rgnToValidate)
      m_dstImage->copy(src,
        gfx::Clip(rc.x, rc.y,
          rc.x+m_bounds.x-src_x,
          rc.y+m_bounds.y-src_y, rc.w, rc.h));
  }
  else {
    for (const auto& rc : rgnToValidate)
      fill_rect(m_dstImage.get(), rc, m_dstImage->maskColor());
  }

  m_validDstRegion.createUnion(m_validDstRegion, rgnToValidate);
}

void ExpandCelCanvas::invalidateDestCanvas()
{
  m_validDstRegion.clear();
}

void ExpandCelCanvas::invalidateDestCanvas(const gfx::Region& rgn)
{
  gfx::Region rgnToInvalidate(rgn);
  rgnToInvalidate.offset(-m_bounds.origin());
  m_validDstRegion.createSubtraction(m_validDstRegion, rgnToInvalidate);
}

void ExpandCelCanvas::copyValidDestToSourceCanvas(const gfx::Region& rgn)
{
  gfx::Region rgn2(rgn);
  rgn2.offset(-m_bounds.origin());
  rgn2.createIntersection(rgn2, m_validSrcRegion);
  rgn2.createIntersection(rgn2, m_validDstRegion);
  for (const auto& rc : rgn2)
    m_srcImage->copy(m_dstImage.get(),
      gfx::Clip(rc.x, rc.y, rc.x, rc.y, rc.w, rc.h));

  // We cannot compare src vs dst in this case (e.g. on tools like
  // spray and jumble that updated the source image form the modified
  // destination).
  m_canCompareSrcVsDst = false;
}

gfx::Rect ExpandCelCanvas::getTrimDstImageBounds() const
{
  if (m_layer->isBackground())
    return m_dstImage->bounds();
  else {
    gfx::Rect bounds;
    algorithm::shrink_bounds(m_dstImage.get(), bounds,
                             m_dstImage->maskColor());
    return bounds;
  }
}

ImageRef ExpandCelCanvas::trimDstImage(const gfx::Rect& bounds) const
{
  return ImageRef(
    crop_image(m_dstImage.get(),
               bounds.x, bounds.y,
               bounds.w, bounds.h,
               m_dstImage->maskColor()));
}

} // namespace app
