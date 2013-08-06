/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include <allegro.h>

#include "app/color_utils.h"
#include "app/ui/color_sliders.h"
#include "base/bind.h"
#include "app/ui/skin/skin_slider_property.h"
#include "ui/box.h"
#include "ui/entry.h"
#include "ui/graphics.h"
#include "ui/label.h"
#include "ui/preferred_size_event.h"
#include "ui/slider.h"

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

namespace {

  // This class is used as SkinSliderProperty for RGB/HSV sliders to
  // draw the background of them.
  class ColorSliderBgPainter : public ISliderBgPainter {
  public:
    ColorSliderBgPainter(ColorSliders::Channel channel)
      : m_channel(channel)
    { }

    void setColor(const app::Color& color) {
      m_color = color;
    }

    void paint(Slider* slider, Graphics* g, const gfx::Rect& rc) {
      ui::Color color;
      for (int x=0; x < rc.w; ++x) {
        switch (m_channel) {
          case ColorSliders::Red:
            color = ui::rgba(255 * x / (rc.w-1), m_color.getGreen(), m_color.getBlue());
            break;
          case ColorSliders::Green:
            color = ui::rgba(m_color.getRed(), 255 * x / (rc.w-1), m_color.getBlue());
            break;
          case ColorSliders::Blue:
            color = ui::rgba(m_color.getRed(), m_color.getGreen(), 255 * x / (rc.w-1));
            break;
          case ColorSliders::Hue:
            color = color_utils::color_for_ui(app::Color::fromHsv(360 * x / (rc.w-1), m_color.getSaturation(), m_color.getValue()));
            break;
          case ColorSliders::Saturation:
            color = color_utils::color_for_ui(app::Color::fromHsv(m_color.getHue(), 100 * x / (rc.w-1), m_color.getValue()));
            break;
          case ColorSliders::Value:
            color = color_utils::color_for_ui(app::Color::fromHsv(m_color.getHue(), m_color.getSaturation(), 100 * x / (rc.w-1)));
            break;
          case ColorSliders::Gray:
            color = color_utils::color_for_ui(app::Color::fromGray(255 * x / (rc.w-1)));
            break;
        }
        g->drawVLine(color, rc.x+x, rc.y, rc.h);
      }
    }

  private:
    ColorSliders::Channel m_channel;
    BITMAP* m_cachedBg;
    app::Color m_color;
  };

}

//////////////////////////////////////////////////////////////////////
// ColorSliders

ColorSliders::ColorSliders()
  : Widget(kGenericWidget)
  , m_grid(3, false)
{
  addChild(&m_grid);
}

ColorSliders::~ColorSliders()
{
}

void ColorSliders::setColor(const app::Color& color)
{
  onSetColor(color);

  updateSlidersBgColor(color);
}

void ColorSliders:: onPreferredSize(PreferredSizeEvent& ev)
{
  ev.setPreferredSize(m_grid.getPreferredSize());
}

void ColorSliders::addSlider(Channel channel, const char* labelText, int min, int max)
{
  Label*  label  = new Label(labelText);
  Slider* slider = new Slider(min, max, 0);
  Entry*  entry  = new Entry(3, "0");

  m_label.push_back(label);
  m_slider.push_back(slider);
  m_entry.push_back(entry);
  m_channel.push_back(channel);

  slider->setProperty(PropertyPtr(new SkinSliderProperty(new ColorSliderBgPainter(channel))));
  slider->setDoubleBuffered(true);

  slider->Change.connect(Bind<void>(&ColorSliders::onSliderChange, this, m_slider.size()-1));
  entry->EntryChange.connect(Bind<void>(&ColorSliders::onEntryChange, this, m_entry.size()-1));

  m_grid.addChildInCell(label,  1, 1, JI_LEFT | JI_MIDDLE);
  m_grid.addChildInCell(slider, 1, 1, JI_HORIZONTAL | JI_VERTICAL | JI_EXPANSIVE);
  m_grid.addChildInCell(entry,  1, 1, JI_LEFT | JI_MIDDLE);
}

void ColorSliders::setSliderValue(int sliderIndex, int value)
{
  Slider* slider = m_slider[sliderIndex];
  slider->setValue(value);

  updateEntryText(sliderIndex);
}

int ColorSliders::getSliderValue(int sliderIndex) const
{
  Slider* slider = m_slider[sliderIndex];

  return slider->getValue();
}

void ColorSliders::onSliderChange(int i)
{
  updateEntryText(i);
  onControlChange(i);
}

void ColorSliders::onEntryChange(int i)
{
  // Update the slider related to the changed entry widget.
  int value = m_entry[i]->getTextInt();

  value = MID(m_slider[i]->getMinValue(),
              value,
              m_slider[i]->getMaxValue());

  m_slider[i]->setValue(value);

  onControlChange(i);
}

void ColorSliders::onControlChange(int i)
{
  // Call derived class impl of getColorFromSliders() to update the
  // background color of sliders.
  app::Color color = getColorFromSliders();

  updateSlidersBgColor(color);

  // Fire ColorChange() signal
  ColorSlidersChangeEvent ev(color, m_channel[i], this);
  ColorChange(ev);
}

// Updates the entry related to the changed slider widget.
void ColorSliders::updateEntryText(int entryIndex)
{
  m_entry[entryIndex]->setTextf("%d", m_slider[entryIndex]->getValue());
}

void ColorSliders::updateSlidersBgColor(const app::Color& color)
{
  for (size_t i = 0; i < m_slider.size(); ++i)
    updateSliderBgColor(m_slider[i], color);
}

void ColorSliders::updateSliderBgColor(Slider* slider, const app::Color& color)
{
  SharedPtr<SkinSliderProperty> sliderProperty(slider->getProperty("SkinProperty"));

  static_cast<ColorSliderBgPainter*>(sliderProperty->getBgPainter())->setColor(color);

  slider->invalidate();
}

//////////////////////////////////////////////////////////////////////
// RgbSliders

RgbSliders::RgbSliders()
  : ColorSliders()
{
  addSlider(Red,   "R", 0, 255);
  addSlider(Green, "G", 0, 255);
  addSlider(Blue,  "B", 0, 255);
}

void RgbSliders::onSetColor(const app::Color& color)
{
  setSliderValue(0, color.getRed());
  setSliderValue(1, color.getGreen());
  setSliderValue(2, color.getBlue());
}

app::Color RgbSliders::getColorFromSliders()
{
  return app::Color::fromRgb(getSliderValue(0),
                             getSliderValue(1),
                             getSliderValue(2));
}

//////////////////////////////////////////////////////////////////////
// HsvSliders

HsvSliders::HsvSliders()
  : ColorSliders()
{
  addSlider(Hue,        "H", 0, 360);
  addSlider(Saturation, "S", 0, 100);
  addSlider(Value,      "B", 0, 100);
}

void HsvSliders::onSetColor(const app::Color& color)
{
  setSliderValue(0, color.getHue());
  setSliderValue(1, color.getSaturation());
  setSliderValue(2, color.getValue());
}

app::Color HsvSliders::getColorFromSliders()
{
  return app::Color::fromHsv(getSliderValue(0),
                             getSliderValue(1),
                             getSliderValue(2));
}

//////////////////////////////////////////////////////////////////////
// GraySlider

GraySlider::GraySlider()
  : ColorSliders()
{
  addSlider(Gray, "V", 0, 255);
}

void GraySlider::onSetColor(const app::Color& color)
{
  setSliderValue(0, color.getGray());
}


app::Color GraySlider::getColorFromSliders()
{
  return app::Color::fromGray(getSliderValue(0));
}

} // namespace app
