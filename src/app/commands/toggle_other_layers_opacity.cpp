// Aseprite
// Copyright (C) 2023-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/new_params.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"
#include "fmt/format.h"
#include "ui/system.h"

namespace app {

struct ToggleOtherLayersOpacityParams : public NewParams {
  Param<bool> preview{ this, false, "preview" };
  Param<int> opacity{ this, 255, "opacity" };
  // Can be used if the option is presend as "Hide Others", so it's
  // checked when the configured opacity == 0
  Param<bool> checkedIfZero{ this, false, "checkedIfZero" };
};

class ToggleOtherLayersOpacityCommand
  : public CommandWithNewParams<ToggleOtherLayersOpacityParams> {
public:
  ToggleOtherLayersOpacityCommand();

private:
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
};

ToggleOtherLayersOpacityCommand::ToggleOtherLayersOpacityCommand()
  : CommandWithNewParams<ToggleOtherLayersOpacityParams>(CommandId::ToggleOtherLayersOpacity(),
                                                         CmdUIOnlyFlag)
{
}

bool ToggleOtherLayersOpacityCommand::onChecked(Context* ctx)
{
  if (params().checkedIfZero.isSet()) {
    auto& pref = Preferences::instance();
    auto& option = (params().preview() ? pref.experimental.nonactiveLayersOpacityPreview :
                                         pref.experimental.nonactiveLayersOpacity);
    return (option() == 0);
  }
  return false;
}

void ToggleOtherLayersOpacityCommand::onExecute(Context* ctx)
{
  auto& pref = Preferences::instance();
  auto& option = (params().preview() ? pref.experimental.nonactiveLayersOpacityPreview :
                                       pref.experimental.nonactiveLayersOpacity);

  // If we want to toggle the other layers in the preview window, and
  // the preview window is hidden, we just show it with the "other
  // layers" hidden.
  if (params().preview()) {
    PreviewEditorWindow* previewWin = App::instance()->mainWindow()->getPreviewEditor();
    if (!previewWin->isPreviewEnabled()) {
      option(0);
      previewWin->setPreviewEnabled(true);
      return;
    }
  }

  if (params().opacity.isSet()) {
    option(params().opacity());
  }
  else {
    option(option() == 0 ? 255 : 0);
  }

  // TODO make the editors listen the opacity change
  if (params().preview()) {
    PreviewEditorWindow* previewWin = App::instance()->mainWindow()->getPreviewEditor();
    if (previewWin && previewWin->previewEditor()) {
      previewWin->previewEditor()->invalidate();
    }
  }
  else {
    app_refresh_screen();
  }
}

std::string ToggleOtherLayersOpacityCommand::onGetFriendlyName() const
{
  std::string text;
  if (params().preview())
    text = Strings::commands_ToggleOtherLayersOpacity_PreviewEditor();
  else
    text = Strings::commands_ToggleOtherLayersOpacity();
  if (params().opacity.isSet())
    text = fmt::format("{} ({})", text, params().opacity());
  return text;
}

Command* CommandFactory::createToggleOtherLayersOpacityCommand()
{
  return new ToggleOtherLayersOpacityCommand;
}

} // namespace app
