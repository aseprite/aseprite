// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/transaction.h"
#include "app/util/autocrop.h"
#include "app/util/misc.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {

class CropSpriteCommand : public Command {
public:
  CropSpriteCommand();
  Command* clone() const override { return new CropSpriteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CropSpriteCommand::CropSpriteCommand()
  : Command("CropSprite",
            "Crop Sprite",
            CmdRecordableFlag)
{
}

bool CropSpriteCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasVisibleMask);
}

void CropSpriteCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  Mask* mask(document->mask());
  {
    Transaction transaction(writer.context(), "Sprite Crop");
    document->getApi(transaction).cropSprite(sprite, mask->bounds());
    transaction.commit();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

class AutocropSpriteCommand : public Command {
public:
  AutocropSpriteCommand();
  Command* clone() const override { return new AutocropSpriteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

AutocropSpriteCommand::AutocropSpriteCommand()
  : Command("AutocropSprite",
            "Trim Sprite",
            CmdRecordableFlag)
{
}

bool AutocropSpriteCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void AutocropSpriteCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Transaction transaction(writer.context(), "Trim Sprite");
    document->getApi(transaction).trimSprite(sprite);
    transaction.commit();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

Command* CommandFactory::createCropSpriteCommand()
{
  return new CropSpriteCommand;
}

Command* CommandFactory::createAutocropSpriteCommand()
{
  return new AutocropSpriteCommand;
}

} // namespace app
