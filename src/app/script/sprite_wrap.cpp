// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/sprite_wrap.h"

#include "app/app.h"
#include "app/cmd/set_sprite_size.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/script/image_wrap.h"
#include "app/site.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "doc/sprite.h"

namespace app {

SpriteWrap::SpriteWrap(Doc* doc)
  : m_doc(doc)
  , m_view(App::instance()->context()->getFirstDocumentView(m_doc))
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
    m_transaction = new Transaction(App::instance()->context(),
                                    "Script Execution",
                                    ModifyDocument);
  }
  return *m_transaction;
}

void SpriteWrap::commit()
{
  commitImages();

  if (m_transaction) {
    m_transaction->commit();
    delete m_transaction;
    m_transaction = nullptr;
  }
}

void SpriteWrap::commitImages()
{
  for (auto it : m_images)
    it.second->commit();
}

Doc* SpriteWrap::document()
{
  return m_doc;
}

doc::Sprite* SpriteWrap::sprite()
{
  return m_doc->sprite();
}

ImageWrap* SpriteWrap::activeImage()
{
  if (!m_view) {
    m_view = App::instance()->context()->getFirstDocumentView(m_doc);
    if (!m_view)
      return nullptr;
  }

#ifdef ENABLE_UI
  Site site;
  m_view->getSite(&site);
  return wrapImage(site.image());
#else
  return nullptr;
#endif
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
