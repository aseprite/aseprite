// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/with_slice.h"

#include "app/cmd_exception.h"
#include "doc/slice.h"

namespace app { namespace cmd {

using namespace doc;

WithSlice::WithSlice(Slice* slice) : m_sliceId(slice ? slice->id() : NullId)
{
}

Slice* WithSlice::slice()
{
  Slice* slice = get<Slice>(m_sliceId);
  if (!slice)
    throw CmdException(fmt::format("Invalid undo information: slice ID {} not found", m_sliceId));
  return slice;
}

}} // namespace app::cmd
