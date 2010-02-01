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

#include <stdio.h>
#include <string.h>

#include "jinete/jbase.h"

#include "util/filetoks.h"
#include "util/hash.h"

static HashTable* msgids = NULL;

static void free_msgid(void *msgid);

int msgids_load(const char *filename)
{
  char buf[4096], leavings[4096];
  char id[4096], trans[4096];
  int donotread = false;
  FILE *f;

  f = fopen(filename, "r");
  if (!f)
    return -1;

  if (!msgids)
    msgids = hash_new(64);

  tok_reset_line_num();
  strcpy(leavings, "");

  while (donotread || tok_read (f, buf, leavings, sizeof (leavings))) {
    donotread = false;

    /* new msgid */
    if (strncmp ("msgid", buf, 5) == 0) {
      strcpy(id, "");
      strcpy(trans, "");

      while (tok_read(f, buf, leavings, sizeof(leavings)) &&
	     strncmp("msgstr", buf, 6) != 0)
	strcat(id, buf);

      if (strncmp("msgstr", buf, 6) == 0) {
	while (tok_read(f, buf, leavings, sizeof(leavings)) &&
	       strncmp("msgid", buf, 5) != 0)
	  strcat(trans, buf);

	if (strncmp("msgid", buf, 5) == 0)
	  donotread = true;

	if (strlen(id) > 0 && strlen(trans) > 0)
	  hash_insert(msgids, id, jstrdup(trans));
      }
    }
  }

  fclose(f);
  return 0;
}

void msgids_clear()
{
  if (msgids) {
    hash_free(msgids, free_msgid);
    msgids = NULL;
  }
}

const char *msgids_get(const char *id)
{
  if (msgids) {
    const char* trans = reinterpret_cast<const char*>(hash_lookup(msgids, id));
    if (trans)
      return trans;
  }
  return id;
}

static void free_msgid(void *msgid)
{
  jfree(msgid);
}
