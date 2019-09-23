// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "fmt/format.h"
#include "render/zoom.h"
#include "ui/manager.h"
#include "ui/system.h"

namespace app {

class ZoomCommand : public Command {
public:
  enum class Action { In, Out, Set };
  enum class Focus { Default, Mouse, Center };

  ZoomCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Action m_action;
  render::Zoom m_zoom;
  Focus m_focus;
};

ZoomCommand::ZoomCommand()
  : Command(CommandId::Zoom(), CmdUIOnlyFlag)
  , m_action(Action::In)
  , m_zoom(1, 1)
  , m_focus(Focus::Default)
{
}

void ZoomCommand::onLoadParams(const Params& params)
{
  std::string action = params.get("action");
  if (action == "in") m_action = Action::In;
  else if (action == "out") m_action = Action::Out;
  else if (action == "set") m_action = Action::Set;

  std::string percentage = params.get("percentage");
  if (!percentage.empty()) {
    m_zoom = render::Zoom::fromScale(
      std::strtod(percentage.c_str(), NULL) / 100.0);
    m_action = Action::Set;
  }

  m_focus = Focus::Default;
  std::string focus = params.get("focus");
  if (focus == "center") m_focus = Focus::Center;
  else if (focus == "mouse") m_focus = Focus::Mouse;
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
  if (pick && pick->type() == Editor::Type())
    editor = static_cast<Editor*>(pick);

  render::Zoom zoom = editor->zoom();

  switch (m_action) {
    case Action::In:
      zoom.in();
      break;
    case Action::Out:
      zoom.out();
      break;
    case Action::Set:
      zoom = m_zoom;
      break;
  }

  Focus focus = m_focus;
  if (focus == Focus::Default) {
    if (Preferences::instance().editor.zoomFromCenterWithKeys()) {
      focus = Focus::Center;
    }
    else {
      focus = Focus::Mouse;
    }
  }

  editor->setZoomAndCenterInMouse(
    zoom, mousePos,
    (focus == Focus::Center ? Editor::ZoomBehavior::CENTER:
                              Editor::ZoomBehavior::MOUSE));
}

std::string ZoomCommand::onGetFriendlyName() const
{
  std::string text;

  switch (m_action) {
    case Action::In:
      text = Strings::commands_Zoom_In();
      break;
    case Action::Out:
      text = Strings::commands_Zoom_Out();
      break;
    case Action::Set:
      text = fmt::format(Strings::commands_Zoom_Set(),
                         int(100.0*m_zoom.scale()));
      break;
  }

  return text;
}

Command* CommandFactory::createZoomCommand()
{
  return new ZoomCommand;
}

} // namespace app
