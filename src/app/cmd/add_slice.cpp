// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_slice.h"

#include "doc/slice.h"
#include "doc/slice_io.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

AddSlice::AddSlice(Sprite* sprite, Slice* slice)
  : WithSprite(sprite)
  , WithSlice(slice)
  , m_size(0)
{
}

void AddSlice::onExecute()
{
  Sprite* sprite = this->sprite();
  Slice* slice = this->slice();

  sprite->slices().add(slice);
  sprite->incrementVersion();
}

void AddSlice::onUndo()
{
  Sprite* sprite = this->sprite();
  Slice* slice = this->slice();
  write_slice(m_stream, slice);
  m_size = size_t(m_stream.tellp());

  sprite->slices().remove(slice);
  sprite->incrementVersion();
  delete slice;
}

void AddSlice::onRedo()
{
  Sprite* sprite = this->sprite();
  Slice* slice = read_slice(m_stream);

  sprite->slices().add(slice);
  sprite->incrementVersion();

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;
}

} // namespace cmd
} // namespace app
