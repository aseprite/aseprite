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
#include <math.h>
#include <stdio.h>

#include "raster/raster.h"

void test ()
{
  Image *image_screen;
  Image *image_bg;
  Sprite *sprite;
  BITMAP *bmp;

  bmp = create_bitmap (SCREEN_W, SCREEN_H);
  sprite = sprite_new (IMAGE_RGB, 128, 128);
  image_screen = image_new (IMAGE_RGB, SCREEN_W, SCREEN_H);
  image_bg = image_new (IMAGE_RGB, SCREEN_W, SCREEN_H);

  /* build bg */
  {
    int x, y, c;
    for (y=0; y<image_bg->h; y++) {
      for (x=0; x<image_bg->w; x++) {
	c = ((255 * y / (image_bg->h-1)) +
	     (255 * x / (image_bg->w-1))) / 2;
	c += (rand () % 3) - 1;
	c = MID (0, c, 255);
	image_bg->method->putpixel (image_bg, x, y,
				    _rgba (c, c, c, 255));
      }
    }

    image_line (image_bg, image_bg->w-1, 0, 0, image_bg->h-1,
		_rgba (255, 0, 0, 255));
  }

  /* build the layers */
  {
    Layer *layer = layer_new(sprite);

    /* add this layer to the sprite */
    layer_add_layer(sprite->set, layer);

    /* build the "stock" and the properties */
    {
      int i, c, image_index;
      Image *image;
      Cel *cel;

      for (i=0; i<32; i++) {
	/* make the image */
	image = image_new (sprite->imgtype, sprite->w, sprite->h);
	image_clear (image, 0);
	for (c=0; c<image->w/2; c++) {
	  int r = 255*i/31;
	  int g = (255*c/(image->w/2-1)*0.8)-r;
	  int b = 255*c/(image->w/2-1)-r;
	  int a = 255*c/(image->w/2-1);
	  image_ellipsefill (image, c, c, image->w-1-c, image->h-1-c,
			    _rgba (MID(0, r, 255), MID(0, g, 255),
				   MID(0, b, 255), MID(0, a, 255)));
	}

	/* add the new image in the stock */
	image_index = stock_add_image(sprite->stock, image);

	/* cel properties */
	cel = cel_new(i, image_index);
	cel_set_position(cel,
			 cos(AL_PI*i/16)*64,
			 sin(AL_PI*i/16)*64);
	cel_set_opacity(cel, 128+sin(AL_PI*i/32)*127);

	layer_add_cel(layer, cel);
      }
    }
  }

  do {
    image_copy(image_screen, image_bg, 0, 0);
    sprite_render(sprite, image_screen,
		  mouse_x-sprite->w/2, mouse_y-sprite->h/2);
    image_to_allegro(image_screen, bmp, 0, 0);

    if (key[KEY_LEFT]) sprite->frame--;
    if (key[KEY_RIGHT]) sprite->frame++;

    text_mode(makecol (0, 0, 0));
    textprintf(bmp, font, 0, 0, makecol(255, 255, 255), "%d", sprite->frame);
    blit(bmp, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
  } while (!key[KEY_ESC]);

  image_free(image_screen);
  image_free(image_bg);
  delete sprite;
  destroy_bitmap(bmp);
}

int main(int argc, char *argv[])
{
  allegro_init();
  set_color_depth(16);
  set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0);
  install_timer();
  install_keyboard();
  install_mouse();

  set_palette(default_palette);

  test();
  return 0;
}

END_OF_MAIN();
