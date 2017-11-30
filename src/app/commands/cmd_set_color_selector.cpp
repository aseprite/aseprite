// Aseprite
// Copyright (C) 2016-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/ui/color_bar.h"

namespace app {

class SetColorSelectorCommand : public Command {
public:
  SetColorSelectorCommand();
  Command* clone() const override { return new SetColorSelectorCommand(*this); }

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
  : Command("SetColorSelector",
            "Set Color Selector",
            CmdUIOnlyFlag)
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
  std::string result = "Set Color Selector: ";

  switch (m_type) {
    case ColorBar::ColorSelector::SPECTRUM:
      result += "Color Spectrum";
      break;
    case ColorBar::ColorSelector::TINT_SHADE_TONE:
      result += "Color Tint/Shade/Tone";
      break;
    case ColorBar::ColorSelector::RGB_WHEEL:
      result += "RGB Color Wheel";
      break;
    case ColorBar::ColorSelector::RYB_WHEEL:
      result += "RYB Color Wheel";
      break;
    default:
      result += "Unknown";
      break;
  }

  return result;
}

Command* CommandFactory::createSetColorSelectorCommand()
{
  return new SetColorSelectorCommand;
}

} // namespace app
