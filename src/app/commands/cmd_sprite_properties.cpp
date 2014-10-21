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

#include "app/color.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/ui/color_button.h"
#include "app/undo_transaction.h"
#include "base/bind.h"
#include "base/mem_utils.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <cstdio>

namespace app {

using namespace ui;

class SpritePropertiesCommand : public Command {
public:
  SpritePropertiesCommand();
  Command* clone() const override { return new SpritePropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

SpritePropertiesCommand::SpritePropertiesCommand()
  : Command("SpriteProperties",
            "Sprite Properties",
            CmdUIOnlyFlag)
{
}

bool SpritePropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void SpritePropertiesCommand::onExecute(Context* context)
{
  Widget* name, *type, *size, *frames, *ok, *box_transparent;
  std::string imgtype_text;
  char buf[256];
  ColorButton* color_button = NULL;

  // Load the window widget
  base::UniquePtr<Window> window(app::load_widget<Window>("sprite_properties.xml", "sprite_properties"));
  name = app::find_widget<Widget>(window, "name");
  type = app::find_widget<Widget>(window, "type");
  size = app::find_widget<Widget>(window, "size");
  frames = app::find_widget<Widget>(window, "frames");
  ok = app::find_widget<Widget>(window, "ok");
  box_transparent = app::find_widget<Widget>(window, "box_transparent");

  // Get sprite properties and fill frame fields
  {
    const ContextReader reader(context);
    const Document* document(reader.document());
    const Sprite* sprite(reader.sprite());

    // Update widgets values
    switch (sprite->pixelFormat()) {
      case IMAGE_RGB:
        imgtype_text = "RGB";
        break;
      case IMAGE_GRAYSCALE:
        imgtype_text = "Grayscale";
        break;
      case IMAGE_INDEXED:
        std::sprintf(buf, "Indexed (%d colors)", sprite->getPalette(FrameNumber(0))->size());
        imgtype_text = buf;
        break;
      default:
        ASSERT(false);
        imgtype_text = "Unknown";
        break;
    }

    // Filename
    name->setText(document->filename());

    // Color mode
    type->setText(imgtype_text.c_str());

    // Sprite size (width and height)
    sprintf(buf, "%dx%d (%s)",
      sprite->width(),
      sprite->height(),
      base::get_pretty_memory_size(sprite->getMemSize()).c_str());

    size->setText(buf);

    // How many frames
    frames->setTextf("%d", (int)sprite->totalFrames());

    if (sprite->pixelFormat() == IMAGE_INDEXED) {
      color_button = new ColorButton(app::Color::fromIndex(sprite->transparentColor()),
                                     IMAGE_INDEXED);

      box_transparent->addChild(color_button);
    }
    else {
      box_transparent->addChild(new Label("(only for indexed images)"));
    }
  }

  window->remapWindow();
  window->centerWindow();

  load_window_pos(window, "SpriteProperties");
  window->setVisible(true);
  window->openWindowInForeground();

  if (window->getKiller() == ok) {
    if (color_button) {
      ContextWriter writer(context);
      Sprite* sprite(writer.sprite());

      // If the transparent color index has changed, we update the
      // property in the sprite.
      int index = color_button->getColor().getIndex();
      if (index != sprite->transparentColor()) {
        UndoTransaction undoTransaction(writer.context(), "Set Transparent Color");
        DocumentApi api = writer.document()->getApi();
        api.setSpriteTransparentColor(sprite, index);
        undoTransaction.commit();

        update_screen_for_document(writer.document());
      }
    }
  }

  save_window_pos(window, "SpriteProperties");
}

Command* CommandFactory::createSpritePropertiesCommand()
{
  return new SpritePropertiesCommand;
}

} // namespace app
