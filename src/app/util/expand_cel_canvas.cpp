/* Aseprite
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
#include "app/undoers/replace_image.h"
#include "app/undoers/set_cel_position.h"
#include "base/unique_ptr.h"
#include "raster/cel.h"
#include "raster/dirty.h"
#include "raster/layer.h"
#include "raster/primitives.h"
#include "raster/sprite.h"
#include "raster/stock.h"

namespace {

static raster::ImageBufferPtr src_buffer;
static raster::ImageBufferPtr dst_buffer;

static void destroy_buffers()
{
  src_buffer.reset(NULL);
  dst_buffer.reset(NULL);
}

static void create_buffers()
{
  if (!src_buffer) {
    app::App::instance()->Exit.connect(&destroy_buffers);

    src_buffer.reset(new raster::ImageBuffer(1));
    dst_buffer.reset(new raster::ImageBuffer(1));
  }
}

}

namespace app {

ExpandCelCanvas::ExpandCelCanvas(Context* context, TiledMode tiledMode, UndoTransaction& undo)
  : m_cel(NULL)
  , m_celImage(NULL)
  , m_celCreated(false)
  , m_closed(false)
  , m_committed(false)
  , m_undo(undo)
{
  create_buffers();

  DocumentLocation location = context->getActiveLocation();
  m_document = location.document();
  m_sprite = location.sprite();
  m_layer = location.layer();

  if (m_layer->isImage()) {
    m_cel = static_cast<LayerImage*>(m_layer)->getCel(location.frame());
    if (m_cel)
      m_celImage = m_sprite->getStock()->getImage(m_cel->getImage());
  }

  // If there is no Cel
  if (m_cel == NULL) {
    // Create the image
    m_celImage = Image::create(m_sprite->getPixelFormat(), m_sprite->getWidth(),
                               m_sprite->getHeight());
    clear_image(m_celImage, m_sprite->getTransparentColor());

    // Create the cel
    m_cel = new Cel(location.frame(), 0);
    static_cast<LayerImage*>(m_layer)->addCel(m_cel);

    m_celCreated = true;
  }

  m_originalCelX = m_cel->getX();
  m_originalCelY = m_cel->getY();

  // Region to draw
  int x1, y1, x2, y2;

  if (tiledMode == TILED_NONE) { // Non-tiled
    x1 = MIN(m_cel->getX(), 0);
    y1 = MIN(m_cel->getY(), 0);
    x2 = MAX(m_cel->getX()+m_celImage->getWidth(), m_sprite->getWidth());
    y2 = MAX(m_cel->getY()+m_celImage->getHeight(), m_sprite->getHeight());
  }
  else {                        // Tiled
    x1 = 0;
    y1 = 0;
    x2 = m_sprite->getWidth();
    y2 = m_sprite->getHeight();
  }

  // create two copies of the image region which we'll modify with the tool
  m_srcImage = crop_image(m_celImage,
    x1-m_cel->getX(),
    y1-m_cel->getY(), x2-x1, y2-y1,
    m_sprite->getTransparentColor(),
    src_buffer);

  m_dstImage = Image::createCopy(m_srcImage, dst_buffer);

  // We have to adjust the cel position to match the m_dstImage
  // position (the new m_dstImage will be used in RenderEngine to
  // draw this cel).
  m_cel->setPosition(x1, y1);
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

void ExpandCelCanvas::commit(const gfx::Rect& bounds)
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  // If the size of each image is the same, we can create an undo
  // with only the differences between both images.
  if (m_cel->getX() == m_originalCelX &&
      m_cel->getY() == m_originalCelY &&
      m_celImage->getWidth() == m_dstImage->getWidth() &&
      m_celImage->getHeight() == m_dstImage->getHeight()) {
    // Was m_celImage created in the start of the tool-loop?.
    if (m_celCreated) {
      // We can keep the m_celImage

      // We copy the destination image to the m_celImage
      copy_image(m_celImage, m_dstImage, 0, 0);

      // Add the m_celImage in the images stock of the sprite.
      m_cel->setImage(m_sprite->getStock()->addImage(m_celImage));

      // Is the undo enabled?.
      if (m_undo.isEnabled()) {
        // We can temporary remove the cel.
        static_cast<LayerImage*>(m_layer)->removeCel(m_cel);

        // We create the undo information (for the new m_celImage
        // in the stock and the new cel in the layer)...
        m_undo.pushUndoer(new undoers::AddImage(m_undo.getObjects(),
                                                m_sprite->getStock(), m_cel->getImage()));
        m_undo.pushUndoer(new undoers::AddCel(m_undo.getObjects(),
                                              m_layer, m_cel));

        // And finally we add the cel again in the layer.
        static_cast<LayerImage*>(m_layer)->addCel(m_cel);
      }
    }
    // If the m_celImage was already created before the whole process...
    else {
      // Add to the undo history the differences between m_celImage and m_dstImage
      if (m_undo.isEnabled()) {
        gfx::Rect dirtyBounds;
        if (bounds.isEmpty())
          dirtyBounds = m_celImage->getBounds();
        else
          dirtyBounds = m_celImage->getBounds().createIntersect(bounds);

        base::UniquePtr<Dirty> dirty(new Dirty(m_celImage, m_dstImage, dirtyBounds));

        dirty->saveImagePixels(m_celImage);
        if (dirty != NULL)
          m_undo.pushUndoer(new undoers::DirtyArea(m_undo.getObjects(), m_celImage, dirty));
      }

      // Copy the destination to the cel image.
      copy_image(m_celImage, m_dstImage, 0, 0);
    }
  }
  // If the size of both images are different, we have to
  // replace the entire image.
  else {
    if (m_undo.isEnabled()) {
      if (m_cel->getX() != m_originalCelX ||
          m_cel->getY() != m_originalCelY) {
        int x = m_cel->getX();
        int y = m_cel->getY();
        m_cel->setPosition(m_originalCelX, m_originalCelY);

        m_undo.pushUndoer(new undoers::SetCelPosition(m_undo.getObjects(), m_cel));

        m_cel->setPosition(x, y);
      }

      m_undo.pushUndoer(new undoers::ReplaceImage(m_undo.getObjects(),
                                                  m_sprite->getStock(), m_cel->getImage()));
    }

    // Replace the image in the stock.
    m_sprite->getStock()->replaceImage(m_cel->getImage(), m_dstImage);

    // Destroy the old cel image.
    delete m_celImage;

    // Now the m_dstImage is used, so we haven't to destroy it.
    m_dstImage = NULL;
  }

  m_committed = true;
}

void ExpandCelCanvas::rollback()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  // Here we destroy the temporary 'cel' created and restore all as it was before
  m_cel->setPosition(m_originalCelX, m_originalCelY);

  if (m_celCreated) {
    static_cast<LayerImage*>(m_layer)->removeCel(m_cel);
    delete m_cel;
    delete m_celImage;
  }

  m_closed = true;
}

} // namespace app
