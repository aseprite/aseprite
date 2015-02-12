// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/with_cel.h"

#include "doc/cel.h"

namespace app {
namespace cmd {

using namespace doc;

WithCel::WithCel(Cel* cel)
  : m_celId(cel->id())
{
}

Cel* WithCel::cel()
{
  return get<Cel>(m_celId);
}

} // namespace cmd
} // namespace app
