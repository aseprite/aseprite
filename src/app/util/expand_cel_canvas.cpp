// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
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
#include "app/cmd/copy_rect.h"
#include "app/cmd/copy_region.h"
#include "app/cmd/patch_cel.h"
#include "app/cmd_sequence.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/site.h"
#include "app/util/cel_ops.h"
#include "app/util/range_utils.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
#include "doc/tileset_hash_table.h"
#include "doc/tilesets.h"
#include "gfx/rect_io.h"
#include "render/render.h"

#define EXP_TRACE(...) // TRACEARGS

namespace {

// We cannot have two ExpandCelCanvas instances at the same time
// (because we share ImageBuffers between them).
static app::ExpandCelCanvas* singleton = nullptr;

static doc::ImageBufferPtr src_buffer;
static doc::ImageBufferPtr dst_buffer;

static void destroy_buffers()
{
  src_buffer.reset();
  dst_buffer.reset();
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
  Site site,
  Layer* layer,
  const TiledMode tiledMode,
  CmdSequence* cmds,
  const Flags flags)
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
  , m_cmds(cmds)
  , m_grid(site.grid())
  , m_tilemapMode(site.tilemapMode())
  , m_tilesetMode(site.tilesetMode())
{
  if (m_layer && m_layer->isTilemap()) {
    m_flags = Flags(m_flags | NeedsSource);
  }
  m_canCompareSrcVsDst = ((m_flags & NeedsSource) == NeedsSource);

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
  gfx::Rect celBounds = (m_celCreated ? m_sprite->bounds(): m_cel->bounds());

  gfx::Rect spriteBounds(0, 0,
    m_sprite->width(),
    m_sprite->height());

  if (tiledMode == TiledMode::NONE) { // Non-tiled
    m_bounds = celBounds.createUnion(spriteBounds);
  }
  else {                         // Tiled
    m_bounds = spriteBounds;
  }

  if (m_tilemapMode == TilemapMode::Tiles) {
    m_bounds = site.grid().canvasToTile(m_bounds);
  }

  // We have to adjust the cel position to match the m_dstImage
  // position (the new m_dstImage will be used in RenderEngine to
  // draw this cel).
  m_cel->setPosition(m_bounds.x, m_bounds.y);

  if (m_celCreated) {
    getDestCanvas();
    m_cel->data()->setImage(m_dstImage, m_layer);

    if (m_layer && m_layer->isImage())
      static_cast<LayerImage*>(m_layer)->addCel(m_cel);
  }
  // If we are in a tilemap, we use m_dstImage to draw pixels (instead
  // of the tilemap image).
  else if (m_layer->isTilemap() &&
           m_tilemapMode == TilemapMode::Pixels) {
    // Calling "getDestCanvas()" we create the m_dstImage
    getDestCanvas();
    m_cel->data()->setImage(m_dstImage, m_layer);
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
  EXP_TRACE("ExpandCelCanvas::commit",
            "validSrcRegion", m_validSrcRegion.bounds(),
            "validDstRegion", m_validDstRegion.bounds());

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

    if (m_layer->isImage()) {
      // We can temporary remove the cel.
      static_cast<LayerImage*>(m_layer)->removeCel(m_cel);

      gfx::Rect trimBounds = getTrimDstImageBounds();
      if (!trimBounds.isEmpty()) {
        // Convert the image to tiles
        if (m_layer->isTilemap() &&
            m_tilemapMode == TilemapMode::Pixels) {
          doc::ImageRef newTilemap;
          draw_image_into_new_tilemap_cel(
            m_cmds, static_cast<doc::LayerTilemap*>(m_layer), m_cel,
            // Draw the dst image in the tilemap
            m_dstImage.get(),
            m_origCelPos,
            gfx::Point(0, 0), // m_dstImage->bounds(),
            trimBounds,
            newTilemap);
        }
        else {
          ImageRef newImage(trimDstImage(trimBounds));
          ASSERT(newImage);

          m_cel->data()->setImage(newImage, m_layer);
          m_cel->setPosition(m_cel->position() + trimBounds.origin());
        }

        // And add the cel again in the layer.
        m_cmds->executeAndAdd(new cmd::AddCel(m_layer, m_cel));
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

#ifdef _DEBUG
    if (m_layer->isTilemap() &&
        m_tilemapMode == TilemapMode::Pixels) {
      ASSERT(m_cel->image() != m_celImage.get());
    }
    else {
      ASSERT(m_cel->image() == m_celImage.get());
    }
#endif

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

    EXP_TRACE(" - regionToPatch", regionToPatch->bounds());

    // Convert the image to tiles again
    if (m_layer->isTilemap() &&
        m_tilemapMode == TilemapMode::Pixels) {
      ASSERT(m_celImage.get() != m_cel->image());
      ASSERT(m_celImage->pixelFormat() == IMAGE_TILEMAP);

      // Validate the whole m_dstImage (invalid areas are cleared, as we
      // don't have a m_celImage)
      validateDestCanvas(gfx::Region(m_bounds));

      // Restore the original m_celImage, because the cel contained
      // the m_dstImage temporally for drawing purposes. No undo
      // information is required at this moment.
      m_cel->data()->setImage(m_celImage, m_layer);

      // Put the region in absolute sprite canvas coordinates (instead
      // of relative to the m_cel).
      regionToPatch->offset(m_bounds.origin());

      modify_tilemap_cel_region(
        m_cmds, m_cel,
        *regionToPatch,
        m_tilesetMode,
        [this](const doc::ImageRef& origTile,
               const gfx::Rect& tileBoundsInCanvas) -> doc::ImageRef {
          return trimDstImage(tileBoundsInCanvas);
        });
    }
    // Check that the region to copy or patch is not empty before we
    // create the new cmd
    else if (!regionToPatch->isEmpty()) {
      ASSERT(m_celImage.get() == m_cel->image());

      if (m_layer->isBackground()) {
        m_cmds->executeAndAdd(
          new cmd::CopyRegion(
            m_cel->image(),
            m_dstImage.get(),
            *regionToPatch,
            m_bounds.origin()));
      }
      else {
        m_cmds->executeAndAdd(
          new cmd::PatchCel(
            m_cel,
            m_dstImage.get(),
            *regionToPatch,
            m_bounds.origin()));
      }
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
  // Restore the original tilemap
  else if (m_layer->isTilemap()) {
    ASSERT(m_celImage->pixelFormat() == IMAGE_TILEMAP);
    m_cel->data()->setImage(m_celImage,
                            m_cel->layer());
  }

  m_closed = true;
}

Image* ExpandCelCanvas::getSourceCanvas()
{
  ASSERT((m_flags & NeedsSource) == NeedsSource);

  if (!m_srcImage) {
    m_srcImage.reset(
      Image::create(m_tilemapMode == TilemapMode::Tiles ? IMAGE_TILEMAP:
                                                          m_sprite->pixelFormat(),
                    m_bounds.w, m_bounds.h, src_buffer));

    m_srcImage->setMaskColor(m_sprite->transparentColor());
  }
  return m_srcImage.get();
}

Image* ExpandCelCanvas::getDestCanvas()
{
  if (!m_dstImage) {
    m_dstImage.reset(
      Image::create(m_tilemapMode == TilemapMode::Tiles ? IMAGE_TILEMAP:
                                                          m_sprite->pixelFormat(),
                    m_bounds.w, m_bounds.h, dst_buffer));

    m_dstImage->setMaskColor(m_sprite->transparentColor());
  }
  return m_dstImage.get();
}

void ExpandCelCanvas::validateSourceCanvas(const gfx::Region& rgn)
{
  EXP_TRACE("ExpandCelCanvas::validateSourceCanvas", rgn.bounds());

  getSourceCanvas();

  gfx::Region rgnToValidate;
  if (m_tilemapMode == TilemapMode::Tiles) {
    for (const auto& rc : rgn)
      rgnToValidate |= gfx::Region(m_grid.canvasToTile(rc));
  }
  else {
    rgnToValidate = rgn;
  }
  EXP_TRACE(" ->", rgnToValidate.bounds());

  rgnToValidate.offset(-m_bounds.origin());
  rgnToValidate.createSubtraction(rgnToValidate, m_validSrcRegion);
  rgnToValidate.createIntersection(rgnToValidate, gfx::Region(m_srcImage->bounds()));

  if (m_celImage) {
    gfx::Region rgnToClear;
    rgnToClear.createSubtraction(
      rgnToValidate,
      gfx::Region(m_celImage->bounds()
                  .offset(m_origCelPos)
                  .offset(-m_bounds.origin())));
    for (const auto& rc : rgnToClear)
      fill_rect(m_srcImage.get(), rc, m_srcImage->maskColor());

    if (m_celImage->pixelFormat() == IMAGE_TILEMAP &&
        m_srcImage->pixelFormat() != IMAGE_TILEMAP) {
      ASSERT(m_tilemapMode == TilemapMode::Pixels);

      // For tilemaps, we can use the Render class to render visible
      // tiles in the rgnToValidate of this cel.
      render::Render subRender;
      for (const auto& rc : rgnToValidate) {
        subRender.renderCel(
          m_srcImage.get(),
          m_sprite,
          m_celImage.get(),
          m_layer,
          m_sprite->palette(m_frame),
          gfx::RectF(0, 0, m_bounds.w, m_bounds.h),
          gfx::Clip(rc.x, rc.y,
                    rc.x+m_bounds.x-m_origCelPos.x,
                    rc.y+m_bounds.y-m_origCelPos.y, rc.w, rc.h),
          255, BlendMode::NORMAL);
      }
    }
    else {
      ASSERT(m_celImage->pixelFormat() != IMAGE_TILEMAP ||
             m_tilemapMode == TilemapMode::Tiles);

      // We can copy the cel image directly
      for (const auto& rc : rgnToValidate)
        m_srcImage->copy(
          m_celImage.get(),
          gfx::Clip(rc.x, rc.y,
                    rc.x+m_bounds.x-m_origCelPos.x,
                    rc.y+m_bounds.y-m_origCelPos.y, rc.w, rc.h));
    }
  }
  else {
    for (const auto& rc : rgnToValidate)
      fill_rect(m_srcImage.get(), rc, m_srcImage->maskColor());
  }

  m_validSrcRegion.createUnion(m_validSrcRegion, rgnToValidate);
}

void ExpandCelCanvas::validateDestCanvas(const gfx::Region& rgn)
{
  EXP_TRACE("ExpandCelCanvas::validateDestCanvas", rgn.bounds());

  Image* src;
  int src_x, src_y;
  if ((m_flags & NeedsSource) == NeedsSource) {
    validateSourceCanvas(rgn);
    src = m_srcImage.get();
    src_x = m_bounds.x;
    src_y = m_bounds.y;
  }
  else {
    src = m_cel->image();
    src_x = m_origCelPos.x;
    src_y = m_origCelPos.y;
  }

  getDestCanvas();              // Create m_dstImage

  gfx::Region rgnToValidate;
  if (m_tilemapMode == TilemapMode::Tiles) {
    for (const auto& rc : rgn)
      rgnToValidate |= gfx::Region(m_grid.canvasToTile(rc));
  }
  else {
    rgnToValidate = rgn;
  }
  EXP_TRACE(" ->", rgnToValidate.bounds());

  rgnToValidate.offset(-m_bounds.origin());
  rgnToValidate.createSubtraction(rgnToValidate, m_validDstRegion);
  rgnToValidate.createIntersection(rgnToValidate, gfx::Region(m_dstImage->bounds()));

  // ASSERT(src);                  // TODO is it always true?
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
  EXP_TRACE("ExpandCelCanvas::invalidateDestCanvas");
  m_validDstRegion.clear();
}

void ExpandCelCanvas::invalidateDestCanvas(const gfx::Region& rgn)
{
  EXP_TRACE("ExpandCelCanvas::invalidateDestCanvas", rgn.bounds());

  gfx::Region rgnToInvalidate(rgn);
  rgnToInvalidate.offset(-m_bounds.origin());
  m_validDstRegion.createSubtraction(m_validDstRegion, rgnToInvalidate);
}

void ExpandCelCanvas::copyValidDestToSourceCanvas(const gfx::Region& rgn)
{
  EXP_TRACE("ExpandCelCanvas::copyValidDestToSourceCanvas", rgn.bounds());

  gfx::Region rgn2(rgn);
  rgn2.offset(-m_bounds.origin());
  rgn2.createIntersection(rgn2, m_validSrcRegion);
  rgn2.createIntersection(rgn2, m_validDstRegion);
  for (const auto& rc : rgn2)
    m_srcImage->copy(m_dstImage.get(),
      gfx::Clip(rc.x, rc.y, rc.x, rc.y, rc.w, rc.h));

  // We cannot compare src vs dst in this case (e.g. on tools like
  // spray and jumble that updated the source image from the modified
  // destination).
  m_canCompareSrcVsDst = false;
}

gfx::Rect ExpandCelCanvas::getTrimDstImageBounds() const
{
  if (m_layer->isBackground())
    return m_dstImage->bounds();
  else {
    gfx::Rect bounds;
    algorithm::shrink_bounds(m_dstImage.get(),
                             m_dstImage->maskColor(), m_layer, bounds);
    return bounds;
  }
}

ImageRef ExpandCelCanvas::trimDstImage(const gfx::Rect& bounds) const
{
  return ImageRef(
    crop_image(m_dstImage.get(),
               bounds.x-m_bounds.x,
               bounds.y-m_bounds.y,
               bounds.w, bounds.h,
               m_dstImage->maskColor()));
}

} // namespace app
