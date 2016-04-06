// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/sprite_wrap.h"

#include "app/cmd/set_sprite_size.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/script/image_wrap.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "app/ui_context.h"
#include "doc/site.h"
#include "doc/sprite.h"

namespace app {

SpriteWrap::SpriteWrap(app::Document* doc)
  : m_doc(doc)
  , m_view(UIContext::instance()->getFirstDocumentView(m_doc))
  , m_transaction(nullptr)
{
}

SpriteWrap::~SpriteWrap()
{
  for (auto it : m_images)
    delete it.second;

  if (m_transaction)
    delete m_transaction;
}

Transaction& SpriteWrap::transaction()
{
  if (!m_transaction) {
    m_transaction = new Transaction(UIContext::instance(),
                                    "Script Execution",
                                    ModifyDocument);
  }
  return *m_transaction;
}

void SpriteWrap::commit()
{
  for (auto it : m_images)
    it.second->commit();

  if (m_transaction) {
    m_transaction->commit();
    delete m_transaction;
    m_transaction = nullptr;
  }
}

app::Document* SpriteWrap::document()
{
  return m_doc;
}

doc::Sprite* SpriteWrap::sprite()
{
  return m_doc->sprite();
}

ImageWrap* SpriteWrap::activeImage()
{
  if (!m_view)
    return nullptr;

  doc::Site site;
  m_view->getSite(&site);
  return wrapImage(site.image());
}

ImageWrap* SpriteWrap::wrapImage(doc::Image* img)
{
  auto it = m_images.find(img->id());
  if (it != m_images.end())
    return it->second;
  else {
    auto wrap = new ImageWrap(this, img);
    m_images[img->id()] = wrap;
    return wrap;
  }
}

} // namespace app
