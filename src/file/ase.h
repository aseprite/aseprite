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

#ifndef FILE_ASE_H
#define FILE_ASE_H

#include <stdio.h>

struct Sprite;

struct Sprite *ase_file_read_f (FILE *f);
int ase_file_write_f (FILE *f, struct Sprite *sprite);

int fgetw (FILE *file);
long fgetl (FILE *file);
int fputw (int w, FILE *file);
int fputl (long l, FILE *file);

#endif /* FILE_ASE_H */
