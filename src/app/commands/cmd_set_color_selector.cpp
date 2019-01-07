// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/i18n/strings.h"
#include "app/ui/color_bar.h"
#include "fmt/format.h"

namespace app {

class SetColorSelectorCommand : public Command {
public:
  SetColorSelectorCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  ColorBar::ColorSelector m_type;
};

SetColorSelectorCommand::SetColorSelectorCommand()
  : Command(CommandId::SetColorSelector(), CmdUIOnlyFlag)
  , m_type(ColorBar::ColorSelector::SPECTRUM)
{
}

void SetColorSelectorCommand::onLoadParams(const Params& params)
{
  std::string type = params.get("type");

  if (type == "spectrum") {
    m_type = ColorBar::ColorSelector::SPECTRUM;
  }
  else if (type == "tint-shade-tone") {
    m_type = ColorBar::ColorSelector::TINT_SHADE_TONE;
  }
  else if (type == "wheel" ||
           type == "rgb-wheel") {
    m_type = ColorBar::ColorSelector::RGB_WHEEL;
  }
  else if (type == "ryb-wheel") {
    m_type = ColorBar::ColorSelector::RYB_WHEEL;
  }
  else if (type == "normal-map-wheel") {
    m_type = ColorBar::ColorSelector::NORMAL_MAP_WHEEL;
  }
}

bool SetColorSelectorCommand::onChecked(Context* context)
{
  return (ColorBar::instance()->getColorSelector() == m_type);
}

void SetColorSelectorCommand::onExecute(Context* context)
{
  ColorBar::instance()->setColorSelector(m_type);
}

std::string SetColorSelectorCommand::onGetFriendlyName() const
{
  std::string type;
  switch (m_type) {
    case ColorBar::ColorSelector::SPECTRUM:
      type = Strings::commands_SetColorSelector_Spectrum();
      break;
    case ColorBar::ColorSelector::TINT_SHADE_TONE:
      type = Strings::commands_SetColorSelector_TintShadeTone();
      break;
    case ColorBar::ColorSelector::RGB_WHEEL:
      type = Strings::commands_SetColorSelector_RGBWheel();
      break;
    case ColorBar::ColorSelector::RYB_WHEEL:
      type = Strings::commands_SetColorSelector_RYBWheel();
      break;
    case ColorBar::ColorSelector::NORMAL_MAP_WHEEL:
      type = Strings::commands_SetColorSelector_NormalMapWheel();
      break;
  }
  return fmt::format(getBaseFriendlyName() + ": {0}", type);
}

Command* CommandFactory::createSetColorSelectorCommand()
{
  return new SetColorSelectorCommand;
}

} // namespace app
