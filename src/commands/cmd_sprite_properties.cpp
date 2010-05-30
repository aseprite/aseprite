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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "core/core.h"
#include "core/color.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "raster/palette.h"
#include "widgets/colbut.h"
#include "sprite_wrappers.h"

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
  bool enabled(Context* context);
  void execute(Context* context);
};

SpritePropertiesCommand::SpritePropertiesCommand()
  : Command("sprite_properties",
	    "Sprite Properties",
	    CmdUIOnlyFlag)
{
}

bool SpritePropertiesCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void SpritePropertiesCommand::execute(Context* context)
{
  JWidget killer, name, type, size, frames, speed, ok;
  const CurrentSpriteReader sprite(context);
  jstring imgtype_text;
  char buf[256];

  /* load the window widget */
  FramePtr window(load_widget("sprite_properties.xml", "sprite_properties"));
  get_widgets(window,
	      "name", &name,
	      "type", &type,
	      "size", &size,
	      "frames", &frames,
	      "speed", &speed,
	      "ok", &ok, NULL);

  /* update widgets values */
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
      assert(false);
      imgtype_text = "Unknown";
      break;
  }

  /* filename */
  name->setText(sprite->getFilename());

  /* color mode */
  type->setText(imgtype_text.c_str());

  /* sprite size (width and height) */
  usprintf(buf, "%dx%d (", sprite->getWidth(), sprite->getHeight());
  get_pretty_memsize(sprite->getMemSize(),
 		     buf+ustrsize(buf),
		     sizeof(buf)-ustrsize(buf));
  ustrcat(buf, ")");
  size->setText(buf);

  /* how many frames */
  frames->setTextf("%d", sprite->getTotalFrames());

  window->remap_window();
  window->center_window();

  for (;;) {
    load_window_pos(window, "SpriteProperties");
    jwidget_show(window);
    window->open_window_fg();
    save_window_pos(window, "SpriteProperties");

    killer = window->get_killer();
    if (killer == ok)
      break;
    else if (killer == speed) {
      dialogs_frame_length(sprite, -1);
    }
    else
      break;
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_sprite_properties_command()
{
  return new SpritePropertiesCommand;
}
