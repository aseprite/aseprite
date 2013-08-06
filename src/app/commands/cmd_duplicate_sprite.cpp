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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/ui_context.h"
#include "raster/sprite.h"
#include "ui/ui.h"

#include <allegro.h>
#include <cstdio>

namespace app {

using namespace ui;

class DuplicateSpriteCommand : public Command {
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
  const ContextReader reader(context);
  const Document* document = reader.document();
  char buf[1024];

  /* load the window widget */
  base::UniquePtr<Window> window(app::load_widget<Window>("duplicate_sprite.xml", "duplicate_sprite"));

  src_name = window->findChild("src_name");
  dst_name = window->findChild("dst_name");
  flatten = window->findChild("flatten");

  src_name->setText(get_filename(document->getFilename()));

  std::sprintf(buf, "%s Copy", document->getFilename());
  dst_name->setText(buf);

  if (get_config_bool("DuplicateSprite", "Flatten", false))
    flatten->setSelected(true);

  // Open the window
  window->openWindowInForeground();

  if (window->getKiller() == window->findChild("ok")) {
    set_config_bool("DuplicateSprite", "Flatten", flatten->isSelected());

    // Make a copy of the document
    Document* docCopy;
    if (flatten->isSelected())
      docCopy = document->duplicate(DuplicateWithFlattenLayers);
    else
      docCopy = document->duplicate(DuplicateExactCopy);

    docCopy->setFilename(dst_name->getText());

    context->addDocument(docCopy);
  }
}

Command* CommandFactory::createDuplicateSpriteCommand()
{
  return new DuplicateSpriteCommand;
}

} // namespace app
