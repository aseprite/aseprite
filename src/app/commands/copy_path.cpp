// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/util/clipboard.h"

#include <cstdio>

namespace app {

class CopyPathCommand : public Command {
public:
  CopyPathCommand();
protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

CopyPathCommand::CopyPathCommand()
  : Command(CommandId::CopyPath(), CmdRecordableFlag)
{
}

bool CopyPathCommand::onEnabled(Context* ctx)
{
  const ContextReader reader(ctx);
  return
    reader.document() &&
    reader.document()->isAssociatedToFile();
}

void CopyPathCommand::onExecute(Context* ctx)
{
  Clipboard::instance()->setClipboardText(
    ctx->activeDocument()->filename());
}

Command* CommandFactory::createCopyPathCommand()
{
  return new CopyPathCommand;
}

} // namespace app
