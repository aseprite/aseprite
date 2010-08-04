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
#include <stdio.h>
#include <string.h>

#include "jinete/jsystem.h"

#include "core/core.h"
#include "file/fli/fli.h"

/* change this number if you want more fluid recorded animations */
#define FRAMES_PER_SECOND   24

static FILE *rec_file = NULL;
static int rec_clock;

static s_fli_header *fli_header;
static unsigned char *omap;
static unsigned char *cmap;
static BITMAP *new_bmp;
static BITMAP *old_bmp;

bool is_rec_screen()
{
  return rec_file ? true: false;
}

void rec_screen_on()
{
  char buf[512];
  int c;

/*   if (!is_interactive() */
/* /\*       || (bitmap_color_depth(ji_screen) != 8) *\/ */
/*       || (rec_file)) */
/*     return; */

  ASSERT(rec_file == NULL);

  /* get a file name for the record */
  for (c=0; c<10000; c++) {
    usprintf(buf, "rec%04d.%s", c, "flc");
    if (!exists(buf))
      break;
  }

  /* open the file */
  rec_file = fopen(buf, "wb");
  if (!rec_file)
    return;

  /* start in position to write frames */
  fseek(rec_file, 128, SEEK_SET);

  /* prepare fli header */
  fli_header = jnew(s_fli_header, 1);
  fli_header->filesize = 0;
  fli_header->frames = 0;
  fli_header->width = SCREEN_W;
  fli_header->height = SCREEN_H;
  fli_header->magic = HEADER_FLC;
  fli_header->depth = 8;
  fli_header->flags = 3;
  fli_header->speed = 1000 / FRAMES_PER_SECOND + 8;
  fli_header->created = 0;
  fli_header->updated = 0;
  fli_header->aspect_x = 1;
  fli_header->aspect_y = 1;
  fli_header->oframe1 = fli_header->oframe2 = 0;

  /* prepare maps */
  omap = (unsigned char*)jmalloc(768);
  cmap = (unsigned char*)jmalloc(768);

  /* prepare old bitmap */
  new_bmp = NULL;
  old_bmp = NULL;

  /* set the position of the mouse in the center of the screen */
  jmouse_set_position(JI_SCREEN_W/2, JI_SCREEN_H/2);

  /* start the clock */
  rec_clock = ji_clock;
}

void rec_screen_off()
{
  if (rec_file) {
    /* write the header and close the file */
    fli_write_header(rec_file, fli_header);
    fclose(rec_file);
    rec_file = NULL;

    /* free memory */
    if (new_bmp) destroy_bitmap(new_bmp);
    if (old_bmp) destroy_bitmap(old_bmp);

    jfree(fli_header);
    jfree(cmap);
    jfree(omap);
  }
}

void rec_screen_poll()
{
  if (!is_interactive() || !rec_file)
    return;
  else if (ji_clock-rec_clock > 1000/FRAMES_PER_SECOND) {
    BITMAP *t;
    int old_flag;
    int c, i;

    /* save the active flag which indicate if the mouse is freeze or not */
    old_flag = freeze_mouse_flag;

    /* freeze the mouse obligatory */
    freeze_mouse_flag = true;

    /* get the active palette color */
    for (c=i=0; c<256; c++) {
      cmap[i++] = _rgb_scale_6[_current_palette[c].r];
      cmap[i++] = _rgb_scale_6[_current_palette[c].g];
      cmap[i++] = _rgb_scale_6[_current_palette[c].b];
    }

    /* save in a bitmap the visible screen portion */
    if (!new_bmp) new_bmp = create_bitmap_ex(8, SCREEN_W, SCREEN_H);
/*     if (ji_screen != screen) */
/*       jmouse_draw_cursor(); */
    blit(screen, new_bmp, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

    /* write the frame in FLC file */
    if (old_bmp)
      fli_write_frame(rec_file, fli_header,
		      (unsigned char *)old_bmp->dat, omap,
		      (unsigned char *)new_bmp->dat, cmap, W_ALL);
    else
      fli_write_frame(rec_file, fli_header, NULL, NULL,
		      (unsigned char *)new_bmp->dat, cmap, W_ALL);

    /* copy this palette to the old one */
    memcpy(omap, cmap, 768);

    /* fixup old bitmap */
/*     if (old_bmp) */
/*       destroy_bitmap(old_bmp); */

    /* swap bitmaps */
    t = old_bmp;
    old_bmp = new_bmp;
    new_bmp = t;

    /* restore the freeze flag by the previous value */
    freeze_mouse_flag = old_flag;

    /* update clock */
    rec_clock = ji_clock;
  }
}
