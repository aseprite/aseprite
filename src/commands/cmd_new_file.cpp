/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "app/color.h"
#include "app/color_utils.h"
#include "base/unique_ptr.h"
#include "commands/command.h"
#include "console.h"
#include "document.h"
#include "gui/gui.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "ui_context.h"
#include "widgets/color_bar.h"

#include <allegro/config.h>
#include <allegro/unicode.h>

//////////////////////////////////////////////////////////////////////
// new_file

class NewFileCommand : public Command
{
public:
  NewFileCommand();
  Command* clone() { return new NewFileCommand(*this); }

protected:
  void onExecute(Context* context);
};

static int _sprite_counter = 0;

NewFileCommand::NewFileCommand()
  : Command("NewFile",
            "New File",
            CmdRecordableFlag)
{
}

/**
 * Shows the "New Sprite" dialog.
 */
void NewFileCommand::onExecute(Context* context)
{
  JWidget width, height, radio1, radio2, radio3, colors, ok, bg_box;
  int imgtype, w, h, bg, ncolors;
  char buf[1024];
  Color bg_table[] = {
    Color::fromMask(),
    Color::fromRgb(0, 0, 0),
    Color::fromRgb(255, 255, 255),
    Color::fromRgb(255, 0, 255),
    app_get_colorbar()->getBgColor()
  };

  // Load the window widget
  FramePtr window(load_widget("new_sprite.xml", "new_sprite"));
  get_widgets(window,
              "width", &width,
              "height", &height,
              "radio1", &radio1,
              "radio2", &radio2,
              "radio3", &radio3,
              "colors", &colors,
              "ok_button", &ok,
              "bg_box", &bg_box, NULL);

  // Default values: Indexed, 320x240, Background color
  imgtype = get_config_int("NewSprite", "Type", IMAGE_INDEXED);
  imgtype = MID(IMAGE_RGB, imgtype, IMAGE_INDEXED);
  w = get_config_int("NewSprite", "Width", 320);
  h = get_config_int("NewSprite", "Height", 240);
  bg = get_config_int("NewSprite", "Background", 4); // Default = Background color
  ncolors = get_config_int("NewSprite", "Colors", 256);

  width->setTextf("%d", MAX(1, w));
  height->setTextf("%d", MAX(1, h));
  colors->setTextf("%d", MID(2, ncolors, 256));

  // Select image-type
  switch (imgtype) {
    case IMAGE_RGB:       radio1->setSelected(true); break;
    case IMAGE_GRAYSCALE: radio2->setSelected(true); break;
    case IMAGE_INDEXED:   radio3->setSelected(true); break;
  }

  // Select background color
  jlistbox_select_index(bg_box, bg);

  // Open the window
  window->open_window_fg();

  if (window->get_killer() == ok) {
    bool ok = false;

    // Get the options
    if (radio1->isSelected())      imgtype = IMAGE_RGB;
    else if (radio2->isSelected()) imgtype = IMAGE_GRAYSCALE;
    else if (radio3->isSelected()) imgtype = IMAGE_INDEXED;

    w = width->getTextInt();
    h = height->getTextInt();
    ncolors = colors->getTextInt();
    bg = jlistbox_get_selected_index(bg_box);

    w = MID(1, w, 9999);
    h = MID(1, h, 9999);
    ncolors = MID(2, ncolors, 256);

    // Select the color
    Color color = Color::fromMask();

    if (bg >= 0 && bg <= 4) {
      color = bg_table[bg];
      ok = true;
    }

    if (ok) {
      // Save the configuration
      set_config_int("NewSprite", "Type", imgtype);
      set_config_int("NewSprite", "Width", w);
      set_config_int("NewSprite", "Height", h);
      set_config_int("NewSprite", "Background", bg);

      // Create the new sprite
      ASSERT(imgtype == IMAGE_RGB || imgtype == IMAGE_GRAYSCALE || imgtype == IMAGE_INDEXED);
      ASSERT(w >= 1 && w <= 9999);
      ASSERT(h >= 1 && h <= 9999);

      UniquePtr<Document> document(
        Document::createBasicDocument(imgtype, w, h,
                                      (imgtype == IMAGE_INDEXED ? ncolors: 256)));
      Sprite* sprite(document->getSprite());

      get_default_palette()->copyColorsTo(sprite->getCurrentPalette());

      usprintf(buf, "Sprite-%04d", ++_sprite_counter);
      document->setFilename(buf);

      // If the background color isn't transparent, we have to
      // convert the `Layer 1' in a `Background'
      if (color.getType() != Color::MaskType) {
        Sprite* sprite = document->getSprite();

        ASSERT(sprite->getCurrentLayer() && sprite->getCurrentLayer()->is_image());

        static_cast<LayerImage*>(sprite->getCurrentLayer())->configureAsBackground();
        image_clear(sprite->getCurrentImage(), color_utils::color_for_image(color, imgtype));
      }

      // Show the sprite to the user
      context->addDocument(document);

      // Release the document as it is already owned by the context.
      // And put the document in a reliable editor.
      set_document_in_more_reliable_editor(document.release());
    }
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createNewFileCommand()
{
  return new NewFileCommand;
}
