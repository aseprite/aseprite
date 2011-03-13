/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef COMMANDS_FILTERS_FILTER_PREVIEW_H_INCLUDED
#define COMMANDS_FILTERS_FILTER_PREVIEW_H_INCLUDED

#include "gui/widget.h"

class FilterManagerImpl;

// Invisible widget to control a effect-preview in the current editor.
class FilterPreview : public Widget
{
public:
  FilterPreview(FilterManagerImpl* filterMgr);
  ~FilterPreview();

  void stop();
  void restartPreview();
  FilterManagerImpl* getFilterManager() const;

protected:
  bool onProcessMessage(JMessage msg);

private:
  FilterManagerImpl* m_filterMgr;
  int m_timerId;
};

#endif
