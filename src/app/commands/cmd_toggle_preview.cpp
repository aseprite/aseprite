/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"

namespace app {

class TogglePreviewCommand : public Command {
public:
  TogglePreviewCommand();
  Command* clone() const override { return new TogglePreviewCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

TogglePreviewCommand::TogglePreviewCommand()
  : Command("TogglePreview",
            "Toggle Preview",
            CmdUIOnlyFlag)
{
}

bool TogglePreviewCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

bool TogglePreviewCommand::onChecked(Context* context)
{
  MainWindow* mainWin = App::instance()->getMainWindow();
  if (!mainWin)
    return false;

  PreviewEditorWindow* previewWin = mainWin->getPreviewEditor();
  return (previewWin && previewWin->isVisible());
}

void TogglePreviewCommand::onExecute(Context* context)
{
  PreviewEditorWindow* previewWin =
    App::instance()->getMainWindow()->getPreviewEditor();

  bool state = previewWin->isPreviewEnabled();
  previewWin->setPreviewEnabled(!state);
}

Command* CommandFactory::createTogglePreviewCommand()
{
  return new TogglePreviewCommand;
}

} // namespace app
