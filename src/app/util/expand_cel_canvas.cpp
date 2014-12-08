/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/expand_cel_canvas.h"

#include "app/app.h"
#include "app/context.h"
#include "app/document.h"
#include "app/document_location.h"
#include "app/undo_transaction.h"
#include "app/undoers/add_cel.h"
#include "app/undoers/add_image.h"
#include "app/undoers/dirty_area.h"
#include "app/undoers/modified_region.h"
#include "app/undoers/replace_image.h"
#include "app/undoers/set_cel_position.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/dirty.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "doc/stock.h"

namespace {

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

ExpandCelCanvas::ExpandCelCanvas(Context* context, TiledMode tiledMode, UndoTransaction& undo, Flags flags)
  : m_cel(NULL)
  , m_celImage(NULL)
  , m_celCreated(false)
  , m_flags(flags)
  , m_srcImage(NULL)
  , m_dstImage(NULL)
  , m_closed(false)
  , m_committed(false)
  , m_undo(undo)
{
  create_buffers();

  DocumentLocation location = context->activeLocation();
  m_document = location.document();
  m_sprite = location.sprite();
  m_layer = location.layer();

  if (m_layer->isImage()) {
    m_cel = static_cast<LayerImage*>(m_layer)->getCel(location.frame());
    if (m_cel)
      m_celImage = m_cel->image();
  }

  // If there is no Cel
  if (m_cel == NULL) {
    // Create the cel
    m_celCreated = true;
    m_cel = new Cel(location.frame(), 0);
    static_cast<LayerImage*>(m_layer)->addCel(m_cel);
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

  if (tiledMode == TILED_NONE) { // Non-tiled
    m_bounds = celBounds.createUnion(spriteBounds);
  }
  else {                         // Tiled
    m_bounds = spriteBounds;
  }

  // We have to adjust the cel position to match the m_dstImage
  // position (the new m_dstImage will be used in RenderEngine to
  // draw this cel).
  m_cel->setPosition(m_bounds.x, m_bounds.y);
}

ExpandCelCanvas::~ExpandCelCanvas()
{
  try {
    if (!m_committed && !m_closed)
      rollback();
  }
  catch (...) {
    // Do nothing
  }
  delete m_srcImage;
  delete m_dstImage;
}

void ExpandCelCanvas::commit()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  // Was the cel created in the start of the tool-loop?
  if (m_celCreated) {
    ASSERT(m_cel);
    ASSERT(!m_celImage);

    // Validate the whole m_dstImage (invalid areas are cleared, as we
    // don't have a m_celImage)
    validateDestCanvas(gfx::Region(m_bounds));

    // Add a copy of m_dstImage in the sprite's image stock
    m_cel->setImage(m_sprite->stock()->addImage(
        Image::createCopy(m_dstImage)));

    // Is the undo enabled?.
    if (m_undo.isEnabled()) {
      // We can temporary remove the cel.
      static_cast<LayerImage*>(m_layer)->removeCel(m_cel);

      // We create the undo information (for the new m_celImage
      // in the stock and the new cel in the layer)...
      m_undo.pushUndoer(new undoers::AddImage(m_undo.getObjects(),
          m_sprite->stock(), m_cel->imageIndex()));
      m_undo.pushUndoer(new undoers::AddCel(m_undo.getObjects(),
          m_layer, m_cel));

      // And finally we add the cel again in the layer.
      static_cast<LayerImage*>(m_layer)->addCel(m_cel);
    }
  }
  else if (m_celImage) {
    // If the size of each image is the same, we can create an undo
    // with only the differences between both images.
    if (m_cel->position() == m_origCelPos &&
        m_bounds.getOrigin() == m_origCelPos &&
        m_celImage->width() == m_dstImage->width() &&
        m_celImage->height() == m_dstImage->height()) {
      // Add to the undo history the differences between m_celImage and m_dstImage
      if (m_undo.isEnabled()) {
        if ((m_flags & UseModifiedRegionAsUndoInfo) == UseModifiedRegionAsUndoInfo) {
          m_undo.pushUndoer(new undoers::ModifiedRegion(
              m_undo.getObjects(), m_celImage, m_validDstRegion));
        }
        else {
          Dirty dirty(m_celImage, m_dstImage, m_validDstRegion);
          dirty.saveImagePixels(m_celImage);
          m_undo.pushUndoer(new undoers::DirtyArea(
              m_undo.getObjects(), m_celImage, &dirty));
        }
      }

      // Copy the destination to the cel image.
      copyValidDestToOriginalCel();
    }
    // If the size of both images are different, we have to
    // replace the entire image.
    else {
      if (m_undo.isEnabled()) {
        if (m_cel->position() != m_origCelPos) {
          gfx::Point newPos = m_cel->position();
          m_cel->setPosition(m_origCelPos);
          m_undo.pushUndoer(new undoers::SetCelPosition(m_undo.getObjects(), m_cel));
          m_cel->setPosition(newPos);
        }

        m_undo.pushUndoer(new undoers::ReplaceImage(m_undo.getObjects(),
            m_sprite->stock(), m_cel->imageIndex()));
      }

      // Validate the whole m_dstImage copying invalid areas from m_celImage
      validateDestCanvas(gfx::Region(m_bounds));

      // Replace the image in the stock. We need to create a copy of
      // image because m_dstImage's ImageBuffer cannot be shared.
      m_sprite->stock()->replaceImage(m_cel->imageIndex(),
        Image::createCopy(m_dstImage));

      // Destroy the old cel image.
      delete m_celImage;
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
    static_cast<LayerImage*>(m_layer)->removeCel(m_cel);
    delete m_cel;
    delete m_celImage;
  }

  m_closed = true;
}

Image* ExpandCelCanvas::getSourceCanvas()
{
  ASSERT((m_flags & NeedsSource) == NeedsSource);

  if (!m_srcImage) {
    m_srcImage = Image::create(m_sprite->pixelFormat(),
      m_bounds.w, m_bounds.h, src_buffer);

    m_srcImage->setMaskColor(m_sprite->transparentColor());
  }
  return m_srcImage;
}

Image* ExpandCelCanvas::getDestCanvas()
{
  if (!m_dstImage) {
    m_dstImage = Image::create(m_sprite->pixelFormat(),
      m_bounds.w, m_bounds.h, dst_buffer);

    m_dstImage->setMaskColor(m_sprite->transparentColor());
  }
  return m_dstImage;
}

void ExpandCelCanvas::validateSourceCanvas(const gfx::Region& rgn)
{
  getSourceCanvas();

  gfx::Region rgnToValidate(rgn);
  rgnToValidate.offset(-m_bounds.getOrigin());
  rgnToValidate.createSubtraction(rgnToValidate, m_validSrcRegion);
  rgnToValidate.createIntersection(rgnToValidate, gfx::Region(m_srcImage->bounds()));

  if (m_celImage) {
    gfx::Region rgnToClear;
    rgnToClear.createSubtraction(rgnToValidate,
      gfx::Region(m_celImage->bounds()
        .offset(m_origCelPos)
        .offset(-m_bounds.getOrigin())));
    for (const auto& rc : rgnToClear)
      fill_rect(m_srcImage, rc, m_srcImage->maskColor());

    for (const auto& rc : rgnToValidate)
      m_srcImage->copy(m_celImage, rc.x, rc.y,
        rc.x+m_bounds.x-m_origCelPos.x,
        rc.y+m_bounds.y-m_origCelPos.y, rc.w, rc.h);
  }
  else {
    for (const auto& rc : rgnToValidate)
      fill_rect(m_srcImage, rc, m_srcImage->maskColor());
  }

  m_validSrcRegion.createUnion(m_validSrcRegion, rgnToValidate);
}

void ExpandCelCanvas::validateDestCanvas(const gfx::Region& rgn)
{
  Image* src;
  int src_x, src_y;
  if ((m_flags & NeedsSource) == NeedsSource) {
    validateSourceCanvas(rgn);
    src = m_srcImage;
    src_x = m_bounds.x;
    src_y = m_bounds.y;
  }
  else {
    src = m_celImage;
    src_x = m_origCelPos.x;
    src_y = m_origCelPos.y;
  }

  getDestCanvas();

  gfx::Region rgnToValidate(rgn);
  rgnToValidate.offset(-m_bounds.getOrigin());
  rgnToValidate.createSubtraction(rgnToValidate, m_validDstRegion);
  rgnToValidate.createIntersection(rgnToValidate, gfx::Region(m_dstImage->bounds()));

  if (src) {
    gfx::Region rgnToClear;
    rgnToClear.createSubtraction(rgnToValidate,
      gfx::Region(src->bounds()
        .offset(src_x, src_y)
        .offset(-m_bounds.getOrigin())));
    for (const auto& rc : rgnToClear)
      fill_rect(m_dstImage, rc, m_dstImage->maskColor());

    for (const auto& rc : rgnToValidate)
      m_dstImage->copy(src, rc.x, rc.y,
        rc.x+m_bounds.x-src_x,
        rc.y+m_bounds.y-src_y, rc.w, rc.h);
  }
  else {
    for (const auto& rc : rgnToValidate)
      fill_rect(m_dstImage, rc, m_dstImage->maskColor());
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
  rgnToInvalidate.offset(-m_bounds.getOrigin());
  m_validDstRegion.createSubtraction(m_validDstRegion, rgnToInvalidate);
}

void ExpandCelCanvas::copyValidDestToSourceCanvas(const gfx::Region& rgn)
{
  gfx::Region rgn2(rgn);
  rgn2.offset(-m_bounds.getOrigin());
  rgn2.createIntersection(rgn2, m_validSrcRegion);
  rgn2.createIntersection(rgn2, m_validDstRegion);
  for (const auto& rc : rgn2)
    m_srcImage->copy(m_dstImage, rc.x, rc.y, rc.x, rc.y, rc.w, rc.h);
}

void ExpandCelCanvas::copyValidDestToOriginalCel()
{
  // Copy valid destination region to the m_celImage
  for (const auto& rc : m_validDstRegion) {
    m_celImage->copy(m_dstImage,
      rc.x-m_bounds.x+m_origCelPos.x,
      rc.y-m_bounds.y+m_origCelPos.y,
      rc.x, rc.y, rc.w, rc.h);
  }
}

} // namespace app
