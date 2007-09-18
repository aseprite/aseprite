/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#include "jinete/base.h"
#include "jinete/list.h"

#include "file/ase.h"
#include "modules/sprites.h"
#include "raster/layer.h"
#include "raster/sprite.h"

#endif

#define FA_ALL       FA_RDONLY | FA_DIREC | FA_ARCH | FA_HIDDEN | FA_SYSTEM

bool load_session(const char *filename)
{
  bool ret = TRUE;
  Sprite *sprite;
  char buf[512];
  FILE *f;
  int n;

  f = fopen(filename, "rb");
  if (!f)
    return FALSE;

  /* how many sprites */
  n = fgetw(f);

  /* read sprites */
  while (n > 0) {
    /* read sprite name */
    fread(buf, 512, 1, f);

    /* read sprite */
    sprite = ase_file_read_f (f);

    /* ok? */
    if (sprite) {
      sprite_set_filename(sprite, buf);

      if (!jlist_empty(sprite->set->layers))
	sprite_set_layer(sprite, jlist_first_data(sprite->set->layers));

      sprite_mount(sprite);
    }
    else {
      ret = FALSE;
      break;
    }
    n--;
  }

  fclose(f);
  return ret;
}

bool save_session(const char *filename)
{
  JList list = get_sprite_list();
  bool ret = TRUE;
  Sprite *sprite;
  JLink link;
  FILE *f;

  f = fopen (filename, "wb");
  if (!f)
    return FALSE;

  /* how many sprites */
  fputw(jlist_length(list), f);

  /* write sprites */
  JI_LIST_FOR_EACH(list, link) {
    sprite = link->data;

    /* write sprite name */
    fwrite(sprite->filename, 512, 1, f);

    /* write sprite */
    if (ase_file_write_f(f, sprite) < 0) {
      ret = FALSE;
      break;
    }
  }

  fclose(f);
  return ret;
}

/* returns true if a ase*.ses file exists (a backup saved session from
   an old crash) */
bool is_backup_session(void)
{
  char buf[1024], path[1024], name[1024];
  struct al_ffblk info;
  int ret;

  get_executable_name(path, sizeof (path));
  usprintf(name, "data/session/ase*.ses");
  replace_filename(buf, path, name, sizeof (buf));

  ret = al_findfirst(buf, &info, FA_ALL);
  al_findclose(&info);

  return (ret == 0);
}
