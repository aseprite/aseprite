/* ASE - Allegro Sprite Editor
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

#include <allegro/config.h>
#include <allegro/file.h>
#include <allegro/unicode.h>

#include "jinete/rect.h"

#include "core/cfg.h"
#include "core/core.h"
#include "core/dirs.h"

#endif

static char config_filename[512];

void ase_config_init(void)
{
  DIRS *dirs, *dir;

  dirs = cfg_filename_dirs();

  /* search the configuration file from first to last path */
  for (dir=dirs; dir; dir=dir->next) {
    if ((dir->path) && exists(dir->path)) {
      ustrcpy(config_filename, dir->path);
      break;
    }
  }

  /* if the file wasn't found, we will create configuration file
     in the first path */
  if (!dir)
    ustrcpy(config_filename, dirs->path);

  dirs_free(dirs);

  override_config_file(config_filename);
}

void ase_config_exit(void)
{
  flush_config_file();
/*   override_config_file(NULL); */
}

bool get_config_bool(const char *section, const char *name, bool value)
{
  const char *got = get_config_string(section, name, value ? "yes": "no");
  return (got &&
	  (ustricmp(got, "yes") == 0 ||
	   ustricmp(got, "true") == 0 ||
	   ustricmp(got, "1") == 0)) ? TRUE: FALSE;
}

void set_config_bool(const char *section, const char *name, bool value)
{
  set_config_string(section, name, value ? "yes": "no");
}

void get_config_rect(const char *section, const char *name, JRect rect)
{
  char **argv;
  int argc;

  argv = get_config_argv(section, name, &argc);
  if (argv && argc == 4) {
    rect->x1 = ustrtol(argv[0], NULL, 10);
    rect->y1 = ustrtol(argv[1], NULL, 10);
    rect->x2 = ustrtol(argv[2], NULL, 10);
    rect->y2 = ustrtol(argv[3], NULL, 10);
  }
}

void set_config_rect(const char *section, const char *name, JRect rect)
{
  char buf[128];
  usprintf(buf, "%d %d %d %d", rect->x1, rect->y1, rect->x2, rect->y2);
  set_config_string(section, name, buf);
}
