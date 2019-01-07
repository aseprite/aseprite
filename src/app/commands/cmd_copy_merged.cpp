// Aseprite
// Copyright (C) 2016-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/util/clipboard.h"

namespace app {

class CopyMergedCommand : public Command {
public:
  CopyMergedCommand();

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

CopyMergedCommand::CopyMergedCommand()
  : Command(CommandId::CopyMerged(), CmdUIOnlyFlag)
{
}

bool CopyMergedCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                         ContextFlags::HasVisibleMask);
}

void CopyMergedCommand::onExecute(Context* ctx)
{
  ContextReader reader(ctx);
  clipboard::copy_merged(reader);
}

Command* CommandFactory::createCopyMergedCommand()
{
  return new CopyMergedCommand;
}

} // namespace app
