// Aseprite
// Copyright (C) 2001-2015  David Capello
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
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "base/mem_utils.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "generated_cel_properties.h"

namespace app {

using namespace ui;

class CelPropertiesWindow : public app::gen::CelProperties {
public:
  CelPropertiesWindow() {
  }
};

class CelPropertiesCommand : public Command {
public:
  CelPropertiesCommand();
  Command* clone() const override { return new CelPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CelPropertiesCommand::CelPropertiesCommand()
  : Command("CelProperties",
            "Cel Properties",
            CmdUIOnlyFlag)
{
}

bool CelPropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsImage);
}

void CelPropertiesCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();
  const Layer* layer = reader.layer();
  const Cel* cel = reader.cel(); // Get current cel (can be NULL)

  CelPropertiesWindow window;
  ui::TooltipManager* tooltipManager = window.findFirstChildByType<ui::TooltipManager>();

  // If the layer isn't writable
  if (!layer->isEditable()) {
    window.ok()->setText("Locked");
    window.ok()->setEnabled(false);
  }

  window.frame()->setTextf("%d/%d",
                           (int)reader.frame()+1,
                           (int)sprite->totalFrames());

  if (cel != NULL) {
    // Position
    window.pos()->setTextf("%d, %d", cel->x(), cel->y());

    // Dimension (and memory size)
    Image* image = cel->image();
    int memsize = image->getRowStrideSize() * image->height();

    window.size()->setTextf("%dx%d (%s)",
      image->width(),
      image->height(),
      base::get_pretty_memory_size(memsize).c_str());

    // Opacity
    window.opacity()->setValue(cel->opacity());
    if (layer->isBackground()) {
      window.opacity()->setEnabled(false);
      tooltipManager->addTooltipFor(window.opacity(),
        "The `Background' layer is opaque,\n"
        "its opacity can't be changed.",
        LEFT);
    }
    else if (sprite->pixelFormat() == IMAGE_INDEXED) {
      window.opacity()->setEnabled(false);
      tooltipManager->addTooltipFor(window.opacity(),
        "Cel opacity of Indexed images\n"
        "cannot be changed.",
        LEFT);
    }
  }
  else {
    window.pos()->setText("None");
    window.size()->setText("Empty (0 bytes)");
    window.opacity()->setValue(0);
    window.opacity()->setEnabled(false);
  }

  window.openWindowInForeground();

  if (window.getKiller() == window.ok()) {
    ContextWriter writer(reader);
    Document* document_writer = writer.document();
    Sprite* sprite_writer = writer.sprite();
    Cel* cel_writer = writer.cel();

    int newOpacity = window.opacity()->getValue();

    // The opacity was changed?
    if (cel_writer != NULL &&
        cel_writer->opacity() != newOpacity) {
      {
        Transaction transaction(writer.context(), "Cel Opacity Change", ModifyDocument);
        document_writer->getApi(transaction)
          .setCelOpacity(sprite_writer, cel_writer, newOpacity);
        transaction.commit();
      }

      update_screen_for_document(document_writer);
    }
  }
}

Command* CommandFactory::createCelPropertiesCommand()
{
  return new CelPropertiesCommand;
}

} // namespace app
