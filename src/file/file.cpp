/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "app.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/shared_ptr.h"
#include "base/string.h"
#include "console.h"
#include "document.h"
#include "file/file.h"
#include "file/file_format.h"
#include "file/file_formats_manager.h"
#include "file/format_options.h"
#include "gui/alert.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/quantization.h"
#include "raster/raster.h"
#include "widgets/statebar.h"

#include <allegro.h>
#include <string.h>

static FileOp* fop_new(FileOpType type);
static void fop_prepare_for_sequence(FileOp* fop);

static FileFormat* get_fileformat(const char* extension);
static int split_filename(const char* filename, char* left, char* right, int* width);

void get_readable_extensions(char* buf, int size)
{
  FileFormatsList::iterator it = FileFormatsManager::instance().begin();
  FileFormatsList::iterator end = FileFormatsManager::instance().end();

  /* clear the string */
  ustrncpy(buf, empty_string, size);

  /* insert file format */
  for (; it != end; ++it) {
    if ((*it)->support(FILE_SUPPORT_LOAD)) {
      if (ustrcmp(buf, empty_string) != 0)
      	ustrncat(buf, ",", size);
      ustrncat(buf, (*it)->extensions(), size);
    }
  }
}

void get_writable_extensions(char* buf, int size)
{
  FileFormatsList::iterator it = FileFormatsManager::instance().begin();
  FileFormatsList::iterator end = FileFormatsManager::instance().end();

  /* clear the string */
  ustrncpy(buf, empty_string, size);

  /* insert file format */
  for (; it != end; ++it) {
    if ((*it)->support(FILE_SUPPORT_SAVE)) {
      if (ustrcmp(buf, empty_string) != 0)
      	ustrncat(buf, ",", size);
      ustrncat(buf, (*it)->extensions(), size);
    }
  }
}

Document* load_document(const char* filename)
{
  Document* document;

  /* TODO add a option to configure what to do with the sequence */
  FileOp *fop = fop_to_load_document(filename, FILE_LOAD_SEQUENCE_NONE);
  if (!fop)
    return NULL;

  /* operate in this same thread */
  fop_operate(fop);
  fop_done(fop);

  fop_post_load(fop);

  if (fop->has_error()) {
    Console console;
    console.printf(fop->error.c_str());
  }

  document = fop->document;
  fop_free(fop);

  return document;
}

int save_document(Document* document)
{
  int ret;
  FileOp* fop = fop_to_save_document(document);
  if (!fop)
    return -1;

  /* operate in this same thread */
  fop_operate(fop);
  fop_done(fop);

  if (fop->has_error()) {
    Console console;
    console.printf(fop->error.c_str());
  }

  ret = (!fop->has_error() ? 0: -1);
  fop_free(fop);

  return ret;
}

FileOp* fop_to_load_document(const char* filename, int flags)
{
  FileOp *fop;

  fop = fop_new(FileOpLoad);
  if (!fop)
    return NULL;

  /* get the extension of the filename (in lower case) */
  std::string extension = base::string_to_lower(get_extension(filename));

  PRINTF("Loading file \"%s\" (%s)\n", filename, extension.c_str());

  /* does file exist? */
  if (!file_exists(filename, FA_ALL, NULL)) {
    fop_error(fop, "File not found: \"%s\"\n", filename);
    goto done;
  }

  /* get the format through the extension of the filename */
  fop->format = get_fileformat(extension.c_str());
  if (!fop->format ||
      !fop->format->support(FILE_SUPPORT_LOAD)) {
    fop_error(fop, "ASE can't load \"%s\" files\n", extension.c_str());
    goto done;
  }

  /* use the "sequence" interface */
  if (fop->format->support(FILE_SUPPORT_SEQUENCES)) {
    /* prepare to load a sequence */
    fop_prepare_for_sequence(fop);

    /* per now, we want load just one file */
    fop->seq.filename_list.push_back(filename);

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
	  fop->seq.filename_list.push_back(buf);
	}
      }

      /* TODO add a better dialog to edit file-names */
      if ((flags & FILE_LOAD_SEQUENCE_ASK) &&
	  App::instance()->isGui()) {
	/* really want load all files? */
	if ((fop->seq.filename_list.size() > 1) &&
	    (Alert::show("Notice"
			 "<<Possible animation with:"
			 "<<%s"
			 "<<Load the sequence of bitmaps?"
			 "||&Agree||&Skip",
			 get_filename(filename)) != 1)) {
	 
	  /* if the user replies "Skip", we need just one file name (the
	     first one) */
	  if (fop->seq.filename_list.size() > 1) {
	    fop->seq.filename_list.erase(fop->seq.filename_list.begin()+1,
					 fop->seq.filename_list.end());
	  }
	}
      }
    }
  }
  else
    fop->filename = filename;

  /* load just one frame */
  if (flags & FILE_LOAD_ONE_FRAME)
    fop->oneframe = true;

done:;
  return fop;
}

FileOp* fop_to_save_document(Document* document)
{
  char extension[32], buf[2048];
  FileOp *fop;
  bool fatal;

  fop = fop_new(FileOpSave);
  if (!fop)
    return NULL;

  /* document to save */
  fop->document = document;

  /* get the extension of the filename (in lower case) */
  ustrcpy(extension, get_extension(fop->document->getFilename()));
  ustrlwr(extension);

  PRINTF("Saving document \"%s\" (%s)\n", fop->document->getFilename(), extension);

  /* get the format through the extension of the filename */
  fop->format = get_fileformat(extension);
  if (!fop->format ||
      !fop->format->support(FILE_SUPPORT_SAVE)) {
    fop_error(fop, "ASE can't save \"%s\" files\n", extension);
    return fop;
  }

  /* warnings */
  ustrcpy(buf, empty_string);
  fatal = false;

  /* check image type support */
  switch (fop->document->getSprite()->getImgType()) {

    case IMAGE_RGB:
      if (!(fop->format->support(FILE_SUPPORT_RGB))) {
	usprintf(buf+ustrlen(buf), "<<- %s", "RGB format");
	fatal = true;
      }
      if (!(fop->format->support(FILE_SUPPORT_RGBA)) &&
	  fop->document->getSprite()->needAlpha()) {
	usprintf(buf+ustrlen(buf), "<<- %s", "Alpha channel");
      }
      break;

    case IMAGE_GRAYSCALE:
      if (!(fop->format->support(FILE_SUPPORT_GRAY))) {
	usprintf(buf+ustrlen(buf), "<<- Grayscale format");
	fatal = true;
      }
      if (!(fop->format->support(FILE_SUPPORT_GRAYA)) &&
	  fop->document->getSprite()->needAlpha()) {
	usprintf(buf+ustrlen(buf), "<<- Alpha channel");
      }
      break;

    case IMAGE_INDEXED:
      if (!(fop->format->support(FILE_SUPPORT_INDEXED))) {
	usprintf(buf+ustrlen(buf), "<<- Indexed format");
	fatal = true;
      }
      break;
  }

  // check frames support
  if (fop->document->getSprite()->getTotalFrames() > 1) {
    if (!fop->format->support(FILE_SUPPORT_FRAMES) &&
	!fop->format->support(FILE_SUPPORT_SEQUENCES)) {
      usprintf(buf+ustrlen(buf), "<<- Frames");
    }
  }

  // layers support
  if (fop->document->getSprite()->getFolder()->get_layers_count() > 1) {
    if (!(fop->format->support(FILE_SUPPORT_LAYERS))) {
      usprintf(buf+ustrlen(buf), "<<- Layers");
    }
  }

  // Palettes support.
  if (fop->document->getSprite()->getPalettes().size() > 1) {
    if (!fop->format->support(FILE_SUPPORT_PALETTES) &&
	!fop->format->support(FILE_SUPPORT_SEQUENCES)) {
      usprintf(buf+ustrlen(buf), "<<- Palette changes between frames");
    }
  }

  /* show the confirmation alert */
  if (ugetc(buf)) {
    /* interative */
    if (App::instance()->isGui()) {
      int ret;

      if (fatal)
	ret = Alert::show("Error<<File format \"%s\" doesn't support:%s"
			  "||&Close",
			  fop->format->name(), buf);
      else
	ret = Alert::show("Warning<<File format \"%s\" doesn't support:%s"
			  "<<Do you want continue?"
			  "||&Yes||&No",
			  fop->format->name(), buf);

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

  // Use the "sequence" interface.
  if (fop->format->support(FILE_SUPPORT_SEQUENCES)) {
    fop_prepare_for_sequence(fop);

    // To save one frame
    if (fop->document->getSprite()->getTotalFrames() == 1) {
      fop->seq.filename_list.push_back(fop->document->getFilename());
    }
    // To save more frames
    else {
      char buf[256], left[256], right[256];
      int frame, width, start_from;

      start_from = split_filename(fop->document->getFilename(), left, right, &width);
      if (start_from < 0) {
	start_from = 0;
	width =
	  (fop->document->getSprite()->getTotalFrames() < 10)? 1:
	  (fop->document->getSprite()->getTotalFrames() < 100)? 2:
	  (fop->document->getSprite()->getTotalFrames() < 1000)? 3: 4;
      }

      for (frame=0; frame<fop->document->getSprite()->getTotalFrames(); frame++) {
	/* get the name for this frame */
	usprintf(buf, "%s%0*d%s", left, width, start_from+frame, right);
	fop->seq.filename_list.push_back(buf);
      }
    }
  }
  else
    fop->filename = fop->document->getFilename();

  // Configure output format?
  if (fop->format->support(FILE_SUPPORT_GET_FORMAT_OPTIONS)) {
    SharedPtr<FormatOptions> format_options = fop->format->getFormatOptions(fop);

    // Does the user cancelled the operation?
    if (format_options == NULL) {
      fop_free(fop);
      return NULL;
    }

    fop->seq.format_options = format_options;
    fop->document->setFormatOptions(format_options);
  }

  return fop;
}

// Executes the file operation: loads or saves the sprite.
//
// It can be called from a different thread of the one used
// by fop_to_load_sprite() or fop_to_save_sprite().
//
// After this function you must to mark the "fop" as "done" calling
// fop_done() function.
void fop_operate(FileOp *fop)
{
  ASSERT(fop != NULL);
  ASSERT(!fop_is_done(fop));

  // Load //////////////////////////////////////////////////////////////////////
  if (fop->type == FileOpLoad &&
      fop->format != NULL &&
      fop->format->support(FILE_SUPPORT_LOAD)) {
    // Load a sequence
    if (fop->is_sequence()) {
      int frame, frames, image_index = 0;
      Image* old_image;
      bool loadres;

      // Default palette
      fop->seq.palette->makeBlack();

      // TODO set_palette for each frame???
#define SEQUENCE_IMAGE()						\
      do {								\
	image_index = fop->document					\
	  ->getSprite()							\
	  ->getStock()->addImage(fop->seq.image);			\
									\
	fop->seq.last_cel->setImage(image_index);			\
	fop->seq.layer->addCel(fop->seq.last_cel);			\
									\
	if (fop->document->getSprite()->getPalette(frame)		\
	      ->countDiff(fop->seq.palette, NULL, NULL) > 0) {		\
	  fop->seq.palette->setFrame(frame);				\
	  fop->document->getSprite()->setPalette(fop->seq.palette, true); \
	}								\
									\
	old_image = fop->seq.image;					\
	fop->seq.image = NULL;						\
	fop->seq.last_cel = NULL;					\
      } while (0)

      /* load the sequence */
      frames = fop->seq.filename_list.size();
      frame = 0;
      old_image = NULL;
      
      fop->seq.has_alpha = false;
      fop->seq.progress_offset = 0.0f;
      fop->seq.progress_fraction = 1.0f / (float)frames;

      std::vector<std::string>::iterator it = fop->seq.filename_list.begin();
      std::vector<std::string>::iterator end = fop->seq.filename_list.end();
      for (; it != end; ++it) {
	fop->filename = it->c_str();

	// Call the "load" procedure to read the first bitmap.
	loadres = fop->format->load(fop);
	if (!loadres) {
	  fop_error(fop, "Error loading frame %d from file \"%s\"\n",
		    frame+1, fop->filename.c_str());
	}

	// For the first frame...
	if (!old_image) {
	  // Error reading the first frame
	  if (!loadres || !fop->document || !fop->seq.last_cel) {
	    if (fop->seq.image) image_free(fop->seq.image);
	    if (fop->seq.last_cel) delete fop->seq.last_cel;
	    if (fop->document) {
	      delete fop->document;
	      fop->document = NULL;
	    }
	    break;
	  }
	  // Read ok
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
	    if (fop->seq.last_cel) delete fop->seq.last_cel;

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
      fop->filename = *fop->seq.filename_list.begin();

      // Final setup
      if (fop->document != NULL) {
	// Configure the layer as the 'Background'
	if (!fop->seq.has_alpha)
	  fop->seq.layer->configureAsBackground();

	// Set the frames range
	fop->document->getSprite()->setTotalFrames(frame);

	// Sets special options from the specific format (e.g. BMP
	// file can contain the number of bits per pixel).
	fop->document->setFormatOptions(fop->seq.format_options);
      }
    }
    // Direct load from one file.
    else {
      // Call the "load" procedure.
      if (!fop->format->load(fop))
	fop_error(fop, "Error loading sprite from file \"%s\"\n",
		  fop->filename.c_str());
    }
  }
  // Save //////////////////////////////////////////////////////////////////////
  else if (fop->type == FileOpSave &&
	   fop->format != NULL &&
	   fop->format->support(FILE_SUPPORT_SAVE)) {
    // Save a sequence
    if (fop->is_sequence()) {
      ASSERT(fop->format->support(FILE_SUPPORT_SEQUENCES));

      Sprite* sprite = fop->document->getSprite();

      // Create a temporary bitmap
      fop->seq.image = image_new(sprite->getImgType(),
				 sprite->getWidth(),
				 sprite->getHeight());
      if (fop->seq.image != NULL) {
	int old_frame = sprite->getCurrentFrame();

	fop->seq.progress_offset = 0.0f;
	fop->seq.progress_fraction = 1.0f / (float)sprite->getTotalFrames();

	// For each frame in the sprite.
	for (int frame=0; frame < sprite->getTotalFrames(); ++frame) {
	  // Draw the "frame" in "fop->seq.image"
	  sprite->setCurrentFrame(frame);
	  sprite->render(fop->seq.image, 0, 0);

	  // Setup the palette.
	  sprite->getPalette(sprite->getCurrentFrame())->copyColorsTo(fop->seq.palette);

	  // Setup the filename to be used.
	  fop->filename = fop->seq.filename_list[sprite->getCurrentFrame()];

	  // Call the "save" procedure... did it fail?
	  if (!fop->format->save(fop)) {
	    fop_error(fop, "Error saving frame %d in the file \"%s\"\n",
		      sprite->getCurrentFrame()+1, fop->filename.c_str());
	    break;
	  }

	  fop->seq.progress_offset += fop->seq.progress_fraction;
	}
	fop->filename = *fop->seq.filename_list.begin();

	// Destroy the image
	image_free(fop->seq.image);

	// Restore frame
	sprite->setCurrentFrame(old_frame);
      }
      else {
	fop_error(fop, "Not enough memory for the temporary bitmap.\n");
      }
    }
    // Direct save to a file.
    else {
      // Call the "save" procedure.
      if (!fop->format->save(fop))
	fop_error(fop, "Error saving the sprite in the file \"%s\"\n",
		  fop->filename.c_str());
    }
  }

  // Progress = 100%
  fop_progress(fop, 1.0f);
}

// After mark the 'fop' as 'done' you must to free it calling fop_free().
void fop_done(FileOp *fop)
{
  // Finally done.
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
  if (fop->format)
    fop->format->destroyData(fop);

  if (fop->seq.palette != NULL)
    delete fop->seq.palette;

  if (fop->mutex)
    delete fop->mutex;

  delete fop;
}

void fop_post_load(FileOp* fop)
{
  if (fop->document == NULL)
    return;

  // Set the filename.
  if (fop->is_sequence())
    fop->document->setFilename(fop->seq.filename_list.begin()->c_str());
  else
    fop->document->setFilename(fop->filename.c_str());

  bool result = fop->format->postLoad(fop);
  if (!result) {
    // Destroy the document
    delete fop->document;
    fop->document = NULL;

    return;
  }

  if (fop->document->getSprite() != NULL) {
    // Select the last layer
    if (fop->document->getSprite()->getFolder()->get_layers_count() > 0) {
      LayerIterator last_layer = --fop->document->getSprite()->getFolder()->get_layer_end();
      fop->document->getSprite()->setCurrentLayer(*last_layer);
    }

    // Creates a suitable palette for RGB images
    if (fop->document->getSprite()->getImgType() == IMAGE_RGB &&
	fop->document->getSprite()->getPalettes().size() <= 1 &&
	fop->document->getSprite()->getPalette(0)->isBlack()) {
      SharedPtr<Palette> palette(quantization::create_palette_from_rgb(fop->document->getSprite()));

      fop->document->getSprite()->resetPalettes();
      fop->document->getSprite()->setPalette(palette, false);
    }
  }

  fop->document->markAsSaved();
}

void fop_sequence_set_format_options(FileOp* fop, const SharedPtr<FormatOptions>& format_options)
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
  uint32_t c = fop->seq.palette->getEntry(index);

  *r = _rgba_getr(c);
  *g = _rgba_getg(c);
  *b = _rgba_getb(c);
}

Image* fop_sequence_image(FileOp* fop, int imgtype, int w, int h)
{
  Sprite* sprite;

  // Create the image
  if (!fop->document) {
    sprite = new Sprite(imgtype, w, h, 256);
    try {
      LayerImage* layer = new LayerImage(sprite);

      // Add the layer
      sprite->getFolder()->add_layer(layer);

      // Done
      fop->document = new Document(sprite);
      fop->seq.layer = layer;
    }
    catch (...) {
      delete sprite;
      throw;
    }
  }
  else {
    sprite = fop->document->getSprite();

    if (sprite->getImgType() != imgtype)
      return NULL;
  }

  if (fop->seq.last_cel) {
    fop_error(fop, "Error: called two times \"fop_sequence_image()\".\n");
    return NULL;
  }

  // Create a bitmap
  Image* image = image_new(imgtype, w, h);

  fop->seq.image = image;
  fop->seq.last_cel = new Cel(fop->seq.frame++, 0);

  return image;
}

void fop_error(FileOp *fop, const char *format, ...)
{
  char buf_error[4096];
  va_list ap;

  va_start(ap, format);
  uvszprintf(buf_error, sizeof(buf_error), format, ap);
  va_end(ap);

  // Concatenate the new error
  {
    ScopedLock lock(*fop->mutex);
    fop->error += buf_error;
  }
}

void fop_progress(FileOp *fop, float progress)
{
  //rest(8);

  ScopedLock lock(*fop->mutex);

  if (fop->is_sequence()) {
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

// Returns true when the file operation finished, this means, when the
// fop_operate() routine ends.
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

static FileOp* fop_new(FileOpType type)
{
  FileOp* fop = new FileOp;

  fop->type = type;
  fop->format = NULL;
  fop->format_data = NULL;
  fop->document = NULL;

  fop->mutex = new Mutex();
  fop->progress = 0.0f;
  fop->done = false;
  fop->stop = false;
  fop->oneframe = false;

  fop->seq.palette = NULL;
  fop->seq.image = NULL;
  fop->seq.progress_offset = 0.0f;
  fop->seq.progress_fraction = 0.0f;
  fop->seq.frame = 0;
  fop->seq.layer = NULL;
  fop->seq.last_cel = NULL;
  
  return fop;
}

static void fop_prepare_for_sequence(FileOp* fop)
{
  fop->seq.palette = new Palette(0, 256);
  fop->seq.format_options.reset();
}

static FileFormat* get_fileformat(const char* extension)
{
  FileFormatsList::iterator it = FileFormatsManager::instance().begin();
  FileFormatsList::iterator end = FileFormatsManager::instance().end();
  char buf[512], *tok;

  for (; it != end; ++it) {
    ustrcpy(buf, (*it)->extensions());

    for (tok=ustrtok(buf, ","); tok;
	 tok=ustrtok(NULL, ",")) {
      if (ustricmp(extension, tok) == 0)
	return (*it);
    }
  }

  return NULL;
}

// Splits a file-name like "my_ani0000.pcx" to "my_ani" and ".pcx",
// returning the number of the center; returns "-1" if the function
// can't split anything
static int split_filename(const char* filename, char* left, char* right, int* width)
{
  char *ptr, *ext;
  char buf[16];
  int chr, ret;

  // Get the extension.
  ext = get_extension(filename);

  // With extension.
  if ((ext) && (*ext)) {
    // Left side (the filename without the extension and without the '.').
    ext--;
    *ext = 0;
    ustrcpy(left, filename);
    *ext = '.';

    // Right side (the extension with the '.').
    ustrcpy(right, ext);
  }
  // Without extension (without right side).
  else {
    ustrcpy(left, filename);
    ustrcpy(right, empty_string);
  }

  // Remove all trailing numbers in the "left" side, and pass they to "buf".

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

  // Convert the "buf" to integer and return it.
  if (ret == 0) {
    while (ptr >= buf)
      *(ptr--) = '0';

    ret = ustrtol(buf, NULL, 10);
  }

  return ret;
}


// Reads a WORD (16 bits) using in little-endian byte ordering.
int fgetw(FILE* file)
{
  int b1, b2;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  // Little endian.
  return ((b2 << 8) | b1);
}

// Reads a DWORD (32 bits) using in little-endian byte ordering.
long fgetl(FILE* file)
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

  // Little endian.
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

// Writes a word using in little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputw(int w, FILE* file)
{
  int b1, b2;

  // Little endian.
  b2 = (w & 0xFF00) >> 8;
  b1 = w & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      return 0;

  return -1;
}

// Writes DWORD a using in little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputl(long l, FILE* file)
{
  int b1, b2, b3, b4;

  // Little endian.
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
