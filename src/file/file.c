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

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <string.h>

#include "jinete/jalert.h"
#include "jinete/jlist.h"

#include "console/console.h"
#include "core/app.h"
#include "core/core.h"
#include "file/file.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "raster/raster.h"
#include "widgets/statebar.h"

#endif

extern FileFormat format_ase;
extern FileFormat format_bmp;
extern FileFormat format_fli;
extern FileFormat format_jpeg;
extern FileFormat format_pcx;
extern FileFormat format_tga;
extern FileFormat format_gif;
extern FileFormat format_ico;
extern FileFormat format_png;

static FileFormat *formats[] =
{
  &format_ase,
  &format_bmp,
  &format_fli,
  &format_jpeg,
  &format_pcx,
  &format_tga,
  &format_gif,
  &format_ico,
  &format_png,
  NULL
};

/* structure used for the sequence of bitmaps */
static struct file_sequence_t {
  /* to load */
  Sprite *sprite;
  Layer *layer;
  int frpos;
  Image *last_image;
  Cel *last_cel;
  /* to save */
  Image *image;
} file_sequence;

static PALETTE file_palette;
static char file_extensions[512];

static FileFormat *get_fileformat(const char *extension);
static int split_filename(const char *filename, char *left, char *right, int *width);

void file_sequence_set_color(int index, int r, int g, int b)
{
  file_palette[index].r = r;
  file_palette[index].g = g;
  file_palette[index].b = b;
}

void file_sequence_get_color(int index, int *r, int *g, int *b)
{
  *r = file_palette[index].r;
  *g = file_palette[index].g;
  *b = file_palette[index].b;
}

Image *file_sequence_image(int imgtype, int w, int h)
{
  Sprite *sprite;
  Image *image;
  Layer *layer;

  /* create the image */
  if (!file_sequence.sprite) {
    sprite = sprite_new(imgtype, w, h);
    if (!sprite)
      return NULL;

    layer = layer_new(sprite);
    if (!layer) {
      sprite_free(sprite);
      return NULL;
    }

    layer_set_name(layer, _("Background"));

    /* add the layer */
    layer_add_layer(sprite->set, layer);

    /* done */
    file_sequence.sprite = sprite;
    file_sequence.layer = layer;
  }
  else {
    sprite = file_sequence.sprite;

    if (sprite->imgtype != imgtype)
      return NULL;
  }

  /* create a bitmap */

  if (file_sequence.last_cel) {
    console_printf(_("Error: called two times \"file_sequence_image()\".\n"));
    return NULL;
  }

  image = image_new(imgtype, w, h);
  if (!image) {
    console_printf(_("Not enough memory to allocate a bitmap.\n"));
    return NULL;
  }

  file_sequence.last_image = image;
  file_sequence.last_cel = cel_new(file_sequence.frpos++, 0);

  return image;
}

Sprite *file_sequence_sprite(void)
{
  return file_sequence.sprite;
}

Image *file_sequence_image_to_save(void)
{
  return file_sequence.image;
}

const char *get_readable_extensions(void)
{
  int c;

  /* clear the string */
  ustrcpy(file_extensions, empty_string);

  /* insert file format */
  for (c=0; formats[c]; c++) {
    if (formats[c]->load)
      ustrcat(file_extensions, formats[c]->exts);

    if (formats[c+1] && formats[c]->load)
      ustrcat(file_extensions, ",");
  }

  return file_extensions;
}

const char *get_writable_extensions(void)
{
  int c;

  /* clear the string */
  ustrcpy(file_extensions, empty_string);

  /* insert file format */
  for (c=0; formats[c]; c++) {
    if (formats[c]->save)
      ustrcat(file_extensions, formats[c]->exts);

    if (formats[c+1] && formats[c]->save)
      ustrcat(file_extensions, ",");
  }

  return file_extensions;
}

Sprite *sprite_load(const char *filename)
{
  char extension[32];
  FileFormat *file;
  Sprite *sprite;

  ustrcpy(extension, get_extension(filename));
  ustrlwr(extension);

  PRINTF("Loading file \"%s\" (%s)\n", filename, extension);

  file = get_fileformat(extension);
  if ((!file) || (!file->load)) {
    console_printf(_("Format \"%s\" isn't supported to open\n"), extension);
    return NULL;
  }

  /* default values */
  memcpy(file_palette, default_palette, sizeof(PALETTE));

  /* use the "sequence" interface */
  if (file->flags & FILE_SUPPORT_SEQUENCES) {
#define SEQUENCE_IMAGE()						\
    do {								\
      index = stock_add_image(file_sequence.sprite->stock,		\
			      file_sequence.last_image);		\
									\
      file_sequence.last_cel->image = index;				\
									\
      layer_add_cel(file_sequence.layer,				\
		    file_sequence.last_cel);				\
									\
      sprite_set_palette(file_sequence.sprite, file_palette, c);	\
									\
      old_image = file_sequence.last_image;				\
									\
      file_sequence.last_image = NULL;					\
      file_sequence.last_cel = NULL;					\
    } while (0)

    char buf[512], left[512], right[512];
    int c, index = 0, width, start_from, frames;
    Image *old_image;
/*     PALETTE first_palette; */
    JList file_names = jlist_new();
    JLink link;

    /* first of all, we must generate the list of files to load in the
       sequence... */

    /* per now, we want load just one file */
    jlist_append(file_names, jstrdup(filename));

    /* check is this could be a sequence */
    start_from = split_filename(filename, left, right, &width);
    if (start_from >= 0) {
      /* try to get more file names */
      for (c=start_from+1; ; c++) {
	/* get the next file name */
	usprintf(buf, "%s%0*d%s", left, width, c, right);

	/* if the file doesn't exist, we doesn't need more files to load */
	if (!exists(buf))
	  break;

	/* add this file name to the list */
	jlist_append(file_names, jstrdup(buf));
      }
    }

    if (is_interactive()) {
      /* really want load all files? */
      if ((jlist_length(file_names) > 1) &&
	  (jalert(_("Notice"
		    "<<Possible animation with:"
		    "<<%s"
		    "<<Load the sequence of bitmaps?"
		    "||&Agree||&Skip"),
		  get_filename(filename)) != 1)) {
	/* if the user replies "Skip", we need just one file name */
	while (jlist_length(file_names) > 1)
	  jlist_delete_link(file_names, jlist_last(file_names));
      }
    }

    /* count how many frames to load */
    frames = jlist_length(file_names);

    /* prepare the sequence */
    file_sequence.sprite = NULL;
    file_sequence.layer = NULL;
    file_sequence.frpos = 0;
    file_sequence.last_image = NULL;
    file_sequence.last_cel = NULL;

    /* load the sequence */
    c = 0;
    old_image = NULL;
    if (frames > 1)
      add_progress(frames);

    JI_LIST_FOR_EACH(file_names, link) {
      /* call the "load" procedure to read the first bitmap */
      add_progress(100);
      sprite = (*file->load)(link->data);
      del_progress();

      /* for the first frame */
      if (!old_image) {
	/* error reading the first frame */
	if ((!sprite) || (!file_sequence.last_cel)) {
	  if (file_sequence.last_image)
	    image_free(file_sequence.last_image);

	  if (file_sequence.last_cel)
	    cel_free(file_sequence.last_cel);

	  if (file_sequence.sprite) {
	    sprite_free(file_sequence.sprite);
	    file_sequence.sprite = NULL;
	  }

	  break;
	}
	/* read ok */
	else {
	  /* add the keyframe */
	  SEQUENCE_IMAGE();

	  /* the first palette will be the palette to use */
/* 	  memcpy(first_palette, file_palette, sizeof(PALETTE)); */
	}
      }
      /* for other frames */
      else {
	/* all done (or maybe not enough memory) */
	if ((!sprite) || (!file_sequence.last_cel)) {
	  if (file_sequence.last_image)
	    image_free(file_sequence.last_image);

	  if (file_sequence.last_cel)
	    cel_free(file_sequence.last_cel);

	  break;
	}

	/* compare the old frame with the new one */
#if USE_LINK /* TODO this should be configurable through a check-box */
	if (image_count_diff(old_image, file_sequence.last_image)) {
	  SEQUENCE_IMAGE();
	}
	/* we don't need this image */
	else {
	  image_free(file_sequence.last_image);

	  /* but add a link frame */
	  file_sequence.last_cel->image = index;
	  layer_add_frame(file_sequence.layer,
			  file_sequence.last_cel);

	  file_sequence.last_image = NULL;
	  file_sequence.last_cel = NULL;
	}
#else
	SEQUENCE_IMAGE();
#endif
      }

      c++;

      if (frames > 1)
	do_progress(c);
    }

    if (frames > 1)
      del_progress();

    /* the sprite */
    sprite = file_sequence.sprite;
    if (sprite) {
      /* set the frames range */
      sprite_set_frames(sprite, c);

      /* restore by the first palette */
      /* TODO remove this */
/*       memcpy(sprite->palette, first_palette, sizeof(PALETTE)); */
    }

    /* delete the entire file names list */
    JI_LIST_FOR_EACH(file_names, link)
      jfree(link->data);
    jlist_free(file_names);
  }
  /* direct load */
  else {
    /* call the "load" procedure */
    sprite = (*file->load)(filename);
  }

  /* error */
  if (!sprite)
    console_printf(_("Error loading \"%s\"\n"), filename);
  else {
    Layer *last_layer = jlist_last_data(sprite->set->layers);

    /* select the last layer */
    sprite_set_layer(sprite, last_layer);

    /* set the filename */
    sprite_set_filename(sprite, filename);
    sprite_mark_as_saved(sprite);

    rebuild_sprite_list();
  }

  return sprite;
}

int sprite_save(Sprite *sprite)
{
  char extension[32], buf[2048];
  FileFormat *file;
  int ret = -1;
  bool fatal;

  ustrcpy(extension, get_extension(sprite->filename));
  ustrlwr(extension);

  PRINTF("Saving sprite \"%s\" (%s)\n", sprite->filename, extension);

  file = get_fileformat(extension);
  if ((!file) || (!file->save)) {
    console_printf(_("Format \"%s\" isn't supported to save\n"), extension);
    return -1;
  }

  /* warnings */
  ustrcpy(buf, empty_string);
  fatal = FALSE;

  /* check image type support */
  switch (sprite->imgtype) {

    case IMAGE_RGB:
      if (!(file->flags & FILE_SUPPORT_RGB)) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("RGB format"));
	fatal = TRUE;
      }
      if (!(file->flags & FILE_SUPPORT_RGBA) &&
	  _rgba_geta(sprite->bgcolor) < 255) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Alpha channel"));
      }
      break;

    case IMAGE_GRAYSCALE:
      if (!(file->flags & FILE_SUPPORT_GRAY)) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Grayscale format"));
	fatal = TRUE;
      }
      if (!(file->flags & FILE_SUPPORT_GRAYA) &&
	  _graya_geta(sprite->bgcolor) < 255) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Alpha channel"));
      }
      break;

    case IMAGE_INDEXED:
      if (!(file->flags & FILE_SUPPORT_INDEXED)) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Indexed format"));
	fatal = TRUE;
      }
      break;
  }

  /* check frames support */
  if (!(file->flags & (FILE_SUPPORT_FRAMES |
		       FILE_SUPPORT_SEQUENCES))) {
    if (sprite->frames > 1)
      usprintf(buf+ustrlen(buf), "<<- %s", _("Frames"));
  }

  /* layers support */
  if (jlist_length(sprite->set->layers) > 1) {
    if (!(file->flags & FILE_SUPPORT_LAYERS)) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Layers"));
    }
  }

  /* palettes support */
  if (jlist_length(sprite->palettes) > 1) {
    if (!(file->flags & (FILE_SUPPORT_PALETTES |
			 FILE_SUPPORT_SEQUENCES))) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Palette changes"));
    }
  }

  /* repositories */
  if (!jlist_empty(sprite->repository.masks)) {
    Mask *mask;
    JLink link;
    int count = 0;

    JI_LIST_FOR_EACH(sprite->repository.masks, link) {
      mask = link->data;

      /* names starting with '*' are ignored */
      if (mask->name && *mask->name == '*')
	continue;

      count++;
    }

    if ((count > 0) && !(file->flags & FILE_SUPPORT_MASKS_REPOSITORY)) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Mask Repository"));
    }
  }

  if (!jlist_empty(sprite->repository.paths)) {
    if (!(file->flags & FILE_SUPPORT_PATHS_REPOSITORY)) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Path Repository"));
    }
  }

  /* show the confirmation alert */
  if (ugetc(buf)) {
    if (is_interactive()) {
      if (fatal)
	ret = jalert(_("Error<<File format \"%s\" doesn't support:%s"
		       "||&Close"),
		     file->name, buf);
      else
	ret = jalert(_("Warning<<File format \"%s\" doesn't support:%s"
		       "<<Do you want continue?"
		       "||&Yes||&No"),
		     file->name, buf);

      if ((fatal) || (ret != 1))
	return 0;
    }
  }

  /* palette */
/*   memcpy(file_palette, sprite->palette, sizeof(PALETTE)); */

  /* use the "sequence" interface */
  if (file->flags & FILE_SUPPORT_SEQUENCES) {
    Image *image;
    int old_frame = sprite->frame;
    char *old_filename = jstrdup(sprite->filename);

    /* create a temporary bitmap */
    image = image_new(sprite->imgtype, sprite->w, sprite->h);

    if (image) {
      /* save one frame */
      if (sprite->frames == 1) {
	/* draw all the sprite in the image */
	sprite->frame = 0;
	image_clear(image, 0);
	sprite_render(sprite, image, 0, 0);

	/* save the temporary image */
	file_sequence.image = image;

	palette_copy(file_palette, sprite_get_palette(sprite, sprite->frame));

	add_progress(100);
	ret = (*file->save)(sprite);
	del_progress();
      }
      /* save more frames */
      else {
	char buf[256], left[256], right[256];
	int width, start_from;

	start_from = split_filename(sprite->filename, left, right, &width);
	if (start_from < 0) {
	  start_from = 0;
	  width = (sprite->frames < 10)? 1:
		  (sprite->frames < 100)? 2:
		  (sprite->frames < 1000)? 3: 4;
	}

	add_progress(sprite->frames);

	for (sprite->frame=0; sprite->frame<sprite->frames; sprite->frame++) {
	  /* draw all the sprite in this frame in the image */
	  image_clear(image, 0);
	  sprite_render(sprite, image, 0, 0);

	  /* get the name for this image */
	  usprintf(buf, "%s%0*d%s", left, width, start_from+sprite->frame, right);

	  /* save the image */
	  file_sequence.image = image;
	  ustrcpy(sprite->filename, buf);

	  palette_copy(file_palette, sprite_get_palette(sprite, sprite->frame));

	  add_progress(100);
	  ret = (*file->save)(sprite);
	  del_progress();

	  /* error? */
	  if (ret != 0)
	    break;

	  do_progress(sprite->frame);
	}

	del_progress();
      }

      /* destroy the image */
      image_free(image);
    }
    else {
      console_printf(_("Not enough memory for the temporary bitmap.\n"));
    }

    sprite->frame = old_frame;
    ustrcpy(sprite->filename, old_filename);
    jfree(old_filename);
  }
  /* direct save */
  else {
    /* call the "save" procedure */
    ret = (*file->save)(sprite);
  }

  if (ret != 0)
    console_printf(_("Error saving \"%s\"\n"), sprite->filename);
  else
    status_bar_set_text(app_get_status_bar(), 1000,
			"File saved: %s", get_filename(sprite->filename));

  return ret;
}

static FileFormat *get_fileformat(const char *extension)
{
  char buf[512], *tok;
  int c;

  for (c=0; formats[c]; c++) {
    ustrcpy(buf, formats[c]->exts);

    for (tok=ustrtok(buf, ","); tok;
	 tok=ustrtok(NULL, ",")) {
      if (ustricmp(extension, tok) == 0)
	return formats[c];
    }
  }

  return NULL;
}

/* splits a file-name like "my_ani0000.pcx" to "my_ani" and ".pcx",
   returning the number of the center; returns "-1" if the function
   can't split anything */
static int split_filename(const char *filename, char *left, char *right, int *width)
{
  char *ptr, *ext;
  char buf[16];
  int chr, ret;

  /* get the extension */
  ext = get_extension(filename);

  /* with extension */
  if ((ext) && (*ext)) {
    /* left side (the filename without the extension and without the '.') */
    ext--;
    *ext = 0;
    ustrcpy(left, filename);
    *ext = '.';

    /* right side (the extension with the '.') */
    ustrcpy(right, ext);
  }
  /* without extension (without right side) */
  else {
    ustrcpy(left, filename);
    ustrcpy(right, empty_string);
  }

  /* remove all trailing numbers in the "left" side, and pass they to "buf" */

  ptr = buf+9;
  ptr[1] = 0;
  ret = -1;

  if (width)
    *width = 0;

  for (;;) {
    chr = ugetat(left, -1);
    if ((chr >= '0') && (chr <= '9')) {
      ret = 0;

      if (ptr >= buf) {
	*(ptr--) = chr;

	if (width)
	  (*width)++;
      }

      uremove(left, -1);
    }
    else
      break;
  }

  /* convert the "buf" to integer and return it */
  if (ret == 0) {
    while (ptr >= buf)
      *(ptr--) = '0';

    ret = ustrtol(buf, NULL, 10);
  }

  return ret;
}
