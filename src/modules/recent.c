/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include <stdio.h>
#include <string.h>

#include "jinete/jlist.h"

#include "core/app.h"
#include "core/cfg.h"
#include "modules/gui.h"
#include "modules/recent.h"

static int installed = FALSE;

static JList recent_files;
static int recent_files_limit = 16;

int init_module_recent(void)
{
  const char *filename;
  char buf[512];
  int c;

  installed = TRUE;
  recent_files = jlist_new();

  for (c=recent_files_limit-1; c>=0; c--) {
    sprintf(buf, "Filename%02d", c);
    filename = get_config_string("RecentFiles", buf, NULL);
    if ((filename) && (*filename))
      recent_file(filename);
  }

  return 0;
}

void exit_module_recent(void)
{
  char buf[512], *filename;
  JLink link;
  int c = 0;

  /* remove list of recent files */
  JI_LIST_FOR_EACH(recent_files, link) {
    filename = link->data;
    sprintf(buf, "Filename%02d", c);
    set_config_string("RecentFiles", buf, filename);
    jfree(filename);
    c++;
  }

  jlist_free(recent_files);
  installed = FALSE;
}

JList get_recent_files_list(void)
{
  return recent_files;
}

void recent_file(const char *filename)
{
  if (installed) {
    char *filename_it;
    int count = 0;
    JLink link;

    JI_LIST_FOR_EACH(recent_files, link) {
      filename_it = link->data;
      if (strcmp(filename, filename_it) == 0) {
	jlist_remove(recent_files, filename_it);
	jlist_prepend(recent_files, filename_it);
	schedule_rebuild_recent_list();
	return;
      }
      count++;
    }

    if (count == recent_files_limit) {
      /* remove the last recent-file entry */
      link = jlist_last(recent_files);

      jfree(link->data);
      jlist_delete_link(recent_files, link);
    }

    jlist_prepend(recent_files, jstrdup(filename));
    schedule_rebuild_recent_list();
  }
}

void unrecent_file(const char *filename)
{
  if (installed) {
    char *filename_it;
    JLink link;

    JI_LIST_FOR_EACH(recent_files, link) {
      filename_it = link->data;
      if (strcmp(filename, filename_it) == 0) {
	jfree(filename_it);
	jlist_delete_link(recent_files, link);
	schedule_rebuild_recent_list();
	break;
      }
    }
  }
}
