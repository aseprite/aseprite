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
#include "base/mem_utils.h"
#include "commands/command.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo/undo_history.h"
#include "undoers/set_cel_opacity.h"

#include <allegro/unicode.h>

class CelPropertiesCommand : public Command
{
public:
  CelPropertiesCommand();
  Command* clone() const { return new CelPropertiesCommand(*this); }

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
  const ActiveDocumentReader document(context);
  const Sprite* sprite = document->getSprite();
  const Layer* layer = sprite->getCurrentLayer();

  // Get current cel (can be NULL)
  const Cel* cel = static_cast<const LayerImage*>(layer)->getCel(sprite->getCurrentFrame());

  UniquePtr<Frame> window(app::load_widget<Frame>("cel_properties.xml", "cel_properties"));
  Widget* label_frame = app::find_widget<Widget>(window, "frame");
  Widget* label_pos = app::find_widget<Widget>(window, "pos");
  Widget* label_size = app::find_widget<Widget>(window, "size");
  Slider* slider_opacity = app::find_widget<Slider>(window, "opacity");
  Widget* button_ok = app::find_widget<Widget>(window, "ok");
  gui::TooltipManager* tooltipManager = window->findFirstChildByType<gui::TooltipManager>();

  // Mini look for the opacity slider
  setup_mini_look(slider_opacity);

  /* if the layer isn't writable */
  if (!layer->is_writable()) {
    button_ok->setText("Locked");
    button_ok->setEnabled(false);
  }

  label_frame->setTextf("%d/%d", sprite->getCurrentFrame()+1, sprite->getTotalFrames());

  if (cel != NULL) {
    // Position
    label_pos->setTextf("%d, %d", cel->getX(), cel->getY());

    // Dimension (and memory size)
    int memsize =
      image_line_size(sprite->getStock()->getImage(cel->getImage()),
                      sprite->getStock()->getImage(cel->getImage())->w)*
      sprite->getStock()->getImage(cel->getImage())->h;

    label_size->setTextf("%dx%d (%s)",
                         sprite->getStock()->getImage(cel->getImage())->w,
                         sprite->getStock()->getImage(cel->getImage())->h,
                         get_pretty_memory_size(memsize).c_str());

    // Opacity
    slider_opacity->setValue(cel->getOpacity());
    if (layer->is_background()) {
      slider_opacity->setEnabled(false);
      tooltipManager->addTooltipFor(slider_opacity,
                                    "The `Background' layer is opaque,\n"
                                    "you can't change its opacity.",
                                    JI_LEFT);
    }
  }
  else {
    label_pos->setText("None");
    label_size->setText("Empty (0 bytes)");
    slider_opacity->setValue(0);
    slider_opacity->setEnabled(false);
  }

  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    DocumentWriter document_writer(document);
    Sprite* sprite_writer = document_writer->getSprite();
    Layer* layer_writer = sprite_writer->getCurrentLayer();
    Cel* cel_writer = static_cast<LayerImage*>(layer_writer)->getCel(sprite->getCurrentFrame());
    undo::UndoHistory* undo = document_writer->getUndoHistory();

    int new_opacity = slider_opacity->getValue();

    // The opacity was changed?
    if (cel_writer != NULL &&
        cel_writer->getOpacity() != new_opacity) {
      if (undo->isEnabled()) {
        undo->setLabel("Cel Opacity Change");
        undo->setModification(undo::ModifyDocument);

        undo->pushUndoer(new undoers::SetCelOpacity(undo->getObjects(), cel_writer));
      }

      // Change cel opacity.
      cel_writer->setOpacity(new_opacity);

      update_screen_for_document(document);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createCelPropertiesCommand()
{
  return new CelPropertiesCommand;
}
