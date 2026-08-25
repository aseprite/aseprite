// Aseprite
// Copyright (C) 2021-2026  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/new_params.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "render/zoom.h"
#include "ui/manager.h"
#include "ui/system.h"

#include <cstdint>

#ifdef ENABLE_SCRIPTING
  #include "app/script/luacpp.h"
#endif

namespace app {

enum class ZoomAction : std::uint8_t { In, Out, Set };
enum class ZoomFocus : std::uint8_t { Default, Mouse, Center };

template<>
void Param<ZoomAction>::fromString(const std::string& value)
{
  if (value == "in")
    setValue(ZoomAction::In);
  else if (value == "out")
    setValue(ZoomAction::Out);
  else if (value == "set")
    setValue(ZoomAction::Set);
}

#ifdef ENABLE_SCRIPTING
template<>
void Param<ZoomAction>::fromLua(lua_State* L, int index)
{
  fromString(lua_tostring(L, index));
}
#endif

template<>
void Param<ZoomFocus>::fromString(const std::string& value)
{
  if (value == "center")
    setValue(ZoomFocus::Center);
  else if (value == "mouse")
    setValue(ZoomFocus::Mouse);
  else
    setValue(ZoomFocus::Default);
}

#ifdef ENABLE_SCRIPTING
template<>
void Param<ZoomFocus>::fromLua(lua_State* L, int index)
{
  fromString(lua_tostring(L, index));
}
#endif

template<>
void Param<render::Zoom>::fromString(const std::string& value)
{
  if (!value.empty())
    setValue(render::Zoom::fromScale(std::strtod(value.c_str(), NULL) / 100.0));
}

#ifdef ENABLE_SCRIPTING
template<>
void Param<render::Zoom>::fromLua(lua_State* L, int index)
{
  if (lua_isnumber(L, index))
    setValue(render::Zoom::fromScale(lua_tonumber(L, index) / 100.0));
  else if (lua_isstring(L, index))
    fromString(lua_tostring(L, index));
}
#endif

struct ZoomParams : public NewParams {
  Param<ZoomAction> action{ this, ZoomAction::In, "action" };
  Param<render::Zoom> percentage{ this, render::Zoom(1, 1), "percentage" };
  Param<ZoomFocus> focus{ this, ZoomFocus::Default, "focus" };
  Param<gfx::Point> position{ this, gfx::Point(0, 0), "position" };
};

class ZoomCommand : public CommandWithNewParams<ZoomParams> {
public:
  ZoomCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;
};

ZoomCommand::ZoomCommand() : CommandWithNewParams<ZoomParams>(CommandId::Zoom())
{
}

bool ZoomCommand::onEnabled(Context* context)
{
  return (Editor::activeEditor() != nullptr);
}

void ZoomCommand::onExecute(Context* context)
{
  // Use the current editor by default.
  auto editor = Editor::activeEditor();
  gfx::Point mousePos = (params().position.isSet() ? params().position() :
                                                     ui::get_mouse_position());

  // Try to use the editor above the mouse.
  ui::Widget* pick = ui::Manager::getDefault()->pickFromScreenPos(mousePos);
  if (pick && pick->type() == Editor::Type())
    editor = static_cast<Editor*>(pick);

  render::Zoom zoom = editor->zoom();

  ZoomAction action = params().action();
  if (params().percentage.isSet())
    action = ZoomAction::Set;

  switch (action) {
    case ZoomAction::In:  zoom.in(); break;
    case ZoomAction::Out: zoom.out(); break;
    case ZoomAction::Set:
      if (params().percentage.isSet())
        zoom = params().percentage();
      break;
  }

  ZoomFocus focus = params().focus();
  if (focus == ZoomFocus::Default) {
    if (Preferences::instance().editor.zoomFromCenterWithKeys()) {
      focus = ZoomFocus::Center;
    }
    else {
      focus = ZoomFocus::Mouse;
    }
  }

  editor->setZoomAndCenterInMouse(
    zoom,
    editor->display()->nativeWindow()->pointFromScreen(mousePos),
    (focus == ZoomFocus::Center ? Editor::ZoomBehavior::CENTER : Editor::ZoomBehavior::MOUSE));
}

std::string ZoomCommand::onGetFriendlyName() const
{
  std::string text;

  ZoomAction action = params().action();
  if (params().percentage.isSet())
    action = ZoomAction::Set;

  switch (action) {
    case ZoomAction::In:  text = Strings::commands_Zoom_In(); break;
    case ZoomAction::Out: text = Strings::commands_Zoom_Out(); break;
    case ZoomAction::Set:
      text = Strings::commands_Zoom_Set(int(100.0 * params().percentage().scale()));
      break;
  }

  return text;
}

Command* CommandFactory::createZoomCommand()
{
  return new ZoomCommand;
}

} // namespace app
