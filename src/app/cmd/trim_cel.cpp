// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/trim_cel.h"

#include "app/cmd/crop_cel.h"
#include "app/cmd/remove_cel.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"

namespace app {
namespace cmd {

using namespace doc;

TrimCel::TrimCel(Cel* cel)
{
  gfx::Rect newBounds;
  if (algorithm::shrink_bounds(cel->image(), newBounds,
                               cel->image()->maskColor())) {
    newBounds.offset(cel->position());
    m_subCmd = new cmd::CropCel(cel, newBounds);
  }
  else {
    m_subCmd = new cmd::RemoveCel(cel);
  }
}

void TrimCel::onExecute()
{
  m_subCmd->execute(context());
}

void TrimCel::onUndo()
{
  m_subCmd->undo();
}

void TrimCel::onRedo()
{
  m_subCmd->redo();
}

} // namespace cmd
} // namespace app
