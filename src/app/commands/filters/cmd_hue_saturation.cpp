// Aseprite
// Copyright (C) 2017  David Capello
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
#include "app/context.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
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

static const char* ConfigSection = "HueSaturation";

class HueSaturationWindow : public FilterWindow {
public:
  HueSaturationWindow(HueSaturationFilter& filter,
                      FilterManagerImpl& filterMgr)
    : FilterWindow("Hue/Saturation", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_filter(filter)
  {
    getContainer()->addChild(&m_sliders);
    m_sliders.setColorType(app::Color::HslType);
    m_sliders.setMode(ColorSliders::Mode::Relative);
    m_sliders.ColorChange.connect(base::Bind<void>(&HueSaturationWindow::onChangeControls, this));
  }

private:

  void onChangeControls() {
    m_filter.setHue(
      double(m_sliders.getRelSliderValue(ColorSliders::Channel::HslHue)));

    m_filter.setSaturation(
      m_sliders.getRelSliderValue(ColorSliders::Channel::HslSaturation) / 100.0);

    m_filter.setLightness(
      m_sliders.getRelSliderValue(ColorSliders::Channel::HslLightness) / 100.0);

    m_filter.setAlpha(
      m_sliders.getRelSliderValue(ColorSliders::Channel::Alpha));

    restartPreview();
  }

  HueSaturationFilter& m_filter;
  ColorSliders m_sliders;
};

class HueSaturationCommand : public Command {
public:
  HueSaturationCommand();
  Command* clone() const override { return new HueSaturationCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

HueSaturationCommand::HueSaturationCommand()
  : Command(CommandId::HueSaturation(), CmdRecordableFlag)
{
}

bool HueSaturationCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void HueSaturationCommand::onExecute(Context* context)
{
  HueSaturationFilter filter;
  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL |
                      TARGET_ALPHA_CHANNEL);

  HueSaturationWindow window(filter, filterMgr);
  window.doModal();
}

Command* CommandFactory::createHueSaturationCommand()
{
  return new HueSaturationCommand;
}

} // namespace app
