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

#include <allegro.h>

#include "jinete/jinete.h"

#include "ui_context.h"
#include "commands/command.h"
#include "app.h"
#include "core/cfg.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// duplicate_sprite

class DuplicateSpriteCommand : public Command
{
public:
  DuplicateSpriteCommand();
  Command* clone() { return new DuplicateSpriteCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

DuplicateSpriteCommand::DuplicateSpriteCommand()
  : Command("duplicate_sprite",
	    "Duplicate Sprite",
	    CmdUIOnlyFlag)
{
}

bool DuplicateSpriteCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void DuplicateSpriteCommand::execute(Context* context)
{
  JWidget src_name, dst_name, flatten;
  const CurrentSpriteReader sprite(context);
  char buf[1024];

  /* load the window widget */
  FramePtr window(load_widget("duplicate_sprite.xml", "duplicate_sprite"));

  src_name = jwidget_find_name(window, "src_name");
  dst_name = jwidget_find_name(window, "dst_name");
  flatten = jwidget_find_name(window, "flatten");

  src_name->setText(get_filename(sprite->getFilename()));

  sprintf(buf, "%s %s", sprite->getFilename(), _("Copy"));
  dst_name->setText(buf);

  if (get_config_bool("DuplicateSprite", "Flatten", false))
    jwidget_select(flatten);

  /* open the window */
  window->open_window_fg();

  if (window->get_killer() == jwidget_find_name(window, "ok")) {
    set_config_bool("DuplicateSprite", "Flatten",
		    jwidget_is_selected(flatten));

    // make a copy of the current sprite
    Sprite *sprite_copy;
    if (jwidget_is_selected(flatten))
      sprite_copy = Sprite::createFlattenCopy(*sprite);
    else
      sprite_copy = new Sprite(*sprite);

    if (sprite_copy != NULL) {
      sprite_copy->setFilename(dst_name->getText());

      context->add_sprite(sprite_copy);
      set_sprite_in_more_reliable_editor(sprite_copy);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_duplicate_sprite_command()
{
  return new DuplicateSpriteCommand;
}
