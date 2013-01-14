/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "commands/command.h"
#include "document_wrappers.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "ui/gui.h"
#include "ui_context.h"

#include <allegro.h>
#include <cstdio>

using namespace ui;

//////////////////////////////////////////////////////////////////////
// duplicate_sprite

class DuplicateSpriteCommand : public Command
{
public:
  DuplicateSpriteCommand();
  Command* clone() { return new DuplicateSpriteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

DuplicateSpriteCommand::DuplicateSpriteCommand()
  : Command("DuplicateSprite",
            "Duplicate Sprite",
            CmdUIOnlyFlag)
{
}

bool DuplicateSpriteCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void DuplicateSpriteCommand::onExecute(Context* context)
{
  Widget* src_name, *dst_name, *flatten;
  const ActiveDocumentReader document(context);
  char buf[1024];

  /* load the window widget */
  UniquePtr<Window> window(app::load_widget<Window>("duplicate_sprite.xml", "duplicate_sprite"));

  src_name = window->findChild("src_name");
  dst_name = window->findChild("dst_name");
  flatten = window->findChild("flatten");

  src_name->setText(get_filename(document->getFilename()));

  std::sprintf(buf, "%s Copy", document->getFilename());
  dst_name->setText(buf);

  if (get_config_bool("DuplicateSprite", "Flatten", false))
    flatten->setSelected(true);

  /* open the window */
  window->openWindowInForeground();

  if (window->getKiller() == window->findChild("ok")) {
    set_config_bool("DuplicateSprite", "Flatten", flatten->isSelected());

    // make a copy of the document
    Document* docCopy;
    if (flatten->isSelected())
      docCopy = document->duplicate(DuplicateWithFlattenLayers);
    else
      docCopy = document->duplicate(DuplicateExactCopy);

    docCopy->setFilename(dst_name->getText());

    context->addDocument(docCopy);
    set_document_in_more_reliable_editor(docCopy);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createDuplicateSpriteCommand()
{
  return new DuplicateSpriteCommand;
}
