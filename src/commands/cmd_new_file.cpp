/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <assert.h>
#include <allegro/config.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "ui_context.h"
#include "commands/command.h"
#include "console.h"
#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "widgets/colbar.h"

//////////////////////////////////////////////////////////////////////
// new_file

class NewFileCommand : public Command
{
public:
  NewFileCommand();
  Command* clone() { return new NewFileCommand(*this); }

protected:
  void execute(Context* context);
};

static int _sprite_counter = 0;

static Sprite* new_sprite(Context* context, int imgtype, int w, int h, int ncolors);

NewFileCommand::NewFileCommand()
  : Command("new_file",
	    "New File",
	    CmdRecordableFlag)
{
}

/**
 * Shows the "New Sprite" dialog.
 */
void NewFileCommand::execute(Context* context)
{
  JWidget width, height, radio1, radio2, radio3, colors, ok, bg_box;
  int imgtype, w, h, bg, ncolors;
  char buf[1024];
  Sprite *sprite;
  color_t color;
  color_t bg_table[] = {
    color_mask(),
    color_rgb(0, 0, 0),
    color_rgb(255, 255, 255),
    color_rgb(255, 0, 255),
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
    case IMAGE_RGB:       jwidget_select(radio1); break;
    case IMAGE_GRAYSCALE: jwidget_select(radio2); break;
    case IMAGE_INDEXED:   jwidget_select(radio3); break;
  }

  // Select background color
  jlistbox_select_index(bg_box, bg);

  // Open the window
  window->open_window_fg();

  if (window->get_killer() == ok) {
    bool ok = false;

    // Get the options
    if (jwidget_is_selected(radio1))      imgtype = IMAGE_RGB;
    else if (jwidget_is_selected(radio2)) imgtype = IMAGE_GRAYSCALE;
    else if (jwidget_is_selected(radio3)) imgtype = IMAGE_INDEXED;

    w = width->getTextInt();
    h = height->getTextInt();
    ncolors = colors->getTextInt();
    bg = jlistbox_get_selected_index(bg_box);

    w = MID(1, w, 9999);
    h = MID(1, h, 9999);
    ncolors = MID(2, ncolors, 256);

    // Select the color
    color = color_mask();

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
      sprite = new_sprite(UIContext::instance(), imgtype, w, h,
			  imgtype == IMAGE_INDEXED ? ncolors: 256);
      if (!sprite) {
	Console console;
	console.printf("Not enough memory to allocate the sprite\n");
      }
      else {
	usprintf(buf, "Sprite-%04d", ++_sprite_counter);
	sprite->setFilename(buf);

	// If the background color isn't transparent, we have to
	// convert the `Layer 1' in a `Background'
	if (color_type(color) != COLOR_TYPE_MASK) {
	  assert(sprite->getCurrentLayer() && sprite->getCurrentLayer()->is_image());

	  static_cast<LayerImage*>(sprite->getCurrentLayer())->configure_as_background();
	  image_clear(sprite->getCurrentImage(), get_color_for_image(imgtype, color));
	}

	// Show the sprite to the user
	context->add_sprite(sprite);
	set_sprite_in_more_reliable_editor(sprite);
      }
    }
  }
}

/**
 * Creates a new sprite with the given dimension with one transparent
 * layer called "Layer 1".
 *
 * @param imgtype Color mode, one of the following values: IMAGE_RGB, IMAGE_GRAYSCALE, IMAGE_INDEXED
 * @param w Width of the sprite
 * @param h Height of the sprite
 */
static Sprite* new_sprite(Context* context, int imgtype, int w, int h, int ncolors)
{
  assert(imgtype == IMAGE_RGB || imgtype == IMAGE_GRAYSCALE || imgtype == IMAGE_INDEXED);
  assert(w >= 1 && w <= 9999);
  assert(h >= 1 && h <= 9999);

  return Sprite::createWithLayer(imgtype, w, h, ncolors);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_new_file_command()
{
  return new NewFileCommand;
}
