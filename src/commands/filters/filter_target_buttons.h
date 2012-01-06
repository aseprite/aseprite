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

#ifndef COMMANDS_FILTERS_FILTER_TARGET_BUTTONS_H_INCLUDED
#define COMMANDS_FILTERS_FILTER_TARGET_BUTTONS_H_INCLUDED

#include "base/signal.h"
#include "filters/target.h"
#include "gui/box.h"

class ButtonBase;

class FilterTargetButtons : public Box
{
public:
  // Creates a new button to handle "targets" to apply some filter in
  // the a sprite.
  FilterTargetButtons(int imgtype, bool withChannels);

  Target getTarget() const { return m_target; }
  void setTarget(Target target);

  Signal0<void> TargetChange;

protected:
  void onChannelChange(ButtonBase* button);
  void onImagesChange(ButtonBase* button);

private:
  void selectTargetButton(const char* name, Target specificTarget);
  int getTargetNormalIcon() const;
  int getTargetSelectedIcon() const;

  Target m_target;
};

#endif
