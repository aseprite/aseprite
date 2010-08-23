/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef WIDGETS_COLBUT_H_INCLUDED
#define WIDGETS_COLBUT_H_INCLUDED

#include "core/color.h"
#include "jinete/jbutton.h"

class Frame;

/* TODO use some JI_SIGNAL_USER */
#define SIGNAL_COLORBUTTON_CHANGE   0x10001

class ColorButton : public ButtonBase
{
public:
  ColorButton(color_t color, int imgtype);
  ~ColorButton();

  int getImgType();

  color_t getColor();
  void setColor(color_t color);

protected:
  // Events
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);

private:
  void draw();
  void openSelectorDialog();
  void closeSelectorDialog();

  color_t m_color;
  int m_imgtype;
  Frame* m_tooltip_window;
};

#endif
