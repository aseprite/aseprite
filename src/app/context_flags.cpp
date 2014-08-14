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

#include "app/context_flags.h"

#include "app/context.h"
#include "app/document.h"
#include "app/document_location.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"

namespace app {

ContextFlags::ContextFlags()
{
  m_flags = 0;
}

void ContextFlags::update(Context* context)
{
  DocumentLocation location = context->activeLocation();
  Document* document = location.document();

  m_flags = 0;

  if (document) {
    m_flags |= HasActiveDocument;

    if (document->lock(Document::ReadLock)) {
      m_flags |= ActiveDocumentIsReadable;

      if (document->isMaskVisible())
        m_flags |= HasVisibleMask;

      Sprite* sprite = location.sprite();
      if (sprite) {
        m_flags |= HasActiveSprite;

        if (sprite->backgroundLayer())
          m_flags |= HasBackgroundLayer;

        Layer* layer = location.layer();
        if (layer) {
          m_flags |= HasActiveLayer;

          if (layer->isBackground())
            m_flags |= ActiveLayerIsBackground;

          if (layer->isReadable())
            m_flags |= ActiveLayerIsReadable;

          if (layer->isWritable())
            m_flags |= ActiveLayerIsWritable;

          if (layer->isImage()) {
            m_flags |= ActiveLayerIsImage;

            Cel* cel = static_cast<LayerImage*>(layer)->getCel(location.frame());
            if (cel) {
              m_flags |= HasActiveCel;

              if (cel->image())
                m_flags |= HasActiveImage;
            }
          }
        }
      }

      if (document->lockToWrite())
        m_flags |= ActiveDocumentIsWritable;

      document->unlock();
    }
  }
}

} // namespace app
