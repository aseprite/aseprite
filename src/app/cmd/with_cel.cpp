// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/with_cel.h"

#include "app/cmd_exception.h"
#include "doc/cel.h"

namespace app { namespace cmd {

using namespace doc;

WithCel::WithCel(Cel* cel) : m_celId(cel ? cel->id() : NullId)
{
}

Cel* WithCel::cel()
{
  Cel* cel = get<Cel>(m_celId);
  if (!cel)
    throw CmdException(fmt::format("Invalid undo information: cel ID {} not found", m_celId));
  return cel;
}

}} // namespace app::cmd
