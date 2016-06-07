// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

class NewLayerGroupCommand : public Command {
public:
  NewLayerGroupCommand();
  Command* clone() const override { return new NewLayerGroupCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

NewLayerGroupCommand::NewLayerGroupCommand()
  : Command("NewLayerGroup",
            "New Layer Group",
            CmdRecordableFlag)
{
}

bool NewLayerGroupCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewLayerGroupCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());

  // load the window widget
  base::UniquePtr<Window> window(app::load_widget<Window>("new_layer.xml", "new_layer_set"));

  window->openWindowInForeground();

  if (window->closer() != window->findChild("ok"))
    return;

  std::string name = window->findChild("name")->text();
  Layer* layer;
  {
    Transaction transaction(writer.context(), "New Group");
    layer = document->getApi(transaction).newLayerGroup(sprite);
    transaction.commit();
  }
  layer->setName(name);

  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  StatusBar::instance()->showTip(1000, "Layer `%s' created", name.c_str());
}

Command* CommandFactory::createNewLayerGroupCommand()
{
  return new NewLayerGroupCommand;
}

} // namespace app
