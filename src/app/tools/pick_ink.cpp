// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/pick_ink.h"

namespace app {
namespace tools {

PickInk::PickInk(Target target) : m_target(target)
{
}

Ink* PickInk::clone()
{
  return new PickInk(*this);
}

bool PickInk::isEyedropper() const
{
  return true;
}

void PickInk::prepareInk(ToolLoop* loop)
{
  // Do nothing
}

void PickInk::inkHline(int x1, int y, int x2, ToolLoop* loop)
{
  // Do nothing
}

} // namespace tools
} // namespace app
