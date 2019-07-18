// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/commands/filters/filter_worker.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/color_sliders.h"
#include "base/bind.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/hue_saturation_filter.h"
#include "ui/button.h"
#include "ui/label.h"
#include "ui/slider.h"
#include "ui/widget.h"
#include "ui/window.h"

namespace app {

struct HueSaturationParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<filters::Target> channels { this, 0, "channels" };
  Param<filters::HueSaturationFilter::Mode> mode { this, filters::HueSaturationFilter::Mode::HSL, "mode" };
  Param<double> hue { this, 0.0, "hue" };
  Param<double> saturation { this, 0.0, "saturation" };
  Param<double> lightness { this, 0.0, { "lightness", "value" } };
  Param<double> alpha { this, 0.0, "alpha" };
};

#ifdef ENABLE_UI

static const char* ConfigSection = "HueSaturation";

class HueSaturationWindow : public FilterWindow {
public:
  HueSaturationWindow(HueSaturationFilter& filter,
                      FilterManagerImpl& filterMgr)
    : FilterWindow("Hue/Saturation", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_filter(filter)
    , m_colorType(2)
  {
    getContainer()->addChild(&m_colorType);
    getContainer()->addChild(&m_sliders);

    auto mode = Preferences::instance().hueSaturation.mode();
    m_colorType.addItem("HSV")->setFocusStop(false);
    m_colorType.addItem("HSL")->setFocusStop(false);
    if (mode == gen::HueSaturationMode::HSV)
      m_colorType.setSelectedItem(0);
    else
      m_colorType.setSelectedItem(1);
    m_colorType.ItemChange.connect(base::Bind<void>(&HueSaturationWindow::onChangeMode, this));

    m_sliders.setColorType(app::Color::HslType);
    m_sliders.setMode(ColorSliders::Mode::Relative);
    m_sliders.ColorChange.connect(base::Bind<void>(&HueSaturationWindow::onChangeControls, this));

    onChangeMode();
  }

private:

  bool isHsl() const {
    return (m_colorType.selectedItem() == 1);
  }

  void onChangeMode() {
    const int isHsl = this->isHsl();

    Preferences::instance().hueSaturation.mode
      (isHsl ? gen::HueSaturationMode::HSL:
               gen::HueSaturationMode::HSV);

    m_filter.setMode(isHsl ?
                     HueSaturationFilter::Mode::HSL:
                     HueSaturationFilter::Mode::HSV);

    m_sliders.setColorType(isHsl ?
                           app::Color::HslType:
                           app::Color::HsvType);

    onChangeControls();
  }

  void onChangeControls() {
    m_sliders.syncRelHsvHslSliders();

    if (isHsl()) {
      m_filter.setHue(
        double(m_sliders.getRelSliderValue(ColorSliders::Channel::HslHue)));
      m_filter.setSaturation(
        m_sliders.getRelSliderValue(ColorSliders::Channel::HslSaturation) / 100.0);
      m_filter.setLightness(
        m_sliders.getRelSliderValue(ColorSliders::Channel::HslLightness) / 100.0);
    }
    else {
      m_filter.setHue(
        double(m_sliders.getRelSliderValue(ColorSliders::Channel::HsvHue)));
      m_filter.setSaturation(
        m_sliders.getRelSliderValue(ColorSliders::Channel::HsvSaturation) / 100.0);
      m_filter.setLightness(
        m_sliders.getRelSliderValue(ColorSliders::Channel::HsvValue) / 100.0);
    }
    m_filter.setAlpha(
      m_sliders.getRelSliderValue(ColorSliders::Channel::Alpha) / 100.0);

    restartPreview();
  }

  HueSaturationFilter& m_filter;
  ButtonSet m_colorType;
  ColorSliders m_sliders;
};

#endif  // ENABLE_UI

class HueSaturationCommand : public CommandWithNewParams<HueSaturationParams> {
public:
  HueSaturationCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

HueSaturationCommand::HueSaturationCommand()
  : CommandWithNewParams<HueSaturationParams>(CommandId::HueSaturation(), CmdRecordableFlag)
{
}

bool HueSaturationCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void HueSaturationCommand::onExecute(Context* context)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && context->isUIAvailable());
#endif

  HueSaturationFilter filter;
  FilterManagerImpl filterMgr(context, &filter);
  if (params().mode.isSet()) filter.setMode(params().mode());
  if (params().hue.isSet()) filter.setHue(params().hue());
  if (params().saturation.isSet()) filter.setSaturation(params().saturation() / 100.0);
  if (params().lightness.isSet()) filter.setLightness(params().lightness() / 100.0);
  if (params().alpha.isSet()) filter.setAlpha(params().alpha() / 100.0);

  filters::Target channels =
    TARGET_RED_CHANNEL |
    TARGET_GREEN_CHANNEL |
    TARGET_BLUE_CHANNEL |
    TARGET_GRAY_CHANNEL |
    TARGET_ALPHA_CHANNEL;
  if (params().channels.isSet()) channels = params().channels();
  filterMgr.setTarget(channels);

#ifdef ENABLE_UI
  if (ui) {
    HueSaturationWindow window(filter, filterMgr);
    window.doModal();
  }
  else
#endif // ENABLE_UI
  {
    start_filter_worker(&filterMgr);
  }
}

Command* CommandFactory::createHueSaturationCommand()
{
  return new HueSaturationCommand;
}

} // namespace app
