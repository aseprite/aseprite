// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/move_thing.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/modules/editors.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "fmt/format.h"
#include "ui/view.h"

namespace app {

class ScrollCommand : public Command {
public:
  ScrollCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  MoveThing m_moveThing;
};

ScrollCommand::ScrollCommand()
  : Command(CommandId::Scroll(), CmdUIOnlyFlag)
{
}

void ScrollCommand::onLoadParams(const Params& params)
{
  m_moveThing.onLoadParams(params);
}

bool ScrollCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::HasActiveDocument);
}

void ScrollCommand::onExecute(Context* context)
{
  ui::View* view = ui::View::getView(current_editor);
  gfx::Point scroll = view->viewScroll();
  gfx::Point delta = m_moveThing.getDelta(context);

  current_editor->setEditorScroll(scroll+delta);
}

std::string ScrollCommand::onGetFriendlyName() const
{
  return fmt::format(getBaseFriendlyName(),
                     m_moveThing.getFriendlyString());
}

Command* CommandFactory::createScrollCommand()
{
  return new ScrollCommand;
}

} // namespace app
