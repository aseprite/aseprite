// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/with_slice.h"

#include "doc/slice.h"

namespace app { namespace cmd {

using namespace doc;

WithSlice::WithSlice(Slice* slice) : m_sliceId(slice->id())
{
}

Slice* WithSlice::slice()
{
  return get<Slice>(m_sliceId);
}

}} // namespace app::cmd
