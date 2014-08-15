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

#include "base/bind.h"
#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "raster/image.h"
#include "raster/layer.h"

#include "generated_layer_properties.h"

namespace app {

using namespace ui;

class LayerPropertiesCommand : public Command {
public:
  LayerPropertiesCommand();
  Command* clone() const override { return new LayerPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LayerPropertiesCommand::LayerPropertiesCommand()
  : Command("LayerProperties",
            "Layer Properties",
            CmdRecordableFlag)
{
}

bool LayerPropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

void LayerPropertiesCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Layer* layer = reader.layer();

  app::gen::LayerProperties window;
  window.name()->setText(layer->name().c_str());
  window.name()->setMinSize(gfx::Size(128, 0));
  window.name()->setExpansive(true);
  window.openWindowInForeground();

  if (window.getKiller() == window.ok()) {
    ContextWriter writer(reader);
    writer.layer()->setName(window.name()->getText());
    update_screen_for_document(writer.document());
  }
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
