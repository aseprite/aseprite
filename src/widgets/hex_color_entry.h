/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2012  David Capello
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

#ifndef WIDGETS_HEX_COLOR_ENTRY_H_INCLUDED
#define WIDGETS_HEX_COLOR_ENTRY_H_INCLUDED

#include "app/color.h"
#include "base/signal.h"
#include "gui/box.h"
#include "gui/entry.h"
#include "gui/label.h"

// Little widget to show a color in hexadecimal format (as HTML).
class HexColorEntry : public Box
{
public:
  HexColorEntry();

  void setColor(const Color& color);

  // Signals
  Signal1<void, const Color&> ColorChange;

protected:
  void onEntryChange();

private:
  Label m_label;
  Entry m_entry;
};

#endif
