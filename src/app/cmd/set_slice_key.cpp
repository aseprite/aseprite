// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_slice_key.h"

#include "doc/slice.h"
#include "doc/slices.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

SetSliceKey::SetSliceKey(Slice* slice, const doc::frame_t frame, const doc::SliceKey& sliceKey)
  : WithSlice(slice)
  , m_frame(frame)
  , m_newSliceKey(sliceKey)
{
  auto it = slice->getIteratorByFrame(frame);
  if (it != slice->end() && it->frame() == frame)
    m_oldSliceKey = *it->value();
}

void SetSliceKey::onExecute()
{
  if (!m_newSliceKey.isEmpty())
    slice()->insert(m_frame, m_newSliceKey);
  else
    slice()->remove(m_frame);

  slice()->incrementVersion();
  slice()->owner()->sprite()->incrementVersion();
}

void SetSliceKey::onUndo()
{
  if (!m_oldSliceKey.isEmpty())
    slice()->insert(m_frame, m_oldSliceKey);
  else
    slice()->remove(m_frame);

  slice()->incrementVersion();
  slice()->owner()->sprite()->incrementVersion();
}

}} // namespace app::cmd
