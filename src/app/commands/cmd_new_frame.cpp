// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/transaction.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <stdexcept>

namespace app {

class NewFrameCommand : public Command {
public:
  enum class Content { CurrentFrame, EmptyFrame };

  NewFrameCommand();
  Command* clone() const override { return new NewFrameCommand(*this); }

protected:
  void onLoadParams(Params* params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Content m_content;
};

NewFrameCommand::NewFrameCommand()
  : Command("NewFrame",
            "New Frame",
            CmdRecordableFlag)
{
}

void NewFrameCommand::onLoadParams(Params* params)
{
  m_content = Content::CurrentFrame;

  std::string content = params->get("content");
  if (content == "current") m_content = Content::CurrentFrame;
  else if (content == "empty") m_content = Content::EmptyFrame;
}

bool NewFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewFrameCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Transaction transaction(writer.context(), "New Frame");
    DocumentApi api = document->getApi(transaction);

    switch (m_content) {
      case Content::CurrentFrame:
        api.addFrame(sprite, writer.frame()+1);
        break;
      case Content::EmptyFrame:
        api.addEmptyFrame(sprite, writer.frame()+1);
        break;
    }

    transaction.commit();
  }
  update_screen_for_document(document);

  StatusBar::instance()
    ->showTip(1000, "New frame %d/%d",
              (int)context->activeLocation().frame()+1,
              (int)sprite->totalFrames());

  App::instance()->getMainWindow()->popTimeline();
}

std::string NewFrameCommand::onGetFriendlyName() const
{
  std::string text = "New Frame";

  switch (m_content) {
    case Content::CurrentFrame:
      text = "New Frame (duplicated)";
      break;
    case Content::EmptyFrame:
      text = "New Frame (empty)";
      break;
  }

  return text;
}

Command* CommandFactory::createNewFrameCommand()
{
  return new NewFrameCommand;
}

} // namespace app
