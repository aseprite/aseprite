// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "doc/dithering_method.h"
#include "doc/image.h"
#include "doc/sprite.h"

namespace app {

class ChangePixelFormatCommand : public Command {
  PixelFormat m_format;
  DitheringMethod m_dithering;
public:
  ChangePixelFormatCommand();
  Command* clone() const override { return new ChangePixelFormatCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

ChangePixelFormatCommand::ChangePixelFormatCommand()
  : Command("ChangePixelFormat",
            "Change Pixel Format",
            CmdUIOnlyFlag)
{
  m_format = IMAGE_RGB;
  m_dithering = DitheringMethod::NONE;
}

void ChangePixelFormatCommand::onLoadParams(const Params& params)
{
  std::string format = params.get("format");
  if (format == "rgb") m_format = IMAGE_RGB;
  else if (format == "grayscale") m_format = IMAGE_GRAYSCALE;
  else if (format == "indexed") m_format = IMAGE_INDEXED;

  std::string dithering = params.get("dithering");
  if (dithering == "ordered")
    m_dithering = DitheringMethod::ORDERED;
  else
    m_dithering = DitheringMethod::NONE;
}

bool ChangePixelFormatCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Sprite* sprite(writer.sprite());

  if (sprite != NULL &&
      sprite->pixelFormat() == IMAGE_INDEXED &&
      m_format == IMAGE_INDEXED &&
      m_dithering == DitheringMethod::ORDERED)
    return false;

  return sprite != NULL;
}

bool ChangePixelFormatCommand::onChecked(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();

  if (sprite != NULL &&
      sprite->pixelFormat() == IMAGE_INDEXED &&
      m_format == IMAGE_INDEXED &&
      m_dithering == DitheringMethod::ORDERED)
    return false;

  return
    sprite != NULL &&
    sprite->pixelFormat() == m_format;
}

void ChangePixelFormatCommand::onExecute(Context* context)
{
  {
    ContextWriter writer(context);
    Transaction transaction(writer.context(), "Color Mode Change");
    Document* document(writer.document());
    Sprite* sprite(writer.sprite());

    document->getApi(transaction).setPixelFormat(sprite, m_format, m_dithering);
    transaction.commit();
  }

  if (context->isUIAvailable())
    app_refresh_screen();
}

Command* CommandFactory::createChangePixelFormatCommand()
{
  return new ChangePixelFormatCommand;
}

} // namespace app
