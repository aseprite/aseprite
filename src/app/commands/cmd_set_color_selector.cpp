// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  void onLoadParams(const Params& params) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;

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
  else if (type == "wheel") {
    m_type = ColorBar::ColorSelector::WHEEL;
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

Command* CommandFactory::createSetColorSelectorCommand()
{
  return new SetColorSelectorCommand;
}

} // namespace app
