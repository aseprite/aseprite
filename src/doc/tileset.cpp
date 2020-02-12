// Aseprite Document Library
// Copyright (c) 2019-2020  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/tileset.h"

#include "doc/remap.h"
#include "doc/sprite.h"

#include <memory>

namespace doc {

Tileset::Tileset(Sprite* sprite,
                 const Grid& grid,
                 const tileset_index ntiles)
  : Object(ObjectType::Tileset)
  , m_sprite(sprite)
  , m_grid(grid)
  , m_tiles(ntiles)
{
  ASSERT(sprite);
  for (tile_index ti=0; ti<ntiles; ++ti) {
    m_tiles[ti] = makeEmptyTile();
    m_hash[m_tiles[ti]] = ti;
  }
}

// static
Tileset* Tileset::MakeCopyWithSameImages(const Tileset* tileset)
{
  std::unique_ptr<Tileset> copy(
    new Tileset(tileset->sprite(),
                tileset->grid(),
                tileset->size()));
  copy->setName(tileset->name());
  for (tile_index ti=0; ti<copy->size(); ++ti) {
    ImageRef image = tileset->get(ti);
    ASSERT(image);
    copy->set(ti, image);
  }
  return copy.release();
}

// static
Tileset* Tileset::MakeCopyCopyingImages(const Tileset* tileset)
{
  std::unique_ptr<Tileset> copy(
    new Tileset(tileset->sprite(),
                tileset->grid(),
                tileset->size()));
  copy->setName(tileset->name());
  for (tile_index ti=0; ti<copy->size(); ++ti) {
    ImageRef image = tileset->get(ti);
    ASSERT(image);
    // TODO can we avoid making a copy of this image
    copy->set(ti, ImageRef(Image::createCopy(image.get())));
  }
  return copy.release();
}

void Tileset::setOrigin(const gfx::Point& pt)
{
  m_grid.origin(pt);
}

int Tileset::getMemSize() const
{
  int size = sizeof(Tileset) + m_name.size();
  for (auto& img : const_cast<Tileset*>(this)->m_tiles) {
    ASSERT(img);
    size += img->getMemSize();
  }
  return size;
}

void Tileset::resize(const tile_index ntiles)
{
  int oldSize = m_tiles.size();
  m_tiles.resize(ntiles);
  for (tile_index ti=oldSize; ti<ntiles; ++ti)
    m_tiles[ti] = makeEmptyTile();
}

void Tileset::remap(const Remap& remap)
{
  Tiles tmp = m_tiles;
  for (tile_index ti=0; ti<size(); ++ti) {
    TRACE("m_tiles[%d] = tmp[%d]\n", remap[ti], ti);
    ASSERT(remap[ti] >= 0);
    ASSERT(remap[ti] < m_tiles.size());
    if (remap[ti] >= 0 &&
        remap[ti] < m_tiles.size()) {
      m_tiles[remap[ti]] = tmp[ti];
    }
  }
}

void Tileset::set(const tile_index ti,
                  const ImageRef& image)
{
  removeFromHash(ti);

  m_tiles[ti] = image;
  m_hash[image] = ti;
}

tile_index Tileset::add(const ImageRef& image)
{
  m_tiles.push_back(image);

  const tile_index newIndex = tile_index(m_tiles.size()-1);
  m_hash[image] = newIndex;
  return newIndex;
}

void Tileset::insert(const tile_index ti,
                     const ImageRef& image)
{
  ASSERT(ti <= size());
  m_tiles.insert(m_tiles.begin()+ti, image);

  // Fix all indexes in the hash that are greater than "ti"
  for (auto& it : m_hash)
    if (it.second >= ti)
      ++it.second;

  // And now we can add the new image with the "ti" index
  m_hash[image] = ti;
}

void Tileset::erase(const tile_index ti)
{
  ASSERT(ti >= 0 && ti < size());
  removeFromHash(ti);
  m_tiles.erase(m_tiles.begin()+ti);
}

ImageRef Tileset::makeEmptyTile()
{
  ImageSpec spec = m_sprite->spec();
  spec.setSize(m_grid.tileSize());
  return ImageRef(Image::create(spec));
}

void Tileset::setExternal(const std::string& filename,
                          const tileset_index& tsi)
{
  m_external.filename = filename;
  m_external.tileset = tsi;
}

tile_index Tileset::findTileIndex(const ImageRef& tileImage)
{
  auto it = m_hash.find(tileImage);
  if (it != m_hash.end())
    return it->second;
  else
    return tile_i_notile;
}

void Tileset::removeFromHash(const tile_index ti)
{
  for (auto it=m_hash.begin(); it!=m_hash.end(); ) {
    if (it->second == ti)
      it = m_hash.erase(it);
    else {
      if (it->second > ti)
        --it->second;
      ++it;
    }
  }
}

} // namespace doc
