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

#ifndef FILE_H
#define FILE_H

#define FILE_SUPPORT_RGB		(1<<0)
#define FILE_SUPPORT_RGBA		(1<<1)
#define FILE_SUPPORT_GRAY		(1<<2)
#define FILE_SUPPORT_GRAYA		(1<<3)
#define FILE_SUPPORT_INDEXED		(1<<4)
#define FILE_SUPPORT_LAYERS		(1<<5)
#define FILE_SUPPORT_FRAMES		(1<<6)
#define FILE_SUPPORT_PALETTES		(1<<7)
#define FILE_SUPPORT_SEQUENCES		(1<<8)
#define FILE_SUPPORT_MASKS_REPOSITORY	(1<<9)
#define FILE_SUPPORT_PATHS_REPOSITORY	(1<<10)

typedef struct Sprite *(*FileLoad) (const char *filename);
typedef int (*FileSave) (struct Sprite *sprite);

/* load or/and save a file type */
typedef struct FileType
{
  const char *name;	/* file type name */
  const char *exts;	/* extensions (e.g. "jpeg,jpg") */
  FileLoad load;	/* procedure to read a sprite in this format */
  FileSave save;	/* procedure to write a sprite in this format */
  int flags;
} FileType;

/* routines to handle sequences */

void file_sequence_set_color(int index, int r, int g, int b);
void file_sequence_get_color(int index, int *r, int *g, int *b);
struct Image *file_sequence_image(int imgtype, int w, int h);
struct Sprite *file_sequence_sprite(void);
struct Image *file_sequence_image_to_save(void);

/* available extensions for each load/save operation */

const char *get_readable_extensions(void);
const char *get_writable_extensions(void);

/* mainly routines to load/save images */

struct Sprite *sprite_load(const char *filename);
int sprite_save(struct Sprite *sprite);

#endif /* FILE_H */
