/* ASEPRITE
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

#ifndef WIDGETS_COLOR_BUTTON_H_INCLUDED
#define WIDGETS_COLOR_BUTTON_H_INCLUDED

#include "app/color.h"
#include "base/compiler_specific.h"
#include "base/signal.h"
#include "gui/button.h"
#include "raster/pixel_format.h"

class ColorSelector;

class ColorButton : public ButtonBase
{
public:
  ColorButton(const Color& color, PixelFormat pixelFormat);
  ~ColorButton();

  PixelFormat getPixelFormat() const;
  void setPixelFormat(PixelFormat pixelFormat);

  Color getColor() const;
  void setColor(const Color& color);

  // Signals
  Signal1<void, const Color&> Change;

protected:
  // Events
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
  void onPaint(PaintEvent& ev) OVERRIDE;

private:
  void openSelectorDialog();
  void closeSelectorDialog();
  void onFrameColorChange(const Color& color);

  Color m_color;
  PixelFormat m_pixelFormat;
  ColorSelector* m_frame;
};

#endif
