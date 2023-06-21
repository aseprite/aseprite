// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color_spaces.h"
#include "app/color_utils.h"
#include "app/modules/gfx.h"
#include "app/ui/color_sliders.h"
#include "app/ui/expr_entry.h"
#include "app/ui/skin/skin_slider_property.h"
#include "app/ui/skin/skin_theme.h"
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

#include <algorithm>
#include <limits>

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
      // Special alpha bar (with two vertical lines)
      if (m_channel == ColorSliders::Channel::Alpha) {
        draw_alpha_slider(g, rc, m_color);
        return;
      }

      // Color space conversion
      auto convertColor = convert_from_current_to_screen_color_space();

      gfx::Color color = gfx::ColorNone;
      int w = std::max(rc.w-1, 1);

      for (int x=0; x <= w; ++x) {
        switch (m_channel) {
          case ColorSliders::Channel::Red:
            color = gfx::rgba(255 * x / w, m_color.getGreen(), m_color.getBlue());
            break;
          case ColorSliders::Channel::Green:
            color = gfx::rgba(m_color.getRed(), 255 * x / w, m_color.getBlue());
            break;
          case ColorSliders::Channel::Blue:
            color = gfx::rgba(m_color.getRed(), m_color.getGreen(), 255 * x / w);
            break;

          case ColorSliders::Channel::HsvHue:
            color = color_utils::color_for_ui(
              app::Color::fromHsv(360.0 * x / w,
                                  m_color.getHsvSaturation(),
                                  m_color.getHsvValue()));
            break;
          case ColorSliders::Channel::HsvSaturation:
            color = color_utils::color_for_ui(
              app::Color::fromHsv(m_color.getHsvHue(),
                                  double(x) / double(w),
                                  m_color.getHsvValue()));
            break;
          case ColorSliders::Channel::HsvValue:
            color = color_utils::color_for_ui(
              app::Color::fromHsv(m_color.getHsvHue(),
                                  m_color.getHsvSaturation(),
                                  double(x) / double(w)));
            break;

          case ColorSliders::Channel::HslHue:
            color = color_utils::color_for_ui(
              app::Color::fromHsl(360.0 * x / w,
                                  m_color.getHslSaturation(),
                                  m_color.getHslLightness()));
            break;
          case ColorSliders::Channel::HslSaturation:
            color = color_utils::color_for_ui(
              app::Color::fromHsl(m_color.getHslHue(),
                                  double(x) / double(w),
                                  m_color.getHslLightness()));
            break;
          case ColorSliders::Channel::HslLightness:
            color = color_utils::color_for_ui(
              app::Color::fromHsl(m_color.getHslHue(),
                                  m_color.getHslSaturation(),
                                  double(x) / double(w)));
            break;

          case ColorSliders::Channel::Gray:
            color = color_utils::color_for_ui(
              app::Color::fromGray(255 * x / w));
            break;
        }
        g->drawVLine(convertColor(color), rc.x+x, rc.y, rc.h);
      }
    }

  private:
    ColorSliders::Channel m_channel;
    app::Color m_color;
  };

  class ColorEntry : public ExprEntry {
  public:
    ColorEntry(Slider* absSlider, Slider* relSlider)
      : ExprEntry()
      , m_absSlider(absSlider)
      , m_relSlider(relSlider)
      , m_recent_focus(false) {
      setText("0");
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

                setTextf("%d", std::clamp(value, minValue(), maxValue()));
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
  , m_items(int(Channel::Channels))
  , m_grid(3, false)
  , m_mode(Mode::Absolute)
  , m_lockSlider(-1)
  , m_lockEntry(-1)
  , m_color(app::Color::fromMask())
{
  addChild(&m_grid);

  // Same order as in Channel enum
  static_assert(Channel::Red == (Channel)0, "");
  static_assert(Channel::Alpha == (Channel)10, "");
  addSlider(Channel::Red,           "R", 0, 255, -100, 100);
  addSlider(Channel::Green,         "G", 0, 255, -100, 100);
  addSlider(Channel::Blue,          "B", 0, 255, -100, 100);
  addSlider(Channel::HsvHue,        "H", 0, 360, -180, 180);
  addSlider(Channel::HsvSaturation, "S", 0, 100, -100, 100);
  addSlider(Channel::HsvValue,      "V", 0, 100, -100, 100);
  addSlider(Channel::HslHue,        "H", 0, 360, -180, 180);
  addSlider(Channel::HslSaturation, "S", 0, 100, -100, 100);
  addSlider(Channel::HslLightness,  "L", 0, 100, -100, 100);
  addSlider(Channel::Gray,          "V", 0, 255, -100, 100);
  addSlider(Channel::Alpha,         "A", 0, 255, -100, 100);

  InitTheme.connect(
    [this] {
      m_grid.setChildSpacing(0);
    }
  );

  m_appConn = App::instance()
    ->ColorSpaceChange.connect([this]{ invalidate(); });
}

void ColorSliders::setColor(const app::Color& color)
{
  m_color = color;
  onSetColor(color);
  updateSlidersBgColor();
}

void ColorSliders::setColorType(const app::Color::Type type)
{
  std::vector<app::Color::Type> types(1, type);
  setColorTypes(types);
}

void ColorSliders::setColorTypes(const std::vector<app::Color::Type>& types)
{
  for (Item& item : m_items)
    item.show = false;

  bool visible = false;
  for (auto type : types) {
    switch (type) {
      case app::Color::RgbType:
        m_items[Channel::Red].show = true;
        m_items[Channel::Green].show = true;
        m_items[Channel::Blue].show = true;
        m_items[Channel::Alpha].show = true;
        visible = true;
        break;
      case app::Color::HsvType:
        m_items[Channel::HsvHue].show = true;
        m_items[Channel::HsvSaturation].show = true;
        m_items[Channel::HsvValue].show = true;
        m_items[Channel::Alpha].show = true;
        visible = true;
        break;
      case app::Color::HslType:
        m_items[Channel::HslHue].show = true;
        m_items[Channel::HslSaturation].show = true;
        m_items[Channel::HslLightness].show = true;
        m_items[Channel::Alpha].show = true;
        visible = true;
        break;
      case app::Color::GrayType:
        m_items[Channel::Gray].show = true;
        m_items[Channel::Alpha].show = true;
        visible = true;
        break;
      case app::Color::MaskType:
      case app::Color::IndexType:
        // Do nothing
        break;
    }
  }

  setVisible(visible);

  updateSlidersVisibility();
  updateSlidersBgColor();
  layout();
}

void ColorSliders::setMode(Mode mode)
{
  m_mode = mode;

  updateSlidersVisibility();
  resetRelativeSliders();
  layout();
}

void ColorSliders::updateSlidersVisibility()
{
  for (auto& item : m_items) {
    bool v = item.show;
    item.label->setVisible(v);
    item.box->setVisible(v);
    item.entry->setVisible(v);
    item.absSlider->setVisible(v && m_mode == Mode::Absolute);
    item.relSlider->setVisible(v && m_mode == Mode::Relative);
  }
}

void ColorSliders::resetRelativeSliders()
{
  for (Item& item : m_items)
    item.relSlider->setValue(0);
}

void ColorSliders::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(m_grid.sizeHint());
}

void ColorSliders::addSlider(const Channel channel,
                             const char* labelText,
                             const int absMin, const int absMax,
                             const int relMin, const int relMax)
{
  auto theme = skin::SkinTheme::get(this);

  Item& item = m_items[channel];
  ASSERT(!item.label);
  item.label     = new Label(labelText);
  item.box       = new HBox();
  item.absSlider = new Slider(absMin, absMax, 0);
  item.relSlider = new Slider(relMin, relMax, 0);
  item.entry     = new ColorEntry(item.absSlider, item.relSlider);

  item.relSlider->setSizeHint(gfx::Size(128, 0));
  item.absSlider->setSizeHint(gfx::Size(128, 0));
  item.absSlider->setProperty(std::make_shared<SkinSliderProperty>(new ColorSliderBgPainter(channel)));
  item.absSlider->setDoubleBuffered(true);
  get_skin_property(item.entry)->setLook(MiniLook);

  item.absSlider->Change.connect([this, channel]{ onSliderChange(channel); });
  item.relSlider->Change.connect([this, channel]{ onSliderChange(channel); });
  item.entry->Change.connect([this, channel]{ onEntryChange(channel); });

  item.box->addChild(item.absSlider);
  item.box->addChild(item.relSlider);
  item.absSlider->setFocusStop(false);
  item.relSlider->setFocusStop(false);
  item.absSlider->setExpansive(true);
  item.relSlider->setExpansive(true);
  item.relSlider->setVisible(false);

  gfx::Size sz(std::numeric_limits<int>::max(),
               theme->dimensions.colorSliderHeight());
  item.label->setMaxSize(sz);
  item.box->setMaxSize(sz);
  item.entry->setMaxSize(sz);

  m_grid.addChildInCell(item.label, 1, 1, LEFT | MIDDLE);
  m_grid.addChildInCell(item.box,   1, 1, HORIZONTAL | VERTICAL);
  m_grid.addChildInCell(item.entry, 1, 1, LEFT | MIDDLE);
}

void ColorSliders::setAbsSliderValue(const Channel i, int value)
{
  if (m_lockSlider == i)
    return;

  m_items[i].absSlider->setValue(value);
  updateEntryText(i);
}

void ColorSliders::setRelSliderValue(const Channel i, int value)
{
  if (m_lockSlider == i)
    return;

  m_items[i].relSlider->setValue(value);
  updateEntryText(i);
}

int ColorSliders::getAbsSliderValue(const Channel i) const
{
  return m_items[i].absSlider->getValue();
}

int ColorSliders::getRelSliderValue(const Channel i) const
{
  return m_items[i].relSlider->getValue();
}

void ColorSliders::syncRelHsvHslSliders()
{
  // From HSV -> HSL
  if (m_items[Channel::HsvHue].show) {
    setRelSliderValue(Channel::HslHue,        getRelSliderValue(Channel::HsvHue));
    setRelSliderValue(Channel::HslSaturation, getRelSliderValue(Channel::HsvSaturation));
    setRelSliderValue(Channel::HslLightness,  getRelSliderValue(Channel::HsvValue));
  }
  // From HSL -> HSV
  else if (m_items[Channel::HslHue].show) {
    setRelSliderValue(Channel::HsvHue,        getRelSliderValue(Channel::HslHue));
    setRelSliderValue(Channel::HsvSaturation, getRelSliderValue(Channel::HslSaturation));
    setRelSliderValue(Channel::HsvValue,      getRelSliderValue(Channel::HslLightness));
  }
}

void ColorSliders::onSliderChange(const Channel i)
{
  base::ScopedValue<int> lock(m_lockSlider, i);

  updateEntryText(i);
  onControlChange(i);
}

void ColorSliders::onEntryChange(const Channel i)
{
  base::ScopedValue<int> lock(m_lockEntry, i);

  // Update the slider related to the changed entry widget.
  int value = m_items[i].entry->textInt();

  Slider* slider = (m_mode == Mode::Absolute ?
                    m_items[i].absSlider:
                    m_items[i].relSlider);
  value = std::clamp(value, slider->getMinValue(), slider->getMaxValue());
  slider->setValue(value);

  onControlChange(i);
}

void ColorSliders::onControlChange(const Channel i)
{
  m_color = getColorFromSliders(i);
  updateSlidersBgColor();

  // Fire ColorChange() signal
  ColorSlidersChangeEvent ev(i, m_mode, m_color,
                             m_items[i].relSlider->getValue(), this);
  ColorChange(ev);
}

// Updates the entry related to the changed slider widget.
void ColorSliders::updateEntryText(const Channel i)
{
  if (m_lockEntry == i)
    return;

  Slider* slider = (m_mode == Mode::Absolute ? m_items[i].absSlider:
                                               m_items[i].relSlider);

  m_items[i].entry->setTextf("%d", slider->getValue());
  if (m_items[i].entry->hasFocus())
    m_items[i].entry->selectAllText();
}

void ColorSliders::updateSlidersBgColor()
{
  for (auto& item : m_items)
    updateSliderBgColor(item.absSlider, m_color);
}

void ColorSliders::updateSliderBgColor(Slider* slider, const app::Color& color)
{
  auto sliderProperty = std::static_pointer_cast<SkinSliderProperty>(
    slider->getProperty(SkinSliderProperty::Name));

  static_cast<ColorSliderBgPainter*>(sliderProperty->getBgPainter())->setColor(color);

  slider->invalidate();
}

void ColorSliders::onSetColor(const app::Color& color)
{
  setAbsSliderValue(Channel::Red,           color.getRed());
  setAbsSliderValue(Channel::Green,         color.getGreen());
  setAbsSliderValue(Channel::Blue,          color.getBlue());
  setAbsSliderValue(Channel::HsvHue,        int(color.getHsvHue()));
  setAbsSliderValue(Channel::HsvSaturation, int(color.getHsvSaturation() * 100.0));
  setAbsSliderValue(Channel::HsvValue,      int(color.getHsvValue() * 100.0));
  setAbsSliderValue(Channel::HslHue,        int(color.getHslHue()));
  setAbsSliderValue(Channel::HslSaturation, int(color.getHslSaturation() * 100.0));
  setAbsSliderValue(Channel::HslLightness,  int(color.getHslLightness() * 100.0));
  setAbsSliderValue(Channel::Gray,          color.getGray());
  setAbsSliderValue(Channel::Alpha,         color.getAlpha());
}

app::Color ColorSliders::getColorFromSliders(const Channel channel) const
{
  // Get the color from sliders.
  switch (channel) {
    case Channel::Red:
    case Channel::Green:
    case Channel::Blue:
      return app::Color::fromRgb(
        getAbsSliderValue(Channel::Red),
        getAbsSliderValue(Channel::Green),
        getAbsSliderValue(Channel::Blue),
        getAbsSliderValue(Channel::Alpha));
    case Channel::HsvHue:
    case Channel::HsvSaturation:
    case Channel::HsvValue:
      return app::Color::fromHsv(
        getAbsSliderValue(Channel::HsvHue),
        getAbsSliderValue(Channel::HsvSaturation) / 100.0,
        getAbsSliderValue(Channel::HsvValue) / 100.0,
        getAbsSliderValue(Channel::Alpha));
    case Channel::HslHue:
    case Channel::HslSaturation:
    case Channel::HslLightness:
      return app::Color::fromHsl(
        getAbsSliderValue(Channel::HslHue),
        getAbsSliderValue(Channel::HslSaturation) / 100.0,
        getAbsSliderValue(Channel::HslLightness) / 100.0,
        getAbsSliderValue(Channel::Alpha));
    case Channel::Gray:
      return app::Color::fromGray(
        getAbsSliderValue(Channel::Gray),
        getAbsSliderValue(Channel::Alpha));
    case Channel::Alpha: {
      app::Color color = m_color;
      color.setAlpha(getAbsSliderValue(Channel::Alpha));
      return color;
    }
  }
  return app::Color::fromMask();
}

} // namespace app
