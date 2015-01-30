/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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
#include "doc/image.h"
#include "doc/layer.h"

#include "generated_layer_properties.h"

namespace app {

using namespace ui;

class LayerVisibilityCommand : public Command {
public:
  LayerVisibilityCommand();
  Command* clone() const override { return new LayerVisibilityCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

LayerVisibilityCommand::LayerVisibilityCommand()
  : Command("LayerVisibility",
            "Layer Visibility",
            CmdRecordableFlag)
{
}

bool LayerVisibilityCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

bool LayerVisibilityCommand::onChecked(Context* context)
{
  const ContextReader reader(context);
  const Layer* layer = reader.layer();
  return (layer && layer->isVisible());
}

void LayerVisibilityCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Layer* layer = writer.layer();

  layer->setVisible(!layer->isVisible());

  update_screen_for_document(writer.document());
}

Command* CommandFactory::createLayerVisibilityCommand()
{
  return new LayerVisibilityCommand;
}

} // namespace app
