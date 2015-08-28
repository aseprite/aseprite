// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file.h"

#include "app/console.h"
#include "app/context.h"
#include "app/document.h"
#include "app/file/file_format.h"
#include "app/file/file_formats_manager.h"
#include "app/file/format_options.h"
#include "app/file/split_filename.h"
#include "app/filename_formatter.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/status_bar.h"
#include "base/fs.h"
#include "base/mutex.h"
#include "base/path.h"
#include "base/scoped_lock.h"
#include "base/shared_ptr.h"
#include "base/string.h"
#include "doc/doc.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/alert.h"

#include <cstring>
#include <cstdarg>

namespace app {

using namespace base;

static FileOp* fop_new(FileOpType type, Context* context);
static void fop_prepare_for_sequence(FileOp* fop);

std::string get_readable_extensions()
{
  std::string buf;

  for (const FileFormat* format : *FileFormatsManager::instance()) {
    if (format->support(FILE_SUPPORT_LOAD)) {
      if (!buf.empty())
        buf.push_back(',');
      buf += format->extensions();
    }
  }

  return buf;
}

std::string get_writable_extensions()
{
  std::string buf;

  for (const FileFormat* format : *FileFormatsManager::instance()) {
    if (format->support(FILE_SUPPORT_SAVE)) {
      if (!buf.empty())
        buf.push_back(',');
      buf += format->extensions();
    }
  }

  return buf;
}

Document* load_document(Context* context, const char* filename)
{
  Document* document;

  /* TODO add a option to configure what to do with the sequence */
  FileOp *fop = fop_to_load_document(context, filename, FILE_LOAD_SEQUENCE_NONE);
  if (!fop)
    return NULL;

  /* operate in this same thread */
  fop_operate(fop, NULL);
  fop_done(fop);

  fop_post_load(fop);

  if (fop->has_error()) {
    Console console(context);
    console.printf(fop->error.c_str());
  }

  document = fop->document;
  fop_free(fop);

  if (document && context)
    document->setContext(context);

  return document;
}

int save_document(Context* context, doc::Document* document)
{
  ASSERT(dynamic_cast<app::Document*>(document));

  int ret;
  FileOp* fop = fop_to_save_document(context,
    static_cast<app::Document*>(document),
    document->filename().c_str(), "");
  if (!fop)
    return -1;

  /* operate in this same thread */
  fop_operate(fop, NULL);
  fop_done(fop);

  if (fop->has_error()) {
    Console console(context);
    console.printf(fop->error.c_str());
  }

  ret = (!fop->has_error() ? 0: -1);
  fop_free(fop);

  return ret;
}

FileOp* fop_to_load_document(Context* context, const char* filename, int flags)
{
  FileOp *fop;

  fop = fop_new(FileOpLoad, context);
  if (!fop)
    return NULL;

  // Get the extension of the filename (in lower case)
  std::string extension = base::string_to_lower(base::get_file_extension(filename));

  LOG("Loading file \"%s\" (%s)\n", filename, extension.c_str());

  // Does file exist?
  if (!base::is_file(filename)) {
    fop_error(fop, "File not found: \"%s\"\n", filename);
    goto done;
  }

  // Get the format through the extension of the filename
  fop->format = FileFormatsManager::instance()
    ->getFileFormatByExtension(extension.c_str());

  if (!fop->format ||
      !fop->format->support(FILE_SUPPORT_LOAD)) {
    fop_error(fop, "ASEPRITE can't load \"%s\" files\n", extension.c_str());
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
      std::string left, right;
      int c, width, start_from;
      char buf[512];

      /* first of all, we must generate the list of files to load in the
         sequence... */

      // Check is this could be a sequence
      start_from = split_filename(filename, left, right, width);
      if (start_from >= 0) {
        // Try to get more file names
        for (c=start_from+1; ; c++) {
          // Get the next file name
          sprintf(buf, "%s%0*d%s", left.c_str(), width, c, right.c_str());

          // If the file doesn't exist, we doesn't need more files to load
          if (!base::is_file(buf))
            break;

          /* add this file name to the list */
          fop->seq.filename_list.push_back(buf);
        }
      }

      /* TODO add a better dialog to edit file-names */
      if ((flags & FILE_LOAD_SEQUENCE_ASK) && context && context->isUIAvailable()) {
        /* really want load all files? */
        if ((fop->seq.filename_list.size() > 1) &&
            (ui::Alert::show("Notice"
              "<<Possible animation with:"
              "<<%s, %s..."
              "<<Do you want to load the sequence of bitmaps?"
              "||&Agree||&Skip",
              base::get_file_name(fop->seq.filename_list[0]).c_str(),
              base::get_file_name(fop->seq.filename_list[1]).c_str()) != 1)) {

          // If the user replies "Skip", we need just one file name
          // (the first one).
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

FileOp* fop_to_save_document(const Context* context, const Document* document,
  const char* filename, const char* fn_format_arg)
{
  FileOp *fop;
  bool fatal;

  fop = fop_new(FileOpSave, const_cast<Context*>(context));
  if (!fop)
    return NULL;

  // Document to save
  fop->document = const_cast<Document*>(document);

  // Get the extension of the filename (in lower case)
  std::string extension = base::string_to_lower(base::get_file_extension(filename));

  LOG("Saving document \"%s\" (%s)\n", filename, extension.c_str());

  // Get the format through the extension of the filename
  fop->format = FileFormatsManager::instance()
    ->getFileFormatByExtension(extension.c_str());

  if (!fop->format ||
      !fop->format->support(FILE_SUPPORT_SAVE)) {
    fop_error(fop, "ASEPRITE can't save \"%s\" files\n", extension.c_str());
    return fop;
  }

  // Warnings
  std::string warnings;
  fatal = false;

  /* check image type support */
  switch (fop->document->sprite()->pixelFormat()) {

    case IMAGE_RGB:
      if (!(fop->format->support(FILE_SUPPORT_RGB))) {
        warnings += "<<- RGB format";
        fatal = true;
      }

      if (!(fop->format->support(FILE_SUPPORT_RGBA)) &&
          fop->document->sprite()->needAlpha()) {

        warnings += "<<- Alpha channel";
      }
      break;

    case IMAGE_GRAYSCALE:
      if (!(fop->format->support(FILE_SUPPORT_GRAY))) {
        warnings += "<<- Grayscale format";
        fatal = true;
      }
      if (!(fop->format->support(FILE_SUPPORT_GRAYA)) &&
          fop->document->sprite()->needAlpha()) {

        warnings += "<<- Alpha channel";
      }
      break;

    case IMAGE_INDEXED:
      if (!(fop->format->support(FILE_SUPPORT_INDEXED))) {
        warnings += "<<- Indexed format";
        fatal = true;
      }
      break;
  }

  // Frames support
  if (fop->document->sprite()->totalFrames() > 1) {
    if (!fop->format->support(FILE_SUPPORT_FRAMES) &&
        !fop->format->support(FILE_SUPPORT_SEQUENCES)) {
      warnings += "<<- Frames";
    }
  }

  // Layers support
  if (fop->document->sprite()->folder()->getLayersCount() > 1) {
    if (!(fop->format->support(FILE_SUPPORT_LAYERS))) {
      warnings += "<<- Layers";
    }
  }

  // Palettes support
  if (fop->document->sprite()->getPalettes().size() > 1) {
    if (!fop->format->support(FILE_SUPPORT_PALETTES) &&
        !fop->format->support(FILE_SUPPORT_SEQUENCES)) {
      warnings += "<<- Palette changes between frames";
    }
  }

  // Check frames support
  if (!fop->document->sprite()->frameTags().empty()) {
    if (!fop->format->support(FILE_SUPPORT_FRAME_TAGS)) {
      warnings += "<<- Frame tags";
    }
  }

  // Big palettes
  if (!fop->format->support(FILE_SUPPORT_BIG_PALETTES)) {
    for (Palette* pal : fop->document->sprite()->getPalettes()) {
      if (pal->size() > 256) {
        warnings += "<<- Palettes with more than 256 colors";
        break;
      }
    }
  }

  // Palette with alpha
  if (!fop->format->support(FILE_SUPPORT_PALETTE_WITH_ALPHA)) {
    bool done = false;
    for (Palette* pal : fop->document->sprite()->getPalettes()) {
      for (int c=0; c<pal->size(); ++c) {
        if (rgba_geta(pal->getEntry(c)) < 255) {
          warnings += "<<- Palette with alpha channel";
          done = true;
          break;
        }
      }
      if (done)
        break;
    }
  }

  // Show the confirmation alert
  if (!warnings.empty()) {
    // Interative
    if (context && context->isUIAvailable()) {
      warnings += "<<You can use \".ase\" format to keep all this information.";

      std::string title, buttons;
      if (fatal) {
        title = "Error";
        buttons = "&Close";
      }
      else {
        title = "Warning";
        buttons = "&Yes||&No";
      }

      int ret = ui::Alert::show("%s<<File format \".%s\" doesn't support:%s"
        "<<Do you want continue with \".%s\" anyway?"
        "||%s",
        title.c_str(),
        fop->format->name(),
        warnings.c_str(),
        fop->format->name(),
        buttons.c_str());

      // Operation can't be done (by fatal error) or the user cancel
      // the operation
      if ((fatal) || (ret != 1)) {
        fop_free(fop);
        return NULL;
      }
    }
    // No interactive & fatal error?
    else if (fatal) {
      fop_error(fop, warnings.c_str());
      return fop;
    }
  }

  // Use the "sequence" interface.
  if (fop->format->support(FILE_SUPPORT_SEQUENCES)) {
    fop_prepare_for_sequence(fop);

    std::string fn = filename;
    std::string fn_format = fn_format_arg;
    bool default_format = false;

    if (fn_format.empty()) {
      if (fop->document->sprite()->totalFrames() == 1)
        fn_format = "{fullname}";
      else {
        fn_format = "{path}/{title}{frame}.{extension}";
        default_format = true;
      }
    }

    // Save one frame
    if (fop->document->sprite()->totalFrames() == 1) {
      FilenameInfo fnInfo;
      fnInfo.filename(fn);

      fn = filename_formatter(fn_format, fnInfo);
      fop->seq.filename_list.push_back(fn);
    }
    // Save multiple frames
    else {
      int width = 0;
      int start_from = 0;

      if (default_format) {
        std::string left, right;
        start_from = split_filename(fn.c_str(), left, right, width);
        if (start_from < 0) {
          start_from = 1;
          width = 1;
        }
        else {
          fn = left;
          fn += right;
        }
      }

      Sprite* spr = fop->document->sprite();
      std::vector<char> buf(32);
      std::sprintf(&buf[0], "{frame%0*d}", width, 0);
      if (default_format)
        fn_format = set_frame_format(fn_format, &buf[0]);
      else if (spr->totalFrames() > 1)
        fn_format = add_frame_format(fn_format, &buf[0]);

      for (frame_t frame(0); frame<spr->totalFrames(); ++frame) {
        FrameTag* innerTag = spr->frameTags().innerTag(frame);
        FrameTag* outerTag = spr->frameTags().outerTag(frame);
        FilenameInfo fnInfo;
        fnInfo
          .filename(fn)
          .innerTagName(innerTag ? innerTag->name(): "")
          .outerTagName(outerTag ? outerTag->name(): "")
          .frame(start_from+frame);

        std::string frame_fn =
          filename_formatter(fn_format, fnInfo);

        fop->seq.filename_list.push_back(frame_fn);
      }
    }
  }
  else
    fop->filename = filename;

  // Configure output format?
  if (fop->format->support(FILE_SUPPORT_GET_FORMAT_OPTIONS)) {
    base::SharedPtr<FormatOptions> format_options = fop->format->getFormatOptions(fop);

    // Does the user cancelled the operation?
    if (!format_options) {
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
void fop_operate(FileOp *fop, IFileOpProgress* progress)
{
  ASSERT(fop != NULL);
  ASSERT(!fop_is_done(fop));

  fop->progressInterface = progress;

  // Load //////////////////////////////////////////////////////////////////////
  if (fop->type == FileOpLoad &&
      fop->format != NULL &&
      fop->format->support(FILE_SUPPORT_LOAD)) {
    // Load a sequence
    if (fop->is_sequence()) {
      Image* old_image;
      bool loadres;

      // Default palette
      fop->seq.palette->makeBlack();

      // TODO set_palette for each frame???
#define SEQUENCE_IMAGE()                                                \
      do {                                                              \
        fop->seq.last_cel->data()->setImage(fop->seq.image);            \
        fop->seq.layer->addCel(fop->seq.last_cel);                      \
                                                                        \
        if (fop->document->sprite()->palette(frame)                     \
              ->countDiff(fop->seq.palette, NULL, NULL) > 0) {          \
          fop->seq.palette->setFrame(frame);                            \
          fop->document->sprite()->setPalette(fop->seq.palette, true);  \
        }                                                               \
                                                                        \
        old_image = fop->seq.image.get();                               \
        fop->seq.image.reset(NULL);                                     \
        fop->seq.last_cel = NULL;                                       \
      } while (0)

      // Load the sequence
      frame_t frames(fop->seq.filename_list.size());
      frame_t frame(0);
      old_image = nullptr;

      fop->seq.has_alpha = false;
      fop->seq.progress_offset = 0.0f;
      fop->seq.progress_fraction = 1.0f / (double)frames;

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
            fop->seq.image.reset();
            delete fop->seq.last_cel;
            delete fop->document;
            fop->document = nullptr;
            break;
          }
          // Read ok
          else {
            // Add the keyframe
            SEQUENCE_IMAGE();
          }
        }
        // For other frames
        else {
          // All done (or maybe not enough memory)
          if (!loadres || !fop->seq.last_cel) {
            fop->seq.image.reset();
            delete fop->seq.last_cel;
            break;
          }

          // Compare the old frame with the new one
#if USE_LINK // TODO this should be configurable through a check-box
          if (count_diff_between_images(old_image, fop->seq.image)) {
            SEQUENCE_IMAGE();
          }
          // We don't need this image
          else {
            delete fop->seq.image;

            // But add a link frame
            fop->seq.last_cel->image = image_index;
            layer_add_frame(fop->seq.layer, fop->seq.last_cel);

            fop->seq.last_image = NULL;
            fop->seq.last_cel = NULL;
          }
#else
          SEQUENCE_IMAGE();
#endif
        }

        ++frame;
        fop->seq.progress_offset += fop->seq.progress_fraction;
      }
      fop->filename = *fop->seq.filename_list.begin();

      // Final setup
      if (fop->document != NULL) {
        // Configure the layer as the 'Background'
        if (!fop->seq.has_alpha)
          fop->seq.layer->configureAsBackground();

        // Set the frames range
        fop->document->sprite()->setTotalFrames(frame);

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
#ifdef ENABLE_SAVE
    // Save a sequence
    if (fop->is_sequence()) {
      ASSERT(fop->format->support(FILE_SUPPORT_SEQUENCES));

      Sprite* sprite = fop->document->sprite();

      // Create a temporary bitmap
      fop->seq.image.reset(Image::create(sprite->pixelFormat(),
          sprite->width(),
          sprite->height()));

      fop->seq.progress_offset = 0.0f;
      fop->seq.progress_fraction = 1.0f / (double)sprite->totalFrames();

      // For each frame in the sprite.
      render::Render render;
      for (frame_t frame(0); frame < sprite->totalFrames(); ++frame) {
        // Draw the "frame" in "fop->seq.image"
        render.renderSprite(fop->seq.image.get(), sprite, frame);

        // Setup the palette.
        sprite->palette(frame)->copyColorsTo(fop->seq.palette);

        // Setup the filename to be used.
        fop->filename = fop->seq.filename_list[frame];

        // Call the "save" procedure... did it fail?
        if (!fop->format->save(fop)) {
          fop_error(fop, "Error saving frame %d in the file \"%s\"\n",
            frame+1, fop->filename.c_str());
          break;
        }

        fop->seq.progress_offset += fop->seq.progress_fraction;
      }
      fop->filename = *fop->seq.filename_list.begin();

      // Destroy the image
      fop->seq.image.reset(NULL);
    }
    // Direct save to a file.
    else {
      // Call the "save" procedure.
      if (!fop->format->save(fop))
        fop_error(fop, "Error saving the sprite in the file \"%s\"\n",
                  fop->filename.c_str());
    }
#else
    fop_error(fop,
      "Save operation is not supported in trial version.\n"
      "Go to " WEBSITE_DOWNLOAD " and get the full-version.");
#endif
  }

  // Progress = 100%
  fop_progress(fop, 1.0f);
}

// After mark the 'fop' as 'done' you must to free it calling fop_free().
void fop_done(FileOp *fop)
{
  // Finally done.
  scoped_lock lock(*fop->mutex);
  fop->done = true;
}

void fop_stop(FileOp *fop)
{
  scoped_lock lock(*fop->mutex);
  if (!fop->done)
    fop->stop = true;
}

FileOp::~FileOp()
{
  if (this->format)
    this->format->destroyData(this);

  delete this->seq.palette;
  delete this->mutex;
}

void FileOp::createDocument(Sprite* spr)
{
  // spr can be NULL if the sprite is set in onPostLoad() then

  ASSERT(this->document == NULL);
  this->document = new Document(spr);
}

void fop_free(FileOp *fop)
{
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

  Sprite* sprite = fop->document->sprite();
  if (sprite) {
    // Creates a suitable palette for RGB images
    if (sprite->pixelFormat() == IMAGE_RGB &&
        sprite->getPalettes().size() <= 1 &&
        sprite->palette(frame_t(0))->isBlack()) {
      base::SharedPtr<Palette> palette(
        render::create_palette_from_sprite(
          sprite, frame_t(0), sprite->lastFrame(), true,
          nullptr, nullptr));

      sprite->resetPalettes();
      sprite->setPalette(palette.get(), false);
    }
  }

  fop->document->markAsSaved();
}

void fop_sequence_set_format_options(FileOp* fop, const base::SharedPtr<FormatOptions>& format_options)
{
  ASSERT(!fop->seq.format_options);
  fop->seq.format_options = format_options;
}

void fop_sequence_set_ncolors(FileOp* fop, int ncolors)
{
  fop->seq.palette->resize(ncolors);
}

int fop_sequence_get_ncolors(FileOp* fop)
{
  return fop->seq.palette->size();
}

void fop_sequence_set_color(FileOp *fop, int index, int r, int g, int b)
{
  fop->seq.palette->setEntry(index, rgba(r, g, b, 255));
}

void fop_sequence_get_color(FileOp *fop, int index, int *r, int *g, int *b)
{
  uint32_t c;

  ASSERT(index >= 0);
  if (index >= 0 && index < fop->seq.palette->size())
    c = fop->seq.palette->getEntry(index);
  else
    c = rgba(0, 0, 0, 255);     // Black color

  *r = rgba_getr(c);
  *g = rgba_getg(c);
  *b = rgba_getb(c);
}

void fop_sequence_set_alpha(FileOp* fop, int index, int a)
{
  int c = fop->seq.palette->getEntry(index);
  int r = rgba_getr(c);
  int g = rgba_getg(c);
  int b = rgba_getb(c);

  fop->seq.palette->setEntry(index, rgba(r, g, b, a));
}

void fop_sequence_get_alpha(FileOp* fop, int index, int* a)
{
  ASSERT(index >= 0);
  if (index >= 0 && index < fop->seq.palette->size())
    *a = rgba_geta(fop->seq.palette->getEntry(index));
  else
    *a = 0;
}

Image* fop_sequence_image(FileOp* fop, PixelFormat pixelFormat, int w, int h)
{
  Sprite* sprite;

  // Create the image
  if (!fop->document) {
    sprite = new Sprite(pixelFormat, w, h, 256);
    try {
      LayerImage* layer = new LayerImage(sprite);

      // Add the layer
      sprite->folder()->addLayer(layer);

      // Done
      fop->createDocument(sprite);
      fop->seq.layer = layer;
    }
    catch (...) {
      delete sprite;
      throw;
    }
  }
  else {
    sprite = fop->document->sprite();

    if (sprite->pixelFormat() != pixelFormat)
      return NULL;
  }

  if (fop->seq.last_cel) {
    fop_error(fop, "Error: called two times \"fop_sequence_image()\".\n");
    return NULL;
  }

  // Create a bitmap
  fop->seq.image.reset(Image::create(pixelFormat, w, h));
  fop->seq.last_cel = new Cel(fop->seq.frame++, ImageRef(NULL));

  return fop->seq.image.get();
}

void fop_error(FileOp *fop, const char *format, ...)
{
  char buf_error[4096];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf_error, sizeof(buf_error), format, ap);
  va_end(ap);

  // Concatenate the new error
  {
    scoped_lock lock(*fop->mutex);
    fop->error += buf_error;
  }
}

void fop_progress(FileOp *fop, double progress)
{
  scoped_lock lock(*fop->mutex);

  if (fop->is_sequence()) {
    fop->progress =
      fop->seq.progress_offset +
      fop->seq.progress_fraction*progress;
  }
  else {
    fop->progress = progress;
  }

  if (fop->progressInterface)
    fop->progressInterface->ackFileOpProgress(progress);
}

double fop_get_progress(FileOp *fop)
{
  double progress;
  {
    scoped_lock lock(*fop->mutex);
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
    scoped_lock lock(*fop->mutex);
    done = fop->done;
  }
  return done;
}

bool fop_is_stop(FileOp *fop)
{
  bool stop;
  {
    scoped_lock lock(*fop->mutex);
    stop = fop->stop;
  }
  return stop;
}

static FileOp* fop_new(FileOpType type, Context* context)
{
  FileOp* fop = new FileOp;

  fop->type = type;
  fop->format = NULL;
  fop->format_data = NULL;
  fop->context = context;
  fop->document = NULL;

  fop->mutex = new base::mutex();
  fop->progress = 0.0f;
  fop->progressInterface = NULL;
  fop->done = false;
  fop->stop = false;
  fop->oneframe = false;

  fop->seq.palette = NULL;
  fop->seq.image.reset(NULL);
  fop->seq.progress_offset = 0.0f;
  fop->seq.progress_fraction = 0.0f;
  fop->seq.frame = frame_t(0);
  fop->seq.layer = NULL;
  fop->seq.last_cel = NULL;

  return fop;
}

static void fop_prepare_for_sequence(FileOp* fop)
{
  fop->seq.palette = new Palette(frame_t(0), 256);
  fop->seq.format_options.reset();
}

} // namespace app
