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

#include "base/bind.h"
#include "gui/gui.h"

#include "app.h"
#include "commands/command.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "document_wrappers.h"

using namespace ui;

//////////////////////////////////////////////////////////////////////
// layer_properties

class LayerPropertiesCommand : public Command
{
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
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document->getSprite());
  const Layer* layer = sprite->getCurrentLayer();

  UniquePtr<Frame> window(new Frame(false, "Layer Properties"));
  Box* box1 = new Box(JI_VERTICAL);
  Box* box2 = new Box(JI_HORIZONTAL);
  Box* box3 = new Box(JI_HORIZONTAL + JI_HOMOGENEOUS);
  Widget* label_name = new Label("Name:");
  Entry* entry_name = new Entry(256, layer->getName().c_str());
  Button* button_ok = new Button("&OK");
  Button* button_cancel = new Button("&Cancel");

  button_ok->Click.connect(Bind<void>(&Frame::closeWindow, window.get(), button_ok));
  button_cancel->Click.connect(Bind<void>(&Frame::closeWindow, window.get(), button_cancel));

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

  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    DocumentWriter documentWriter(document);

    const_cast<Layer*>(layer)->setName(entry_name->getText());

    update_screen_for_document(document);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}
