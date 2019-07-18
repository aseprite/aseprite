// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include "app/commands/filters/filter_worker.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/ui/color_button.h"
#include "app/ui/color_sliders.h"
#include "app/ui/slider2.h"
#include "base/bind.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/brightness_contrast_filter.h"
#include "ui/button.h"
#include "ui/label.h"
#include "ui/widget.h"
#include "ui/window.h"

namespace app {

struct BrightnessContrastParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<filters::Target> channels { this, 0, "channels" };
  Param<double> brightness { this, 0.0, "brightness" };
  Param<double> contrast { this, 0.0, "contrast" };
};

#ifdef ENABLE_UI

static const char* ConfigSection = "BrightnessContrast";

class BrightnessContrastWindow : public FilterWindow {
public:
  BrightnessContrastWindow(BrightnessContrastFilter& filter,
                           FilterManagerImpl& filterMgr)
    : FilterWindow("Brightness/Contrast", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_brightness(-100, 100, int(100.0 * filter.brightness()))
    , m_contrast(-100, 100, int(100.0 * filter.contrast()))
    , m_filter(filter)
  {
    getContainer()->addChild(new ui::Label("Brightness:"));
    getContainer()->addChild(&m_brightness);
    getContainer()->addChild(new ui::Label("Contrast:"));
    getContainer()->addChild(&m_contrast);
    m_brightness.Change.connect(base::Bind<void>(&BrightnessContrastWindow::onChange, this));
    m_contrast.Change.connect(base::Bind<void>(&BrightnessContrastWindow::onChange, this));
  }

private:

  void onChange() {
    m_filter.setBrightness(m_brightness.getValue() / 100.0);
    m_filter.setContrast(m_contrast.getValue() / 100.0);
    restartPreview();
  }

  Slider2 m_brightness;
  Slider2 m_contrast;
  BrightnessContrastFilter& m_filter;
};

#endif  // ENABLE_UI

class BrightnessContrastCommand : public CommandWithNewParams<BrightnessContrastParams> {
public:
  BrightnessContrastCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

BrightnessContrastCommand::BrightnessContrastCommand()
  : CommandWithNewParams<BrightnessContrastParams>(CommandId::BrightnessContrast(), CmdRecordableFlag)
{
}

bool BrightnessContrastCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void BrightnessContrastCommand::onExecute(Context* context)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && context->isUIAvailable());
#endif

  BrightnessContrastFilter filter;
  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL |
                      TARGET_ALPHA_CHANNEL);

  if (params().channels.isSet()) filterMgr.setTarget(params().channels());
  if (params().brightness.isSet()) filter.setBrightness(params().brightness() / 100.0);
  if (params().contrast.isSet()) filter.setContrast(params().contrast() / 100.0);

#ifdef ENABLE_UI
  if (ui) {
    BrightnessContrastWindow window(filter, filterMgr);
    window.doModal();
  }
  else
#endif // ENABLE_UI
  {
    start_filter_worker(&filterMgr);
  }
}

Command* CommandFactory::createBrightnessContrastCommand()
{
  return new BrightnessContrastCommand;
}

} // namespace app
