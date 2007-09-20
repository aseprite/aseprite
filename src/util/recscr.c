/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <stdio.h>
#include <string.h>

#include "jinete/system.h"

#include "core/core.h"

#include "gfli.h"

#endif

/* change this number if you want more fluid recorded animations */
#define FRAMES_PER_SECOND   24

static FILE *rec_file = NULL;
static int rec_clock;

static s_fli_header *fli_header;
static unsigned char *omap;
static unsigned char *cmap;
static BITMAP *old_bmp;

bool is_rec_screen(void)
{
  return rec_file ? TRUE: FALSE;
}

void rec_screen_on(void)
{
  char buf[512];
  int c;

  if (!is_interactive ()
      || (bitmap_color_depth (ji_screen) != 8)
      || (rec_file))
    return;

  /* get a file name for the record */
  for (c=0; c<10000; c++) {
    usprintf (buf, "rec%04d.%s", c, "flc");
    if (!exists (buf))
      break;
  }

  /* open the file */
  rec_file = fopen (buf, "wb");
  if (!rec_file)
    return;

  /* start in position to write frames */
  fseek(rec_file, 128, SEEK_SET);

  /* prepare fli header */
  fli_header = jnew (s_fli_header, 1);
  fli_header->filesize = 0;
  fli_header->frames = 0;
  fli_header->width = JI_SCREEN_W;
  fli_header->height = JI_SCREEN_H;
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
  omap = jmalloc (768);
  cmap = jmalloc (768);

  /* prepare old bitmap */
  old_bmp = NULL;

  /* set the position of the mouse in the center of the screen */
  ji_mouse_set_position (JI_SCREEN_W/2, JI_SCREEN_H/2);

  /* start the clock */
  rec_clock = ji_clock;
}

void rec_screen_off(void)
{
  if (rec_file) {
    /* write the header and close the file */
    fli_write_header (rec_file, fli_header);
    fclose (rec_file);
    rec_file = NULL;

    /* free memory */
    if (old_bmp)
      destroy_bitmap (old_bmp);

    jfree (fli_header);
    jfree (cmap);
    jfree (omap);
  }
}

void rec_screen_poll (void)
{
  if (!is_interactive () || !rec_file)
    return;
  else if (ji_clock-rec_clock > JI_TICKS_PER_SEC/FRAMES_PER_SECOND) {
    int old_flag;
    BITMAP *bmp;
    int c, i;

    /* save the active flag which indicate if the mouse is freeze or not */
    old_flag = freeze_mouse_flag;

    /* freeze the mouse obligatory */
    freeze_mouse_flag = TRUE;

    /* get the active palette color */
    for (c=i=0; c<256; c++) {
      cmap[i++] = _rgb_scale_6[_current_palette[c].r];
      cmap[i++] = _rgb_scale_6[_current_palette[c].g];
      cmap[i++] = _rgb_scale_6[_current_palette[c].b];
    }

    /* save in a bitmap the visible screen portion */
    bmp = create_bitmap (JI_SCREEN_W, JI_SCREEN_H);
    blit (ji_screen, bmp, 0, 0, 0, 0, JI_SCREEN_W, JI_SCREEN_H);

    /* write the frame in FLC file */
    if (old_bmp)
      fli_write_frame (rec_file, fli_header,
		       (unsigned char *)old_bmp->dat, omap,
		       (unsigned char *)bmp->dat, cmap, W_ALL);
    else
      fli_write_frame (rec_file, fli_header, NULL, NULL,
		       (unsigned char *)bmp->dat, cmap, W_ALL);

    /* copy this palette to the old one */
    memcpy (omap, cmap, 768);

    /* fixup old bitmap */
    if (old_bmp)
      destroy_bitmap (old_bmp);

    old_bmp = bmp;

    /* restore the freeze flag by the previous value */
    freeze_mouse_flag = old_flag;

    /* update clock */
    rec_clock = ji_clock;
  }
}
