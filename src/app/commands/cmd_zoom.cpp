// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/modules/editors.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "render/zoom.h"

namespace app {

class ZoomCommand : public Command {
public:
  enum Action { In, Out, Set };

  ZoomCommand();
  Command* clone() const override { return new ZoomCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Action m_action;
  int m_percentage;
};

ZoomCommand::ZoomCommand()
  : Command("Zoom",
            "Zoom",
            CmdUIOnlyFlag)
{
}

void ZoomCommand::onLoadParams(const Params& params)
{
  std::string action = params.get("action");
  if (action == "in") m_action = In;
  else if (action == "out") m_action = Out;
  else if (action == "set") m_action = Set;

  std::string percentage = params.get("percentage");
  if (!percentage.empty()) {
    m_percentage = std::strtol(percentage.c_str(), NULL, 10);
    m_action = Set;
  }
}

bool ZoomCommand::onEnabled(Context* context)
{
  return current_editor != NULL;
}

void ZoomCommand::onExecute(Context* context)
{
  render::Zoom zoom = current_editor->zoom();

  switch (m_action) {
    case In:
      zoom.in();
      break;
    case Out:
      zoom.out();
      break;
    case Set:
      switch (m_percentage) {
        case 3200: zoom = render::Zoom(32, 1); break;
        case 1600: zoom = render::Zoom(16, 1); break;
        case 800: zoom = render::Zoom(8, 1); break;
        case 400: zoom = render::Zoom(4, 1); break;
        case 200: zoom = render::Zoom(2, 1); break;
        default: zoom = render::Zoom(1, 1); break;
      }
      break;
  }

  current_editor->setEditorZoom(zoom);
}

std::string ZoomCommand::onGetFriendlyName() const
{
  std::string text = "Zoom";

  switch (m_action) {
    case In:
      text += " in";
      break;
    case Out:
      text += " out";
      break;
    case Set:
      text += " " + base::convert_to<std::string>(m_percentage) + "%";
      break;
  }

  return text;
}

Command* CommandFactory::createZoomCommand()
{
  return new ZoomCommand;
}

} // namespace app
