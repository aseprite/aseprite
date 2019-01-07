// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/util/autocrop.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {

class CropSpriteCommand : public Command {
public:
  CropSpriteCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  gfx::Rect m_bounds;
};

CropSpriteCommand::CropSpriteCommand()
  : Command(CommandId::CropSprite(), CmdRecordableFlag)
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
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());

  gfx::Rect bounds;
  if (m_bounds.isEmpty())
    bounds = document->mask()->bounds();
  else
    bounds = m_bounds;

  {
    Tx tx(writer.context(), "Sprite Crop");
    document->getApi(tx).cropSprite(sprite, bounds);
    tx.commit();
  }
  document->generateMaskBoundaries();

#ifdef ENABLE_UI
  if (context->isUIAvailable())
    update_screen_for_document(document);
#endif
}

class AutocropSpriteCommand : public Command {
public:
  AutocropSpriteCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

AutocropSpriteCommand::AutocropSpriteCommand()
  : Command(CommandId::AutocropSprite(), CmdRecordableFlag)
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
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Tx tx(writer.context(), "Trim Sprite");
    document->getApi(tx).trimSprite(sprite);
    tx.commit();
  }
  document->generateMaskBoundaries();

#ifdef ENABLE_UI
  if (context->isUIAvailable())
    update_screen_for_document(document);
#endif
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
