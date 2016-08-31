// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "render/zoom.h"
#include "ui/manager.h"
#include "ui/system.h"

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
  render::Zoom m_zoom;
};

ZoomCommand::ZoomCommand()
  : Command("Zoom",
            "Zoom",
            CmdUIOnlyFlag)
  , m_zoom(1, 1)
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
    m_zoom = render::Zoom::fromScale(
      std::strtod(percentage.c_str(), NULL) / 100.0);
    m_action = Set;
  }
}

bool ZoomCommand::onEnabled(Context* context)
{
  return (current_editor != NULL);
}

void ZoomCommand::onExecute(Context* context)
{
  // Use the current editor by default.
  Editor* editor = current_editor;
  gfx::Point mousePos = ui::get_mouse_position();

  // Try to use the editor above the mouse.
  ui::Widget* pick = ui::Manager::getDefault()->pick(mousePos);
  if (pick && pick->type() == editor_type())
    editor = static_cast<Editor*>(pick);

  render::Zoom zoom = editor->zoom();

  switch (m_action) {
    case In:
      zoom.in();
      break;
    case Out:
      zoom.out();
      break;
    case Set:
      zoom = m_zoom;
      break;
  }

  bool center = Preferences::instance().editor.zoomFromCenterWithKeys();

  editor->setZoomAndCenterInMouse(
    zoom, mousePos,
    (center ? Editor::ZoomBehavior::CENTER:
              Editor::ZoomBehavior::MOUSE));
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
      text += " " + base::convert_to<std::string>(int(100.0*m_zoom.scale())) + "%";
      break;
  }

  return text;
}

Command* CommandFactory::createZoomCommand()
{
  return new ZoomCommand;
}

} // namespace app
