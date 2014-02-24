/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/pick_ink.h"

namespace app {
namespace tools {

PickInk::PickInk(Target target) : m_target(target)
{
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
