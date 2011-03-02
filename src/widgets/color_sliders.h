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

#ifndef WIDGETS_COLOR_SLIDERS_H_INCLUDED
#define WIDGETS_COLOR_SLIDERS_H_INCLUDED

#include "app/color.h"
#include "base/signal.h"
#include "gui/grid.h"
#include "gui/widget.h"

#include <vector>

class Label;
class Slider;
class Entry;

class ColorSliders : public Widget
{
public:
  enum Channel { Red, Green, Blue,
		 Hue, Saturation, Value,
		 Gray };

  ColorSliders();
  ~ColorSliders();

  void setColor(const Color& color);

  // Signals
  Signal1<void, const Color&> ColorChange;

protected:
  void onPreferredSize(PreferredSizeEvent& ev);

  // For derived classes
  void addSlider(Channel channel, const char* labelText, int min, int max);
  void setSliderValue(int sliderIndex, int value);
  int getSliderValue(int sliderIndex) const;

  virtual void onSetColor(const Color& color) = 0;
  virtual Color getColorFromSliders() = 0;

private:
  void onSliderChange(int i);
  void onEntryChange(int i);
  void onControlChange();

  void updateSlidersBgColor(const Color& color);
  void updateSliderBgColor(Slider* slider, const Color& color);

  std::vector<Label*> m_label;
  std::vector<Slider*> m_slider;
  std::vector<Entry*> m_entry;
  Grid m_grid;
};

class RgbSliders : public ColorSliders
{
public:
  RgbSliders();

private:
  virtual void onSetColor(const Color& color);
  virtual Color getColorFromSliders();
};

class HsvSliders : public ColorSliders
{
public:
  HsvSliders();

private:
  virtual void onSetColor(const Color& color);
  virtual Color getColorFromSliders();
};

class GraySlider : public ColorSliders
{
public:
  GraySlider();

private:
  virtual void onSetColor(const Color& color);
  virtual Color getColorFromSliders();
};

#endif
