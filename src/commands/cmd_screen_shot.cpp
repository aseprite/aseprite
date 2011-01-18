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

#include <allegro.h>

#include "gui/jsystem.h"

#include "commands/command.h"
#include "file/file.h"
#include "raster/raster.h"
#include "util/misc.h"

//////////////////////////////////////////////////////////////////////
// screen_shot

class ScreenShotCommand : public Command
{
public:
  ScreenShotCommand();
  Command* clone() { return new ScreenShotCommand(*this); }

protected:
  void onExecute(Context* context);
};

ScreenShotCommand::ScreenShotCommand()
  : Command("screen_shot",
	    "Screen Shot",
	    CmdUIOnlyFlag)
{
}

void ScreenShotCommand::onExecute(Context* context)
{
  int c, old_flag;
  char buf[512];
  PALETTE rgbpal;
  BITMAP *bmp;

  /* save the active flag which indicate if the mouse is freeze or not */
  old_flag = freeze_mouse_flag;

  /* freeze the mouse obligatory */
  freeze_mouse_flag = true;

  /* get the active palette color */
  get_palette(rgbpal);

  /* get a file name for the capture */
  for (c=0; c<10000; c++) {
    usprintf(buf, "shot%04d.%s", c, "png");
    if (!exists(buf))
      break;
  }

  if (ji_screen != screen)
    jmouse_draw_cursor();

  /* save in a bitmap the visible screen portion */
  bmp = create_sub_bitmap(ji_screen, 0, 0, JI_SCREEN_W, JI_SCREEN_H);

  /* creates a sprite with one layer, one image to save the bitmap as a PNG */
  {
    int imgtype = bitmap_color_depth(bmp) == 8 ? IMAGE_INDEXED: IMAGE_RGB;
    Sprite* sprite = Sprite::createWithLayer(imgtype, bmp->w, bmp->h, 256);
    Image* image = sprite->getCurrentImage();
    int x, y, r, g, b;

    Palette *pal = new Palette(0, 256);
    pal->fromAllegro(rgbpal);
    sprite->setPalette(pal, true);
    delete pal;

    /* convert Allegro "BITMAP" to ASE "Image" */
    if (imgtype == IMAGE_RGB) {
      ase_uint32 *address;

      for (y=0; y<image->h; ++y) {
	address = (ase_uint32 *)image->line[y];
	for (x=0; x<image->w; ++x) {
	  c = getpixel(bmp, x, y);
	  r = getr(c);
	  g = getg(c);
	  b = getb(c);
	  *(address++) = _rgba(r, g, b, 255);
	}
      }
    }
    else if (imgtype == IMAGE_INDEXED) {
      ase_uint8 *address;

      for (y=0; y<image->h; ++y) {
	address = (ase_uint8 *)image->line[y];
	for (x=0; x<image->w; ++x) {
	  *(address++) = getpixel(bmp, x, y);
	}
      }
    }

    /* save the sprite */
    sprite->setFilename(buf);
    sprite_save(sprite);

    /* destroy the sprite */
    delete sprite;
  }

  /* destroy the bitmap */
  destroy_bitmap(bmp);

  /* restore the freeze flag by the previous value */
  freeze_mouse_flag = old_flag;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_screen_shot_command()
{
  return new ScreenShotCommand;
}
