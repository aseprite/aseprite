// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  Command* clone() const override { return new CopyMergedCommand(*this); }

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

CopyMergedCommand::CopyMergedCommand()
  : Command("CopyMerged",
            "Copy Merged",
            CmdUIOnlyFlag)
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
