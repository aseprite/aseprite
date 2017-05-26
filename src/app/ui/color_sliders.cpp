// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color_utils.h"
#include "app/ui/color_sliders.h"
#include "app/ui/skin/skin_slider_property.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "gfx/hsl.h"
#include "gfx/rgb.h"
#include "ui/box.h"
#include "ui/entry.h"
#include "ui/graphics.h"
#include "ui/label.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/slider.h"
#include "ui/theme.h"

#include <climits>

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
      gfx::Color color = gfx::ColorNone;
      int w = MAX(rc.w-1, 1);

      for (int x=0; x <= w; ++x) {
        switch (m_channel) {
          case ColorSliders::Red:
            color = gfx::rgba(255 * x / w, m_color.getGreen(), m_color.getBlue());
            break;
          case ColorSliders::Green:
            color = gfx::rgba(m_color.getRed(), 255 * x / w, m_color.getBlue());
            break;
          case ColorSliders::Blue:
            color = gfx::rgba(m_color.getRed(), m_color.getGreen(), 255 * x / w);
            break;
          case ColorSliders::Hue:
            color = color_utils::color_for_ui(app::Color::fromHsv(360 * x / w, m_color.getSaturation(), m_color.getValue()));
            break;
          case ColorSliders::Saturation:
            color = color_utils::color_for_ui(app::Color::fromHsv(m_color.getHue(), 100 * x / w, m_color.getValue()));
            break;
          case ColorSliders::Value:
            color = color_utils::color_for_ui(app::Color::fromHsv(m_color.getHue(), m_color.getSaturation(), 100 * x / w));
            break;
          case ColorSliders::Gray:
          case ColorSliders::Alpha:
            color = color_utils::color_for_ui(app::Color::fromGray(255 * x / w));
            break;
        }
        g->drawVLine(color, rc.x+x, rc.y, rc.h);
      }
    }

  private:
    ColorSliders::Channel m_channel;
    app::Color m_color;
  };

  class ColorEntry : public Entry {
  public:
    ColorEntry(Slider* absSlider, Slider* relSlider)
      : Entry(4, "0")
      , m_absSlider(absSlider)
      , m_relSlider(relSlider)
      , m_recent_focus(false) {
    }

  private:
    int minValue() const {
      if (m_absSlider->isVisible())
        return m_absSlider->getMinValue();
      else if (m_relSlider->isVisible())
        return m_relSlider->getMinValue();
      else
        return 0;
    }

    int maxValue() const {
      if (m_absSlider->isVisible())
        return m_absSlider->getMaxValue();
      else if (m_relSlider->isVisible())
        return m_relSlider->getMaxValue();
      else
        return 0;
    }

    bool onProcessMessage(Message* msg) override {
      switch (msg->type()) {

        case kFocusEnterMessage:
          m_recent_focus = true;
          break;

        case kKeyDownMessage:
          if (Entry::onProcessMessage(msg))
            return true;

          if (hasFocus()) {
            int scancode = static_cast<KeyMessage*>(msg)->scancode();

            switch (scancode) {
              // Enter just remove the focus
              case kKeyEnter:
              case kKeyEnterPad:
                releaseFocus();
                return true;

              case kKeyDown:
              case kKeyUp: {
                int value = textInt();
                if (scancode == kKeyDown)
                  --value;
                else
                  ++value;

                setTextf("%d", MID(minValue(), value, maxValue()));
                selectAllText();

                onChange();
                return true;
              }
            }

            // Process focus movement key here because if our
            // CustomizedGuiManager catches this kKeyDownMessage it
            // will process it as a shortcut to switch the Timeline.
            //
            // Note: The default ui::Manager handles focus movement
            // shortcuts only for foreground windows.
            // TODO maybe that should change
            if (hasFocus() &&
                manager()->processFocusMovementMessage(msg))
              return true;
          }
          return false;
      }

      bool result = Entry::onProcessMessage(msg);

      if (msg->type() == kMouseDownMessage && m_recent_focus) {
        m_recent_focus = false;
        selectAllText();
      }

      return result;
    }

    Slider* m_absSlider;
    Slider* m_relSlider;

    // TODO remove this calling setFocus() in
    //      Widget::onProcessMessage() instead of
    //      Manager::handleWindowZOrder()
    bool m_recent_focus;
  };

}

//////////////////////////////////////////////////////////////////////
// ColorSliders

ColorSliders::ColorSliders()
  : Widget(kGenericWidget)
  , m_grid(3, false)
  , m_mode(Absolute)
  , m_lockEntry(-1)
{
  addChild(&m_grid);
  m_grid.setChildSpacing(0);
}

ColorSliders::~ColorSliders()
{
}

void ColorSliders::setColor(const app::Color& color)
{
  onSetColor(color);

  updateSlidersBgColor(color);
}

void ColorSliders::setMode(Mode mode)
{
  m_mode = mode;

  for (Slider* slider : m_absSlider)
    slider->setVisible(mode == Absolute);

  for (Slider* slider : m_relSlider)
    slider->setVisible(mode == Relative);

  resetRelativeSliders();
  layout();
}

void ColorSliders::resetRelativeSliders()
{
  for (Slider* slider : m_relSlider)
    slider->setValue(0);
}

void ColorSliders::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(m_grid.sizeHint());
}

void ColorSliders::addSlider(Channel channel, const char* labelText, int min, int max)
{
  Label*  label     = new Label(labelText);
  Slider* absSlider = new Slider(min, max, 0);
  Slider* relSlider = new Slider(min-max, max-min, 0);
  Entry*  entry     = new ColorEntry(absSlider, relSlider);

  m_label.push_back(label);
  m_absSlider.push_back(absSlider);
  m_relSlider.push_back(relSlider);
  m_entry.push_back(entry);
  m_channel.push_back(channel);

  absSlider->setProperty(SkinSliderPropertyPtr(new SkinSliderProperty(new ColorSliderBgPainter(channel))));
  absSlider->setDoubleBuffered(true);
  get_skin_property(entry)->setLook(MiniLook);

  absSlider->Change.connect(base::Bind<void>(&ColorSliders::onSliderChange, this, m_absSlider.size()-1));
  relSlider->Change.connect(base::Bind<void>(&ColorSliders::onSliderChange, this, m_relSlider.size()-1));
  entry->Change.connect(base::Bind<void>(&ColorSliders::onEntryChange, this, m_entry.size()-1));

  HBox* box = new HBox();
  box->addChild(absSlider);
  box->addChild(relSlider);
  absSlider->setFocusStop(false);
  relSlider->setFocusStop(false);
  absSlider->setExpansive(true);
  relSlider->setExpansive(true);
  relSlider->setVisible(false);

  gfx::Size sz(INT_MAX, SkinTheme::instance()->dimensions.colorSliderHeight());
  label->setMaxSize(sz);
  box->setMaxSize(sz);
  entry->setMaxSize(sz);

  m_grid.addChildInCell(label, 1, 1, LEFT | MIDDLE);
  m_grid.addChildInCell(box,   1, 1, HORIZONTAL | VERTICAL);
  m_grid.addChildInCell(entry, 1, 1, LEFT | MIDDLE);
}

void ColorSliders::setAbsSliderValue(int sliderIndex, int value)
{
  m_absSlider[sliderIndex]->setValue(value);
  updateEntryText(sliderIndex);
}

int ColorSliders::getAbsSliderValue(int sliderIndex) const
{
  return m_absSlider[sliderIndex]->getValue();
}

int ColorSliders::getRelSliderValue(int sliderIndex) const
{
  return m_relSlider[sliderIndex]->getValue();
}

void ColorSliders::onSliderChange(int i)
{
  updateEntryText(i);
  onControlChange(i);
}

void ColorSliders::onEntryChange(int i)
{
  base::ScopedValue<int> lock(m_lockEntry, i, m_lockEntry);

  // Update the slider related to the changed entry widget.
  int value = m_entry[i]->textInt();

  Slider* slider = (m_mode == Absolute ? m_absSlider[i]: m_relSlider[i]);
  value = MID(slider->getMinValue(), value, slider->getMaxValue());
  slider->setValue(value);

  onControlChange(i);
}

void ColorSliders::onControlChange(int i)
{
  // Call derived class impl of getColorFromSliders() to update the
  // background color of sliders.
  app::Color color = getColorFromSliders();

  updateSlidersBgColor(color);

  // Fire ColorChange() signal
  ColorSlidersChangeEvent ev(m_channel[i], m_mode,
                             color, m_relSlider[i]->getValue(), this);
  ColorChange(ev);
}

// Updates the entry related to the changed slider widget.
void ColorSliders::updateEntryText(int entryIndex)
{
  if (m_lockEntry == entryIndex)
    return;

  Slider* slider = (m_mode == Absolute ? m_absSlider[entryIndex]:
                                         m_relSlider[entryIndex]);

  m_entry[entryIndex]->setTextf("%d", slider->getValue());
  if (m_entry[entryIndex]->hasFocus())
    m_entry[entryIndex]->selectAllText();
}

void ColorSliders::updateSlidersBgColor(const app::Color& color)
{
  for (size_t i = 0; i < m_absSlider.size(); ++i)
    updateSliderBgColor(m_absSlider[i], color);
}

void ColorSliders::updateSliderBgColor(Slider* slider, const app::Color& color)
{
  SkinSliderPropertyPtr sliderProperty(slider->getProperty(SkinSliderProperty::Name));

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
  addSlider(Alpha, "A", 0, 255);
}

void RgbSliders::onSetColor(const app::Color& color)
{
  setAbsSliderValue(0, color.getRed());
  setAbsSliderValue(1, color.getGreen());
  setAbsSliderValue(2, color.getBlue());
  setAbsSliderValue(3, color.getAlpha());
}

app::Color RgbSliders::getColorFromSliders()
{
  return app::Color::fromRgb(getAbsSliderValue(0),
                             getAbsSliderValue(1),
                             getAbsSliderValue(2),
                             getAbsSliderValue(3));
}

//////////////////////////////////////////////////////////////////////
// HsvSliders

HsvSliders::HsvSliders()
  : ColorSliders()
{
  addSlider(Hue,        "H", 0, 360);
  addSlider(Saturation, "S", 0, 100);
  addSlider(Value,      "B", 0, 100);
  addSlider(Alpha,      "A", 0, 255);
}

void HsvSliders::onSetColor(const app::Color& color)
{
  setAbsSliderValue(0, int(color.getHue()));
  setAbsSliderValue(1, int(color.getSaturation()));
  setAbsSliderValue(2, int(color.getValue()));
  setAbsSliderValue(3, color.getAlpha());
}

app::Color HsvSliders::getColorFromSliders()
{
  return app::Color::fromHsv(getAbsSliderValue(0),
                             getAbsSliderValue(1),
                             getAbsSliderValue(2),
                             getAbsSliderValue(3));
}

//////////////////////////////////////////////////////////////////////
// HslSliders

HslSliders::HslSliders()
  : ColorSliders()
{
  addSlider(Hue,        "H", 0, 180);
  addSlider(Saturation, "S", 0, 100);
  addSlider(Value,      "L", 0, 100);
}

void HslSliders::onSetColor(const app::Color& color)
{
  gfx::Hsl hsl(gfx::Rgb(color.getRed(),
                        color.getGreen(),
                        color.getBlue()));

  setAbsSliderValue(0, hsl.hue());
  setAbsSliderValue(1, hsl.saturation() * 100.0);
  setAbsSliderValue(2, hsl.lightness() * 100.0);
}

app::Color HslSliders::getColorFromSliders()
{
  gfx::Hsl hsl(getAbsSliderValue(0),
               getAbsSliderValue(1) / 100.0,
               getAbsSliderValue(2) / 100.0);
  gfx::Rgb rgb(hsl);
  return app::Color::fromRgb(rgb.red(),
                             rgb.green(),
                             rgb.blue(), 255);
}

//////////////////////////////////////////////////////////////////////
// GraySlider

GraySlider::GraySlider()
  : ColorSliders()
{
  addSlider(Gray,  "V", 0, 255);
  addSlider(Alpha, "A", 0, 255);
}

void GraySlider::onSetColor(const app::Color& color)
{
  setAbsSliderValue(0, color.getGray());
  setAbsSliderValue(1, color.getAlpha());
}

app::Color GraySlider::getColorFromSliders()
{
  return app::Color::fromGray(getAbsSliderValue(0),
                              getAbsSliderValue(1));
}

} // namespace app
