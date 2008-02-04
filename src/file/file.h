/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007, 2008  David A. Capello
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

#include "jinete/jbase.h"
#include <allegro/color.h>

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

struct Image;
struct Cel;
struct Layer;
struct Sprite;

struct FileFormat;
struct FileOp;

/* file operations */
typedef enum { FileOpLoad, FileOpSave } FileOpType;
typedef bool (*FileLoad)(struct FileOp *fop);
typedef bool (*FileSave)(struct FileOp *fop);

/* load or/and save a file format */
typedef struct FileFormat
{
  const char *name;	/* file format name */
  const char *exts;	/* extensions (e.g. "jpeg,jpg") */
  FileLoad load;	/* procedure to read a sprite in this format */
  FileSave save;	/* procedure to write a sprite in this format */
  int flags;
} FileFormat;

/* structure to load & save files */
typedef struct FileOp
{
  FileOpType type;		/* operation type: 0=load, 1=save */
  FileFormat *format;
  struct Sprite *sprite;	/* loaded sprite/sprite to be saved */
  char *filename;		/* file-name to load/save */

  /* shared fields between threads */
  JMutex mutex;			/* mutex to access to the next two fields */
  float progress;		/* progress (1.0 is ready) */
  char *error;			/* error string */
  bool done : 1;		/* true if the operation finished */
  bool stop : 1;		/* force the break of the operation */

  /* data for sequences */
  struct {
    JList filename_list;	/* all file names to load/save */
    RGB *palette;		/* palette of the sequence */
    struct Image *image;	/* image to be saved/loaded */
    /* for the progress bar */
    float progress_offset;	/* progress offset from the current frame */
    float progress_fraction;	/* progress fraction for one frame */
    /* to load sequences */
    int frame;
    struct Layer *layer;
    struct Cel *last_cel;
  } seq;
} FileOp;

/* available extensions for each load/save operation */

void get_readable_extensions(char *buf, int size);
void get_writable_extensions(char *buf, int size);

/* high-level routines to load/save sprites */

struct Sprite *sprite_load(const char *filename);
int sprite_save(struct Sprite *sprite);

/* low-level routines to load/save sprites */

FileOp *fop_to_load_sprite(const char *filename);
FileOp *fop_to_save_sprite(struct Sprite *sprite);
void fop_operate(FileOp *fop);
void fop_free(FileOp *fop);

void fop_sequence_set_color(FileOp *fop, int index, int r, int g, int b);
void fop_sequence_get_color(FileOp *fop, int index, int *r, int *g, int *b);
struct Image *fop_sequence_image(FileOp *fi, int imgtype, int w, int h);

void fop_error(FileOp *fop, const char *error, ...);
void fop_progress(FileOp *fop, float progress);

float fop_get_progress(FileOp *fop);
bool fop_is_done(FileOp *fop);
bool fop_is_stop(FileOp *fop);

#endif /* FILE_H */
