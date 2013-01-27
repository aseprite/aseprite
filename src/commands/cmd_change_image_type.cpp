/* ASEPRITE
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

#include "config.h"

#include "app.h"
#include "commands/command.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "undo_transaction.h"

#include <allegro/unicode.h>

class ChangePixelFormatCommand : public Command
{
  PixelFormat m_format;
  DitheringMethod m_dithering;
public:
  ChangePixelFormatCommand();
  Command* clone() const { return new ChangePixelFormatCommand(*this); }

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
  m_dithering = DITHERING_NONE;
}

void ChangePixelFormatCommand::onLoadParams(Params* params)
{
  std::string format = params->get("format");
  if (format == "rgb") m_format = IMAGE_RGB;
  else if (format == "grayscale") m_format = IMAGE_GRAYSCALE;
  else if (format == "indexed") m_format = IMAGE_INDEXED;

  std::string dithering = params->get("dithering");
  if (dithering == "ordered")
    m_dithering = DITHERING_ORDERED;
  else
    m_dithering = DITHERING_NONE;
}

bool ChangePixelFormatCommand::onEnabled(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document ? document->getSprite(): 0);

  if (sprite != NULL &&
      sprite->getPixelFormat() == IMAGE_INDEXED &&
      m_format == IMAGE_INDEXED &&
      m_dithering == DITHERING_ORDERED)
    return false;

  return sprite != NULL;
}

bool ChangePixelFormatCommand::onChecked(Context* context)
{
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);

  if (sprite != NULL &&
      sprite->getPixelFormat() == IMAGE_INDEXED &&
      m_format == IMAGE_INDEXED &&
      m_dithering == DITHERING_ORDERED)
    return false;

  return
    sprite != NULL &&
    sprite->getPixelFormat() == m_format;
}

void ChangePixelFormatCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  {
    UndoTransaction undoTransaction(document, "Color Mode Change");
    undoTransaction.setPixelFormat(m_format, m_dithering);
    undoTransaction.commit();
  }
  app_refresh_screen(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createChangePixelFormatCommand()
{
  return new ChangePixelFormatCommand;
}
