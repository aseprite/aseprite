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
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  gfx::Rect m_bounds;
};

CropSpriteCommand::CropSpriteCommand()
  : Command("CropSprite",
            "Crop Sprite",
            CmdRecordableFlag)
{
}

void CropSpriteCommand::onLoadParams(const Params& params)
{
  m_bounds = gfx::Rect(0, 0, 0, 0);
  if (params.has_param("x")) m_bounds.x = params.get_as<int>("x");
  if (params.has_param("y")) m_bounds.y = params.get_as<int>("y");
  if (params.has_param("width")) m_bounds.w = params.get_as<int>("width");
  if (params.has_param("height")) m_bounds.h = params.get_as<int>("height");
}

bool CropSpriteCommand::onEnabled(Context* context)
{
  return
    context->checkFlags(
      ContextFlags::ActiveDocumentIsWritable |
      (m_bounds.isEmpty() ? ContextFlags::HasVisibleMask: 0));
}

void CropSpriteCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());

  gfx::Rect bounds;
  if (m_bounds.isEmpty())
    bounds = document->mask()->bounds();
  else
    bounds = m_bounds;

  {
    Transaction transaction(writer.context(), "Sprite Crop");
    document->getApi(transaction).cropSprite(sprite, bounds);
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
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
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
