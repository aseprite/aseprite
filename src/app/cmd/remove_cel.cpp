// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_cel.h"

#include "doc/cel.h"
#include "doc/layer.h"

namespace app {
namespace cmd {

using namespace doc;

RemoveCel::RemoveCel(Cel* cel)
  : AddCel(cel->layer(), cel)
{
}

void RemoveCel::onExecute()
{
  AddCel::onUndo();
}

void RemoveCel::onUndo()
{
  AddCel::onRedo();
}

void RemoveCel::onRedo()
{
  AddCel::onUndo();
}

} // namespace cmd
} // namespace app
