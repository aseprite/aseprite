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

#ifndef CORE_DIRS_H
#define CORE_DIRS_H

typedef struct DIRS DIRS;

struct DIRS
{
  char *path;
  struct DIRS *next;
};

DIRS *dirs_new(void);
void dirs_free(DIRS *dirs);
void dirs_add_path(DIRS *dirs, const char *path);
void dirs_cat_dirs(DIRS *dirs, DIRS *more);

DIRS *filename_in_bindir(const char *filename);
DIRS *filename_in_datadir(const char *filename);
DIRS *filename_in_homedir(const char *filename);
DIRS *filename_in_homedir(const char *filename);
DIRS *cfg_filename_dirs(void);

#endif /* CORE_DIRS_H */

