/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/undo_transaction.h"
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
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  bool onChecked(Context* context);
  void onExecute(Context* context);
};

ChangePixelFormatCommand::ChangePixelFormatCommand()
  : Command("ChangePixelFormat",
            "Change Pixel Format",
            CmdUIOnlyFlag)
{
  m_format = IMAGE_RGB;
  m_dithering = DitheringMethod::NONE;
}

void ChangePixelFormatCommand::onLoadParams(Params* params)
{
  std::string format = params->get("format");
  if (format == "rgb") m_format = IMAGE_RGB;
  else if (format == "grayscale") m_format = IMAGE_GRAYSCALE;
  else if (format == "indexed") m_format = IMAGE_INDEXED;

  std::string dithering = params->get("dithering");
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
    UndoTransaction undoTransaction(writer.context(), "Color Mode Change");
    Document* document(writer.document());
    Sprite* sprite(writer.sprite());

    document->getApi().setPixelFormat(sprite, m_format, m_dithering);
    undoTransaction.commit();
  }
  app_refresh_screen();
}

Command* CommandFactory::createChangePixelFormatCommand()
{
  return new ChangePixelFormatCommand;
}

} // namespace app
