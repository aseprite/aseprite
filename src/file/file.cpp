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

#include <string.h>
#include <allegro.h>

#include "jinete/jalert.h"
#include "jinete/jlist.h"
#include "Vaca/Mutex.h"
#include "Vaca/ScopedLock.h"

#include "console.h"
#include "app.h"
#include "core/core.h"
#include "file/file.h"
#include "file/format_options.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "util/quantize.h"
#include "widgets/statebar.h"

using Vaca::Mutex;
using Vaca::ScopedLock;

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

static FileOp *fop_new(FileOpType type);
static void fop_prepare_for_sequence(FileOp *fop);

static FileFormat *get_fileformat(const char *extension);
static int split_filename(const char *filename, char *left, char *right, int *width);

void get_readable_extensions(char* buf, int size)
{
  int c;

  /* clear the string */
  ustrncpy(buf, empty_string, size);

  /* insert file format */
  for (c=0; formats[c]; c++) {
    if (formats[c]->load)
      ustrncat(buf, formats[c]->exts, size);

    if (formats[c+1] && formats[c]->load)
      ustrncat(buf, ",", size);
  }
}

void get_writable_extensions(char* buf, int size)
{
  int c;

  /* clear the string */
  ustrncpy(buf, empty_string, size);

  /* insert file format */
  for (c=0; formats[c]; c++) {
    if (formats[c]->save)
      ustrncat(buf, formats[c]->exts, size);

    if (formats[c+1] && formats[c]->save)
      ustrncat(buf, ",", size);
  }
}

Sprite *sprite_load(const char *filename)
{
  Sprite *sprite;
  /* TODO add a option to configure what to do with the sequence */
  FileOp *fop = fop_to_load_sprite(filename, FILE_LOAD_SEQUENCE_NONE);
  if (!fop)
    return NULL;

  /* operate in this same thread */
  fop_operate(fop);
  fop_done(fop);

  if (fop->error) {
    Console console;
    console.printf(fop->error);
  }

  sprite = fop->sprite;
  fop_free(fop);

  return sprite;
}

int sprite_save(Sprite *sprite)
{
  int ret;
  FileOp *fop = fop_to_save_sprite(sprite);
  if (!fop)
    return -1;

  /* operate in this same thread */
  fop_operate(fop);
  fop_done(fop);

  if (fop->error) {
    Console console;
    console.printf(fop->error);
  }

  ret = (fop->error == NULL) ? 0: -1;
  fop_free(fop);

  return ret;
}

FileOp *fop_to_load_sprite(const char *filename, int flags)
{
  char *extension;
  FileOp *fop;

  fop = fop_new(FileOpLoad);
  if (!fop)
    return NULL;

  /* get the extension of the filename (in lower case) */
  extension = jstrdup(get_extension(filename));
  ustrlwr(extension);

  PRINTF("Loading file \"%s\" (%s)\n", filename, extension);

  /* does file exist? */
  if (!file_exists(filename, FA_ALL, NULL)) {
    fop_error(fop, _("File not found: \"%s\"\n"), filename);
    goto done;
  }

  /* get the format through the extension of the filename */
  fop->format = get_fileformat(extension);
  if (!fop->format ||
      !fop->format->load) {
    fop_error(fop, _("ASE can't load \"%s\" files\n"), extension);
    goto done;
  }

  /* use the "sequence" interface */
  if (fop->format->flags & FILE_SUPPORT_SEQUENCES) {
    /* prepare to load a sequence */
    fop_prepare_for_sequence(fop);

    /* per now, we want load just one file */
    jlist_append(fop->seq.filename_list, jstrdup(filename));

    /* don't load the sequence (just the one file/one frame) */
    if (!(flags & FILE_LOAD_SEQUENCE_NONE)) {
      char buf[512], left[512], right[512];
      int c, width, start_from;

      /* first of all, we must generate the list of files to load in the
	 sequence... */

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
	  jlist_append(fop->seq.filename_list,
		       jstrdup(buf));
	}
      }

      /* TODO add a better dialog to edit file-names */
      if ((flags & FILE_LOAD_SEQUENCE_ASK) &&
	  is_interactive()) {
	/* really want load all files? */
	if ((jlist_length(fop->seq.filename_list) > 1) &&
	    (jalert(_("Notice"
		      "<<Possible animation with:"
		      "<<%s"
		      "<<Load the sequence of bitmaps?"
		      "||&Agree||&Skip"),
		    get_filename(filename)) != 1)) {
	  /* if the user replies "Skip", we need just one file name (the
	     first one) */
	  while (jlist_length(fop->seq.filename_list) > 1) {
	    JLink link = jlist_last(fop->seq.filename_list);
	    jfree(link->data);
	    jlist_delete_link(fop->seq.filename_list, link);
	  }
	}
      }
    }
  }
  else
    fop->filename = jstrdup(filename);

  /* load just one frame */
  if (flags & FILE_LOAD_ONE_FRAME)
    fop->oneframe = true;

done:;
  jfree(extension);
  return fop;
}

FileOp *fop_to_save_sprite(Sprite *sprite)
{
  char extension[32], buf[2048];
  FileOp *fop;
  bool fatal;

  fop = fop_new(FileOpSave);
  if (!fop)
    return NULL;

  /* sprite to save */
  fop->sprite = sprite;

  /* get the extension of the filename (in lower case) */
  ustrcpy(extension, get_extension(fop->sprite->getFilename()));
  ustrlwr(extension);

  PRINTF("Saving sprite \"%s\" (%s)\n", fop->sprite->getFilename(), extension);

  /* get the format through the extension of the filename */
  fop->format = get_fileformat(extension);
  if (!fop->format ||
      !fop->format->save) {
    fop_error(fop, _("ASE can't save \"%s\" files\n"), extension);
    return fop;
  }

  /* warnings */
  ustrcpy(buf, empty_string);
  fatal = false;

  /* check image type support */
  switch (fop->sprite->getImgType()) {

    case IMAGE_RGB:
      if (!(fop->format->flags & FILE_SUPPORT_RGB)) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("RGB format"));
	fatal = true;
      }
      if (!(fop->format->flags & FILE_SUPPORT_RGBA) &&
	  fop->sprite->needAlpha()) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Alpha channel"));
      }
      break;

    case IMAGE_GRAYSCALE:
      if (!(fop->format->flags & FILE_SUPPORT_GRAY)) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Grayscale format"));
	fatal = true;
      }
      if (!(fop->format->flags & FILE_SUPPORT_GRAYA) &&
	  fop->sprite->needAlpha()) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Alpha channel"));
      }
      break;

    case IMAGE_INDEXED:
      if (!(fop->format->flags & FILE_SUPPORT_INDEXED)) {
	usprintf(buf+ustrlen(buf), "<<- %s", _("Indexed format"));
	fatal = true;
      }
      break;
  }

  // check frames support
  if (!(fop->format->flags & (FILE_SUPPORT_FRAMES |
			      FILE_SUPPORT_SEQUENCES))) {
    if (fop->sprite->getTotalFrames() > 1)
      usprintf(buf+ustrlen(buf), "<<- %s", _("Frames"));
  }

  // layers support
  if (fop->sprite->getFolder()->get_layers_count() > 1) {
    if (!(fop->format->flags & FILE_SUPPORT_LAYERS)) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Layers"));
    }
  }

  /* palettes support */
  if (jlist_length(fop->sprite->getPalettes()) > 1) {
    if (!(fop->format->flags & (FILE_SUPPORT_PALETTES |
				FILE_SUPPORT_SEQUENCES))) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Palette changes between frames"));
    }
  }

  /* repositories */
  JList masks = fop->sprite->getMasksRepository();
  if (!jlist_empty(masks)) {
    Mask *mask;
    JLink link;
    int count = 0;

    JI_LIST_FOR_EACH(masks, link) {
      mask = reinterpret_cast<Mask*>(link->data);

      // Names starting with '*' are ignored
      if (mask->name && *mask->name == '*')
	continue;

      count++;
    }

    if ((count > 0) && !(fop->format->flags & FILE_SUPPORT_MASKS_REPOSITORY)) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Mask Repository"));
    }
  }

  if (!jlist_empty(fop->sprite->getPathsRepository())) {
    if (!(fop->format->flags & FILE_SUPPORT_PATHS_REPOSITORY)) {
      usprintf(buf+ustrlen(buf), "<<- %s", _("Path Repository"));
    }
  }

  /* show the confirmation alert */
  if (ugetc(buf)) {
    /* interative */
    if (is_interactive()) {
      int ret;

      if (fatal)
	ret = jalert(_("Error<<File format \"%s\" doesn't support:%s"
		       "||&Close"),
		     fop->format->name, buf);
      else
	ret = jalert(_("Warning<<File format \"%s\" doesn't support:%s"
		       "<<Do you want continue?"
		       "||&Yes||&No"),
		     fop->format->name, buf);

      /* operation can't be done (by fatal error) or the user cancel
	 the operation */
      if ((fatal) || (ret != 1)) {
	fop_free(fop);
	return NULL;
      }
    }
    /* no interactive & fatal error? */
    else if (fatal) {
      fop_error(fop, buf);
      return fop;
    }
  }

  /* use the "sequence" interface */
  if (fop->format->flags & FILE_SUPPORT_SEQUENCES) {
    fop_prepare_for_sequence(fop);

    /* to save one frame */
    if (fop->sprite->getTotalFrames() == 1) {
      jlist_append(fop->seq.filename_list,
		   jstrdup(fop->sprite->getFilename()));
    }
    /* to save more frames */
    else {
      char buf[256], left[256], right[256];
      int frame, width, start_from;

      start_from = split_filename(fop->sprite->getFilename(), left, right, &width);
      if (start_from < 0) {
	start_from = 0;
	width =
	  (fop->sprite->getTotalFrames() < 10)? 1:
	  (fop->sprite->getTotalFrames() < 100)? 2:
	  (fop->sprite->getTotalFrames() < 1000)? 3: 4;
      }

      for (frame=0; frame<fop->sprite->getTotalFrames(); frame++) {
	/* get the name for this frame */
	usprintf(buf, "%s%0*d%s", left, width, start_from+frame, right);
	jlist_append(fop->seq.filename_list, jstrdup(buf));
      }
    }
  }
  else
    fop->filename = jstrdup(fop->sprite->getFilename());

  /* configure output format? */
  if (fop->format->get_options != NULL) {
    FormatOptions *format_options = (fop->format->get_options)(fop);

    /* does the user cancelled the operation? */
    if (format_options == NULL) {
      fop_free(fop);
      return NULL;
    }

    fop->seq.format_options = format_options;
    fop->sprite->setFormatOptions(format_options);
  }

  return fop;
}

/**
 * Finally does the file operation: loading or saving the sprite.
 *
 * It can be called from a different thread of the one used
 * by @ref fop_to_load_sprite or @ref fop_to_save_sprite.
 *
 * After operate you must to mark the 'fop' as 'done' using @ref fop_done.
 */
void fop_operate(FileOp *fop)
{
  ASSERT(fop != NULL);
  ASSERT(!fop_is_done(fop));

  /* load ***********************************************************/
  if (fop->type == FileOpLoad &&
      fop->format != NULL &&
      fop->format->load != NULL) {
    /* load a sequence */
    if (fop->seq.filename_list != NULL) {
      int frame, frames, image_index = 0;
      Image *old_image;
      JLink link;
      bool loadres;

      /* default palette */
      fop->seq.palette->makeBlack();

       /* TODO set_palette for each frame??? */
#define SEQUENCE_IMAGE()						\
      do {								\
	image_index = stock_add_image(fop->sprite->getStock(),		\
				      fop->seq.image);			\
									\
	fop->seq.last_cel->image = image_index;				\
	fop->seq.layer->add_cel(fop->seq.last_cel);			\
									\
	if (fop->sprite->getPalette(frame)				\
	      ->countDiff(fop->seq.palette, NULL, NULL) > 0) {		\
	  fop->seq.palette->setFrame(frame);				\
	  fop->sprite->setPalette(fop->seq.palette, true);		\
	}								\
									\
	old_image = fop->seq.image;					\
	fop->seq.image = NULL;						\
	fop->seq.last_cel = NULL;					\
      } while (0)

      /* load the sequence */
      frames = jlist_length(fop->seq.filename_list);
      frame = 0;
      old_image = NULL;
      
      fop->seq.has_alpha = false;
      fop->seq.progress_offset = 0.0f;
      fop->seq.progress_fraction = 1.0f / (float)frames;

      JI_LIST_FOR_EACH(fop->seq.filename_list, link) {
	fop->filename = reinterpret_cast<char*>(link->data);

	/* call the "load" procedure to read the first bitmap */
	loadres = (*fop->format->load)(fop);
	if (!loadres) {
	  fop_error(fop, _("Error loading frame %d from file \"%s\"\n"),
		    frame+1, fop->filename);
	}

	/* for the first frame... */
	if (!old_image) {
	  /* error reading the first frame */
	  if (!loadres || !fop->sprite || !fop->seq.last_cel) {
	    if (fop->seq.image) image_free(fop->seq.image);
	    if (fop->seq.last_cel) cel_free(fop->seq.last_cel);
	    if (fop->sprite) {
	      delete fop->sprite;
	      fop->sprite = NULL;
	    }
	    break;
	  }
	  /* read ok */
	  else {
	    /* add the keyframe */
	    SEQUENCE_IMAGE();
	  }
	}
	/* for other frames */
	else {
	  /* all done (or maybe not enough memory) */
	  if (!loadres || !fop->seq.last_cel) {
	    if (fop->seq.image) image_free(fop->seq.image);
	    if (fop->seq.last_cel) cel_free(fop->seq.last_cel);

	    break;
	  }

	  /* compare the old frame with the new one */
#if USE_LINK /* TODO this should be configurable through a check-box */
	  if (image_count_diff(old_image, fop->seq.image)) {
	    SEQUENCE_IMAGE();
	  }
	  /* we don't need this image */
	  else {
	    image_free(fop->seq.image);

	    /* but add a link frame */
	    fop->seq.last_cel->image = image_index;
	    layer_add_frame(fop->seq.layer, fop->seq.last_cel);

	    fop->seq.last_image = NULL;
	    fop->seq.last_cel = NULL;
	  }
#else
	  SEQUENCE_IMAGE();
#endif
	}

	frame++;
	fop->seq.progress_offset += fop->seq.progress_fraction;
      }
      fop->filename = jstrdup((char*)jlist_first_data(fop->seq.filename_list));

      // Final setup
      if (fop->sprite != NULL) {
	// Configure the layer as the 'Background'
	if (!fop->seq.has_alpha)
	  fop->seq.layer->configure_as_background();

	// Set the frames range
	fop->sprite->setTotalFrames(frame);

	// Set the frames range
	fop->sprite->setFormatOptions(fop->seq.format_options);
      }
    }
    /* direct load from one file */
    else {
      /* call the "load" procedure */
      if (!(*fop->format->load)(fop))
	fop_error(fop, _("Error loading sprite from file \"%s\"\n"),
		  fop->filename);
    }

    if (fop->sprite != NULL) {
      /* select the last layer */
      if (fop->sprite->getFolder()->get_layers_count() > 0) {
	LayerIterator last_layer = --fop->sprite->getFolder()->get_layer_end();
	fop->sprite->setCurrentLayer(*last_layer);
      }

      /* set the filename */
      if (fop->seq.filename_list)
	fop->sprite->setFilename(reinterpret_cast<char*>(jlist_first_data(fop->seq.filename_list)));
      else
	fop->sprite->setFilename(fop->filename);

      // Quantize a palette for RGB images
      if (fop->sprite->getImgType() == IMAGE_RGB)
	sprite_quantize(fop->sprite);

      fop->sprite->markAsSaved();
    }
  }
  /* save ***********************************************************/
  else if (fop->type == FileOpSave &&
	   fop->format != NULL &&
	   fop->format->save != NULL) {
    /* save a sequence */
    if (fop->seq.filename_list != NULL) {
      ASSERT(fop->format->flags & FILE_SUPPORT_SEQUENCES);

      /* create a temporary bitmap */
      fop->seq.image = image_new(fop->sprite->getImgType(),
				 fop->sprite->getWidth(),
				 fop->sprite->getHeight());
      if (fop->seq.image != NULL) {
	int old_frame = fop->sprite->getCurrentFrame();

	fop->seq.progress_offset = 0.0f;
	fop->seq.progress_fraction = 1.0f / (float)fop->sprite->getTotalFrames();

	/* for each frame in the sprite */
	for (int frame=0; frame < fop->sprite->getTotalFrames(); ++frame) {
	  fop->sprite->setCurrentFrame(frame);

	  /* draw all the sprite in this frame in the image */
	  image_clear(fop->seq.image, 0);
	  fop->sprite->render(fop->seq.image, 0, 0);

	  /* setup the palette */
	  fop->sprite->getPalette(fop->sprite->getCurrentFrame())
	    ->copyColorsTo(fop->seq.palette);

	  /* setup the filename to be used */
	  fop->filename = reinterpret_cast<char*>
	    (jlist_nth_data(fop->seq.filename_list,
			    fop->sprite->getCurrentFrame()));

	  /* call the "save" procedure... did it fail? */
	  if (!(*fop->format->save)(fop)) {
	    fop_error(fop, _("Error saving frame %d in the file \"%s\"\n"),
		      fop->sprite->getCurrentFrame()+1, fop->filename);
	    break;
	  }

	  fop->seq.progress_offset += fop->seq.progress_fraction;
	}
	fop->filename = jstrdup(reinterpret_cast<char*>(jlist_first_data(fop->seq.filename_list)));

	// Destroy the image
	image_free(fop->seq.image);

	// Restore frame
	fop->sprite->setCurrentFrame(old_frame);
      }
      else {
	fop_error(fop, _("Not enough memory for the temporary bitmap.\n"));
      }
    }
    /* direct save to a file */
    else {
      /* call the "save" procedure */
      if (!(*fop->format->save)(fop))
	fop_error(fop, _("Error saving the sprite in the file \"%s\"\n"),
		  fop->filename);
    }
  }

  /* progress = 100% */
  fop_progress(fop, 1.0f);
}

/**
 * After mark the 'fop' as 'done' you must to free it calling @ref fop_free.
 */
void fop_done(FileOp *fop)
{
  /* finally done */
  ScopedLock lock(*fop->mutex);
  fop->done = true;
}

void fop_stop(FileOp *fop)
{
  ScopedLock lock(*fop->mutex);
  if (!fop->done)
    fop->stop = true;
}

void fop_free(FileOp *fop)
{
  if (fop->filename)
    jfree(fop->filename);

  if (fop->error)
    jfree(fop->error);

  if (fop->seq.filename_list) {
    JLink link;

    /* free old filenames strings */
    JI_LIST_FOR_EACH(fop->seq.filename_list, link)
      jfree(link->data);

    jlist_free(fop->seq.filename_list);
  }

  if (fop->seq.palette != NULL)
    delete fop->seq.palette;

  if (fop->mutex)
    delete fop->mutex;

  jfree(fop);
}

void fop_sequence_set_format_options(FileOp *fop, FormatOptions *format_options)
{
  ASSERT(fop->seq.format_options == NULL);
  fop->seq.format_options = format_options;
}

void fop_sequence_set_color(FileOp *fop, int index, int r, int g, int b)
{
  fop->seq.palette->setEntry(index, _rgba(r, g, b, 255));
}

void fop_sequence_get_color(FileOp *fop, int index, int *r, int *g, int *b)
{
  ase_uint32 c = fop->seq.palette->getEntry(index);

  *r = _rgba_getr(c);
  *g = _rgba_getg(c);
  *b = _rgba_getb(c);
}

Image *fop_sequence_image(FileOp *fop, int imgtype, int w, int h)
{
  Sprite* sprite;

  // Create the image
  if (!fop->sprite) {
    sprite = new Sprite(imgtype, w, h, 256);
    try {
      LayerImage* layer = new LayerImage(sprite);

      // Add the layer
      sprite->getFolder()->add_layer(layer);

      // Done
      fop->sprite = sprite;
      fop->seq.layer = layer;
    }
    catch (...) {
      delete sprite;
      throw;
    }
  }
  else {
    sprite = fop->sprite;

    if (sprite->getImgType() != imgtype)
      return NULL;
  }

  // Create a bitmap

  if (fop->seq.last_cel) {
    fop_error(fop, _("Error: called two times \"fop_sequence_image()\".\n"));
    return NULL;
  }

  Image* image = image_new(imgtype, w, h);
  if (!image) {
    fop_error(fop, _("Not enough memory to allocate a bitmap.\n"));
    return NULL;
  }

  fop->seq.image = image;
  fop->seq.last_cel = cel_new(fop->seq.frame++, 0);

  return image;
}

void fop_error(FileOp *fop, const char *format, ...)
{
  char buf_error[4096];
  va_list ap;

  va_start(ap, format);
  uvszprintf(buf_error, sizeof(buf_error), format, ap);
  va_end(ap);

  {
    ScopedLock lock(*fop->mutex);

    // Concatenate old errors with the new one
    if (fop->error) {
      char *old_error = fop->error;
      fop->error = reinterpret_cast<char*>(jmalloc(ustrsizez(old_error) + ustrsizez(buf_error) + 1));
      ustrcpy(fop->error, old_error);
      ustrcat(fop->error, buf_error);
      jfree(old_error);
    }
    /* first error */
    else
      fop->error = jstrdup(buf_error);
  }
}

void fop_progress(FileOp *fop, float progress)
{
  /* rest(8); */

  ScopedLock lock(*fop->mutex);

  if (fop->seq.filename_list != NULL) {
    fop->progress =
      fop->seq.progress_offset +
      fop->seq.progress_fraction*progress;
  }
  else {
    fop->progress = progress;
  }
}

float fop_get_progress(FileOp *fop)
{
  float progress;
  {
    ScopedLock lock(*fop->mutex);
    progress = fop->progress;
  }
  return progress;
}

/**
 * Returns true when the file operation finished, this means, when the
 * 'fop_operate()' routine ends.
 */
bool fop_is_done(FileOp *fop)
{
  bool done;
  {
    ScopedLock lock(*fop->mutex);
    done = fop->done;
  }
  return done;
}

bool fop_is_stop(FileOp *fop)
{
  bool stop;
  {
    ScopedLock lock(*fop->mutex);
    stop = fop->stop;
  }
  return stop;
}

static FileOp *fop_new(FileOpType type)
{
  FileOp *fop = jnew(FileOp, 1);
  if (!fop)
    return NULL;

  fop->type = type;
  fop->format = NULL;
  fop->sprite = NULL;
  fop->filename = NULL;

  fop->mutex = new Mutex();
  fop->progress = 0.0f;
  fop->error = NULL;
  fop->done = false;
  fop->stop = false;
  fop->oneframe = false;

  fop->seq.filename_list = NULL;
  fop->seq.palette = NULL;
  fop->seq.image = NULL;
  fop->seq.progress_offset = 0.0f;
  fop->seq.progress_fraction = 0.0f;
  fop->seq.frame = 0;
  fop->seq.layer = NULL;
  fop->seq.last_cel = NULL;
  
  return fop;
}

static void fop_prepare_for_sequence(FileOp *fop)
{
  fop->seq.filename_list = jlist_new();
  fop->seq.palette = new Palette(0, 256);
  fop->seq.format_options = NULL;
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


/**
 * Reads a WORD (16 bits) using in little-endian byte ordering.
 */
int fgetw(FILE *file)
{
  int b1, b2;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  /* little endian */
  return ((b2 << 8) | b1);
}

/**
 * Reads a DWORD (32 bits) using in little-endian byte ordering.
 */
long fgetl(FILE *file)
{
  int b1, b2, b3, b4;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  b3 = fgetc(file);
  if (b3 == EOF)
    return EOF;

  b4 = fgetc(file);
  if (b4 == EOF)
    return EOF;

  /* little endian */
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

/**
 * Writes a word using in little-endian byte ordering.
 * 
 * @return 0 in success or -1 in error
 */
int fputw(int w, FILE *file)
{
  int b1, b2;

  /* little endian */
  b2 = (w & 0xFF00) >> 8;
  b1 = w & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      return 0;

  return -1;
}

/**
 * Writes DWORD a using in little-endian byte ordering.
 *
 * @return 0 in success or -1 in error
 */
int fputl(long l, FILE *file)
{
  int b1, b2, b3, b4;

  /* little endian */
  b4 = (int)((l & 0xFF000000L) >> 24);
  b3 = (int)((l & 0x00FF0000L) >> 16);
  b2 = (int)((l & 0x0000FF00L) >> 8);
  b1 = (int)l & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      if (fputc(b3, file) == b3)
	if (fputc(b4, file) == b4)
	  return 0;

  return -1;
}
