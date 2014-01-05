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

namespace app {

using namespace ui;

class LayerPropertiesCommand : public Command {
public:
  LayerPropertiesCommand();
  Command* clone() { return new LayerPropertiesCommand(*this); }

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

  base::UniquePtr<Window> window(new Window(Window::WithTitleBar, "Layer Properties"));
  Box* box1 = new Box(JI_VERTICAL);
  Box* box2 = new Box(JI_HORIZONTAL);
  Box* box3 = new Box(JI_HORIZONTAL + JI_HOMOGENEOUS);
  Widget* label_name = new Label("Name:");
  Entry* entry_name = new Entry(256, layer->getName().c_str());
  Button* button_ok = new Button("&OK");
  Button* button_cancel = new Button("&Cancel");

  button_ok->Click.connect(Bind<void>(&Window::closeWindow, window.get(), button_ok));
  button_cancel->Click.connect(Bind<void>(&Window::closeWindow, window.get(), button_cancel));

  jwidget_set_min_size(entry_name, 128, 0);
  entry_name->setExpansive(true);

  box2->addChild(label_name);
  box2->addChild(entry_name);
  box1->addChild(box2);
  box3->addChild(button_ok);
  box3->addChild(button_cancel);
  box1->addChild(box3);
  window->addChild(box1);

  entry_name->setFocusMagnet(true);
  button_ok->setFocusMagnet(true);

  window->openWindowInForeground();

  if (window->getKiller() == button_ok) {
    ContextWriter writer(reader);

    writer.layer()->setName(entry_name->getText());

    update_screen_for_document(writer.document());
  }
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
