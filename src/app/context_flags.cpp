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
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "doc/stock.h"

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

          if (layer->isVisible())
            m_flags |= ActiveLayerIsVisible;

          if (layer->isEditable())
            m_flags |= ActiveLayerIsEditable;

          if (layer->isImage()) {
            m_flags |= ActiveLayerIsImage;

            Cel* cel = layer->cel(location.frame());
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
