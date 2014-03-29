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
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_api.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"
#include "app/undo_transaction.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

class NewLayerSetCommand : public Command {
public:
  NewLayerSetCommand();
  Command* clone() const OVERRIDE { return new NewLayerSetCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

NewLayerSetCommand::NewLayerSetCommand()
  : Command("NewLayerSet",
            "New Layer Set",
            CmdRecordableFlag)
{
}

bool NewLayerSetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewLayerSetCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());

  // load the window widget
  base::UniquePtr<Window> window(app::load_widget<Window>("new_layer.xml", "new_layer_set"));

  window->openWindowInForeground();

  if (window->getKiller() != window->findChild("ok"))
    return;

  std::string name = window->findChild("name")->getText();
  Layer* layer;
  {
    UndoTransaction undoTransaction(writer.context(), "New Layer");
    layer = document->getApi().newLayerFolder(sprite);
    undoTransaction.commit();
  }
  layer->setName(name);

  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  StatusBar::instance()->showTip(1000, "Layer `%s' created", name.c_str());
}

Command* CommandFactory::createNewLayerSetCommand()
{
  return new NewLayerSetCommand;
}

} // namespace app
