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

#include "test/test.h"

#include "jinete/jinete.h"
#include "core/file_system.h"

static void display_fileitem(FileItem *fi, int level, int deep)
{
  JList list;
  JLink link;
  int c;

  if (level == 0)
    printf("+ ");
  else
    printf("  ");

  for (c=0; c<level; ++c)
    printf("  ");

  printf("%s (%s)\n",
	 fileitem_get_filename(fi),
	 fileitem_get_displayname(fi));
  fflush(stdout);

  if (!fileitem_is_browsable(fi) || deep == 0)
    return;

  list = fileitem_get_children(fi);
  if (list) {
    JI_LIST_FOR_EACH(list, link) {
      display_fileitem(link->data, level+1, deep-1);
    }
  }
}

int main(int argc, char *argv[])
{
  allegro_init();
  assert(file_system_init());

  trace("*** Listing root of the file-system (deep = 2)...\n");
  {
    FileItem *root = get_root_fileitem();
    assert(root != NULL);
    display_fileitem(root, 0, 2);
  }

#ifdef ALLEGRO_WINDOWS
  trace("*** Testing specific directories retrieval...\n");
  {
    FileItem *c_drive, *c_drive2;
    FileItem *my_pc;

    trace("*** Getting 'C:\\' using 'get_fileitem_from_path'...\n");
    c_drive = get_fileitem_from_path("C:\\");
    assert(c_drive != NULL);

    trace("*** Getting 'C:\\' again\n");
    c_drive2 = get_fileitem_from_path("C:\\");
    assert(c_drive == c_drive2);

    trace("*** Displaying 'C:\\'...\n");
    display_fileitem(c_drive, 0, 0);

    trace("*** Getting 'My PC' (using 'fileitem_get_parent')...\n");
    my_pc = fileitem_get_parent(c_drive);
    assert(my_pc != NULL);

    trace("*** Listing 'My PC'...\n");
    display_fileitem(my_pc, 0, 1);
  }
#endif

  trace("*** Testing 'filename_has_extension'...\n");
  {
    assert(filename_has_extension("hi.png", "png"));
    assert(!filename_has_extension("hi.png", "pngg"));
    assert(!filename_has_extension("hi.png", "ppng"));
    assert(filename_has_extension("hi.jpeg", "jpg,jpeg"));
    assert(filename_has_extension("hi.jpg", "jpg,jpeg"));
    assert(!filename_has_extension("hi.ase", "jpg,jpeg"));
    assert(filename_has_extension("hi.ase", "jpg,jpeg,ase"));
    assert(filename_has_extension("hi.ase", "ase,jpg,jpeg"));
    trace("+ All OK.\n");
  }

  file_system_exit();
  allegro_exit();
  return 0;
}

END_OF_MAIN();
