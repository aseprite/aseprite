// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/sprites.h"

#include "base/mutex.h"
#include "base/unique_ptr.h"
#include "doc/sprite.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/primitives.h"
#include "raster/stock.h"

#include <algorithm>

namespace doc {

Sprites::Sprites(Document* doc)
  : m_doc(doc)
{
  ASSERT(doc != NULL);
}

Sprites::~Sprites()
{
  deleteAll();
}

Sprite* Sprites::add(int width, int height, ColorMode mode, int ncolors)
{
  base::UniquePtr<Sprite> spr(
    raster::Sprite::createBasicSprite(
      (raster::PixelFormat)mode, width, height, ncolors));

  add(spr);

  return spr.release();
}

Sprite* Sprites::add(Sprite* spr)
{
  ASSERT(spr != NULL);

  m_sprites.insert(begin(), spr);

  notifyObservers(&SpritesObserver::onAddSprite, spr);
  return spr;
}

void Sprites::remove(Sprite* spr)
{
  iterator it = std::find(begin(), end(), spr);
  ASSERT(it != end());

  if (it != end())
    m_sprites.erase(it);
}

void Sprites::move(Sprite* spr, int index)
{
  remove(spr);

  m_sprites.insert(begin()+index, spr);
}

void Sprites::deleteAll()
{
  std::vector<Sprite*> copy = m_sprites;

  for (iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
    Sprite* spr = *it;
    delete spr;
  }

  m_sprites.clear();
}

} // namespace doc
