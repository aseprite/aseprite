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

#include <allegro/unicode.h>

#include "base/bind.h"
#include "gui/jinete.h"

#include "app/color.h"
#include "base/mem_utils.h"
#include "commands/command.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "widgets/color_button.h"

/* TODO remove this */
void dialogs_frame_length(const SpriteReader& sprite, int sprite_frpos);

//////////////////////////////////////////////////////////////////////
// sprite_properties

class SpritePropertiesCommand : public Command
{
public:
  SpritePropertiesCommand();
  Command* clone() { return new SpritePropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

SpritePropertiesCommand::SpritePropertiesCommand()
  : Command("sprite_properties",
	    "Sprite Properties",
	    CmdUIOnlyFlag)
{
}

bool SpritePropertiesCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void SpritePropertiesCommand::onExecute(Context* context)
{
  JWidget name, type, size, frames, ok;
  Button* speed;
  base::string imgtype_text;
  char buf[256];

  // Load the window widget
  FramePtr window(load_widget("sprite_properties.xml", "sprite_properties"));
  get_widgets(window,
	      "name", &name,
	      "type", &type,
	      "size", &size,
	      "frames", &frames,
	      "speed", &speed,
	      "ok", &ok, NULL);

  // Get sprite properties and fill frame fields
  {
    const CurrentSpriteReader sprite(context);

    // Update widgets values
    switch (sprite->getImgType()) {
      case IMAGE_RGB:
	imgtype_text = "RGB";
	break;
      case IMAGE_GRAYSCALE:
	imgtype_text = "Grayscale";
	break;
      case IMAGE_INDEXED:
	sprintf(buf, "Indexed (%d colors)", sprite->getPalette(0)->size());
	imgtype_text = buf;
	break;
      default:
	ASSERT(false);
	imgtype_text = "Unknown";
	break;
    }

    // Filename
    name->setText(sprite->getFilename());

    // Color mode
    type->setText(imgtype_text.c_str());

    // Sprite size (width and height)
    usprintf(buf, "%dx%d (%s)",
	     sprite->getWidth(),
	     sprite->getHeight(),
	     get_pretty_memory_size(sprite->getMemSize()));

    size->setText(buf);

    // How many frames
    frames->setTextf("%d", sprite->getTotalFrames());

    // Speed button
    speed->Click.connect(Bind<void>(&dialogs_frame_length, sprite, -1));
  }

  window->remap_window();
  window->center_window();

  load_window_pos(window, "SpriteProperties");
  window->setVisible(true);
  window->open_window_fg();
  save_window_pos(window, "SpriteProperties");
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_sprite_properties_command()
{
  return new SpritePropertiesCommand;
}
