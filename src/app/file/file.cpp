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
  /* TODO add a option to configure what to do with the sequence */
  base::UniquePtr<FileOp> fop(FileOp::createLoadDocumentOperation(context, filename, FILE_LOAD_SEQUENCE_NONE));
  if (!fop)
    return nullptr;

  // Operate in this same thread
  fop->operate();
  fop->done();
  fop->postLoad();

  if (fop->hasError()) {
    Console console(context);
    console.printf(fop->error().c_str());
  }

  Document* document = fop->releaseDocument();
  fop.release();

  if (document && context)
    document->setContext(context);

  return document;
}

int save_document(Context* context, doc::Document* document)
{
  ASSERT(dynamic_cast<app::Document*>(document));

  UniquePtr<FileOp> fop(
    FileOp::createSaveDocumentOperation(
      context,
      static_cast<app::Document*>(document),
      document->filename().c_str(), ""));
  if (!fop)
    return -1;

  // Operate in this same thread
  fop->operate();
  fop->done();

  if (fop->hasError()) {
    Console console(context);
    console.printf(fop->error().c_str());
  }

  return (!fop->hasError() ? 0: -1);
}

// static
FileOp* FileOp::createLoadDocumentOperation(Context* context, const char* filename, int flags)
{
  base::UniquePtr<FileOp> fop(
    new FileOp(FileOpLoad, context));
  if (!fop)
    return nullptr;

  // Get the extension of the filename (in lower case)
  std::string extension = base::string_to_lower(base::get_file_extension(filename));

  LOG("Loading file \"%s\" (%s)\n", filename, extension.c_str());

  // Does file exist?
  if (!base::is_file(filename)) {
    fop->setError("File not found: \"%s\"\n", filename);
    goto done;
  }

  // Get the format through the extension of the filename
  fop->m_format = FileFormatsManager::instance()
    ->getFileFormatByExtension(extension.c_str());

  if (!fop->m_format ||
      !fop->m_format->support(FILE_SUPPORT_LOAD)) {
    fop->setError("%s can't load \"%s\" files\n", PACKAGE, extension.c_str());
    goto done;
  }

  /* use the "sequence" interface */
  if (fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
    /* prepare to load a sequence */
    fop->prepareForSequence();

    /* per now, we want load just one file */
    fop->m_seq.filename_list.push_back(filename);

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
          fop->m_seq.filename_list.push_back(buf);
        }
      }

      /* TODO add a better dialog to edit file-names */
      if ((flags & FILE_LOAD_SEQUENCE_ASK) && context && context->isUIAvailable()) {
        /* really want load all files? */
        if ((fop->m_seq.filename_list.size() > 1) &&
            (ui::Alert::show("Notice"
              "<<Possible animation with:"
              "<<%s, %s..."
              "<<Do you want to load the sequence of bitmaps?"
              "||&Agree||&Skip",
              base::get_file_name(fop->m_seq.filename_list[0]).c_str(),
              base::get_file_name(fop->m_seq.filename_list[1]).c_str()) != 1)) {

          // If the user replies "Skip", we need just one file name
          // (the first one).
          if (fop->m_seq.filename_list.size() > 1) {
            fop->m_seq.filename_list.erase(fop->m_seq.filename_list.begin()+1,
                                           fop->m_seq.filename_list.end());
          }
        }
      }
    }
  }
  else
    fop->m_filename = filename;

  /* load just one frame */
  if (flags & FILE_LOAD_ONE_FRAME)
    fop->m_oneframe = true;

done:;
  return fop.release();
}

// static
FileOp* FileOp::createSaveDocumentOperation(const Context* context,
                                            const Document* document,
                                            const char* filename,
                                            const char* fn_format_arg)
{
  base::UniquePtr<FileOp> fop(
    new FileOp(FileOpSave, const_cast<Context*>(context)));

  // Document to save
  fop->m_document = const_cast<Document*>(document);

  // Get the extension of the filename (in lower case)
  std::string extension = base::string_to_lower(base::get_file_extension(filename));

  LOG("Saving document \"%s\" (%s)\n", filename, extension.c_str());

  // Get the format through the extension of the filename
  fop->m_format = FileFormatsManager::instance()
    ->getFileFormatByExtension(extension.c_str());

  if (!fop->m_format ||
      !fop->m_format->support(FILE_SUPPORT_SAVE)) {
    fop->setError("%s can't save \"%s\" files\n", PACKAGE, extension.c_str());
    return fop.release();
  }

  // Warnings
  std::string warnings;
  bool fatal = false;

  /* check image type support */
  switch (fop->m_document->sprite()->pixelFormat()) {

    case IMAGE_RGB:
      if (!(fop->m_format->support(FILE_SUPPORT_RGB))) {
        warnings += "<<- RGB format";
        fatal = true;
      }

      if (!(fop->m_format->support(FILE_SUPPORT_RGBA)) &&
          fop->m_document->sprite()->needAlpha()) {

        warnings += "<<- Alpha channel";
      }
      break;

    case IMAGE_GRAYSCALE:
      if (!(fop->m_format->support(FILE_SUPPORT_GRAY))) {
        warnings += "<<- Grayscale format";
        fatal = true;
      }
      if (!(fop->m_format->support(FILE_SUPPORT_GRAYA)) &&
          fop->m_document->sprite()->needAlpha()) {

        warnings += "<<- Alpha channel";
      }
      break;

    case IMAGE_INDEXED:
      if (!(fop->m_format->support(FILE_SUPPORT_INDEXED))) {
        warnings += "<<- Indexed format";
        fatal = true;
      }
      break;
  }

  // Frames support
  if (fop->m_document->sprite()->totalFrames() > 1) {
    if (!fop->m_format->support(FILE_SUPPORT_FRAMES) &&
        !fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
      warnings += "<<- Frames";
    }
  }

  // Layers support
  if (fop->m_document->sprite()->folder()->getLayersCount() > 1) {
    if (!(fop->m_format->support(FILE_SUPPORT_LAYERS))) {
      warnings += "<<- Layers";
    }
  }

  // Palettes support
  if (fop->m_document->sprite()->getPalettes().size() > 1) {
    if (!fop->m_format->support(FILE_SUPPORT_PALETTES) &&
        !fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
      warnings += "<<- Palette changes between frames";
    }
  }

  // Check frames support
  if (!fop->m_document->sprite()->frameTags().empty()) {
    if (!fop->m_format->support(FILE_SUPPORT_FRAME_TAGS)) {
      warnings += "<<- Frame tags";
    }
  }

  // Big palettes
  if (!fop->m_format->support(FILE_SUPPORT_BIG_PALETTES)) {
    for (const Palette* pal : fop->m_document->sprite()->getPalettes()) {
      if (pal->size() > 256) {
        warnings += "<<- Palettes with more than 256 colors";
        break;
      }
    }
  }

  // Palette with alpha
  if (!fop->m_format->support(FILE_SUPPORT_PALETTE_WITH_ALPHA)) {
    bool done = false;
    for (const Palette* pal : fop->m_document->sprite()->getPalettes()) {
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
        fop->m_format->name(),
        warnings.c_str(),
        fop->m_format->name(),
        buttons.c_str());

      // Operation can't be done (by fatal error) or the user cancel
      // the operation
      if ((fatal) || (ret != 1))
        return nullptr;
    }
    // No interactive & fatal error?
    else if (fatal) {
      fop->setError(warnings.c_str());
      return fop.release();
    }
  }

  // Use the "sequence" interface.
  if (fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
    fop->prepareForSequence();

    std::string fn = filename;
    std::string fn_format = fn_format_arg;
    bool default_format = false;

    if (fn_format.empty()) {
      if (fop->m_document->sprite()->totalFrames() == 1)
        fn_format = "{fullname}";
      else {
        fn_format = "{path}/{title}{frame}.{extension}";
        default_format = true;
      }
    }

    // Save one frame
    if (fop->m_document->sprite()->totalFrames() == 1) {
      FilenameInfo fnInfo;
      fnInfo.filename(fn);

      fn = filename_formatter(fn_format, fnInfo);
      fop->m_seq.filename_list.push_back(fn);
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

      Sprite* spr = fop->m_document->sprite();
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
          .frame(start_from+frame)
          .tagFrame(innerTag ? frame-innerTag->fromFrame():
                               start_from+frame);

        std::string frame_fn =
          filename_formatter(fn_format, fnInfo);

        fop->m_seq.filename_list.push_back(frame_fn);
      }
    }
  }
  else
    fop->m_filename = filename;

  // Configure output format?
  if (fop->m_format->support(FILE_SUPPORT_GET_FORMAT_OPTIONS)) {
    base::SharedPtr<FormatOptions> format_options =
      fop->m_format->getFormatOptions(fop);

    // Does the user cancelled the operation?
    if (!format_options)
      return nullptr;

    fop->m_seq.format_options = format_options;
    fop->m_document->setFormatOptions(format_options);
  }

  return fop.release();
}

// Executes the file operation: loads or saves the sprite.
//
// It can be called from a different thread of the one used
// by FileOp::createLoadDocumentOperation() or createSaveDocumentOperation().
//
// After this function you must to mark the FileOp as "done" calling
// FileOp::done() function.
void FileOp::operate(IFileOpProgress* progress)
{
  ASSERT(!isDone());

  m_progressInterface = progress;

  // Load //////////////////////////////////////////////////////////////////////
  if (m_type == FileOpLoad &&
      m_format != NULL &&
      m_format->support(FILE_SUPPORT_LOAD)) {
    // Load a sequence
    if (isSequence()) {
      // Default palette
      m_seq.palette->makeBlack();

      // Load the sequence
      frame_t frames(m_seq.filename_list.size());
      frame_t frame(0);
      Image* old_image = nullptr;

      // TODO set_palette for each frame???
      auto add_image = [&]() {
        m_seq.last_cel->data()->setImage(m_seq.image);
        m_seq.layer->addCel(m_seq.last_cel);

        if (m_document->sprite()->palette(frame)
            ->countDiff(m_seq.palette, NULL, NULL) > 0) {
          m_seq.palette->setFrame(frame);
          m_document->sprite()->setPalette(m_seq.palette, true);
        }

        old_image = m_seq.image.get();
        m_seq.image.reset(NULL);
        m_seq.last_cel = NULL;
      };

      m_seq.has_alpha = false;
      m_seq.progress_offset = 0.0f;
      m_seq.progress_fraction = 1.0f / (double)frames;

      auto it = m_seq.filename_list.begin(),
           end = m_seq.filename_list.end();
      for (; it != end; ++it) {
        m_filename = it->c_str();

        // Call the "load" procedure to read the first bitmap.
        bool loadres = m_format->load(this);
        if (!loadres) {
          setError("Error loading frame %d from file \"%s\"\n",
                   frame+1, m_filename.c_str());
        }

        // For the first frame...
        if (!old_image) {
          // Error reading the first frame
          if (!loadres || !m_document || !m_seq.last_cel) {
            m_seq.image.reset();
            delete m_seq.last_cel;
            delete m_document;
            m_document = nullptr;
            break;
          }
          // Read ok
          else {
            // Add the keyframe
            add_image();
          }
        }
        // For other frames
        else {
          // All done (or maybe not enough memory)
          if (!loadres || !m_seq.last_cel) {
            m_seq.image.reset();
            delete m_seq.last_cel;
            break;
          }

          // Compare the old frame with the new one
#if USE_LINK // TODO this should be configurable through a check-box
          if (count_diff_between_images(old_image, m_seq.image)) {
            add_image();
          }
          // We don't need this image
          else {
            delete m_seq.image;

            // But add a link frame
            m_seq.last_cel->image = image_index;
            layer_add_frame(m_seq.layer, m_seq.last_cel);

            m_seq.last_image = NULL;
            m_seq.last_cel = NULL;
          }
#else
          add_image();
#endif
        }

        ++frame;
        m_seq.progress_offset += m_seq.progress_fraction;
      }
      m_filename = *m_seq.filename_list.begin();

      // Final setup
      if (m_document != NULL) {
        // Configure the layer as the 'Background'
        if (!m_seq.has_alpha)
          m_seq.layer->configureAsBackground();

        // Set the frames range
        m_document->sprite()->setTotalFrames(frame);

        // Sets special options from the specific format (e.g. BMP
        // file can contain the number of bits per pixel).
        m_document->setFormatOptions(m_seq.format_options);
      }
    }
    // Direct load from one file.
    else {
      // Call the "load" procedure.
      if (!m_format->load(this))
        setError("Error loading sprite from file \"%s\"\n",
                 m_filename.c_str());
    }
  }
  // Save //////////////////////////////////////////////////////////////////////
  else if (m_type == FileOpSave &&
           m_format != NULL &&
           m_format->support(FILE_SUPPORT_SAVE)) {
#ifdef ENABLE_SAVE
    // Save a sequence
    if (isSequence()) {
      ASSERT(m_format->support(FILE_SUPPORT_SEQUENCES));

      Sprite* sprite = m_document->sprite();

      // Create a temporary bitmap
      m_seq.image.reset(Image::create(sprite->pixelFormat(),
          sprite->width(),
          sprite->height()));

      m_seq.progress_offset = 0.0f;
      m_seq.progress_fraction = 1.0f / (double)sprite->totalFrames();

      // For each frame in the sprite.
      render::Render render;
      for (frame_t frame(0); frame < sprite->totalFrames(); ++frame) {
        // Draw the "frame" in "m_seq.image"
        render.renderSprite(m_seq.image.get(), sprite, frame);

        // Setup the palette.
        sprite->palette(frame)->copyColorsTo(m_seq.palette);

        // Setup the filename to be used.
        m_filename = m_seq.filename_list[frame];

        // Call the "save" procedure... did it fail?
        if (!m_format->save(this)) {
          setError("Error saving frame %d in the file \"%s\"\n",
                   frame+1, m_filename.c_str());
          break;
        }

        m_seq.progress_offset += m_seq.progress_fraction;
      }
      m_filename = *m_seq.filename_list.begin();

      // Destroy the image
      m_seq.image.reset(NULL);
    }
    // Direct save to a file.
    else {
      // Call the "save" procedure.
      if (!m_format->save(this))
        setError("Error saving the sprite in the file \"%s\"\n",
                 m_filename.c_str());
    }
#else
    setError(
      "Save operation is not supported in trial version.\n"
      "Go to " WEBSITE_DOWNLOAD " and get the full-version.");
#endif
  }

  // Progress = 100%
  setProgress(1.0f);
}

// After mark the 'fop' as 'done' you must to free it calling fop_free().
void FileOp::done()
{
  // Finally done.
  scoped_lock lock(m_mutex);
  m_done = true;
}

void FileOp::stop()
{
  scoped_lock lock(m_mutex);
  if (!m_done)
    m_stop = true;
}

FileOp::~FileOp()
{
  if (m_format)
    m_format->destroyData(this);

  delete m_seq.palette;
}

void FileOp::createDocument(Sprite* spr)
{
  // spr can be NULL if the sprite is set in onPostLoad() then

  ASSERT(m_document == NULL);
  m_document = new Document(spr);
}

void FileOp::postLoad()
{
  if (m_document == NULL)
    return;

  // Set the filename.
  if (isSequence())
    m_document->setFilename(m_seq.filename_list.begin()->c_str());
  else
    m_document->setFilename(m_filename.c_str());

  bool result = m_format->postLoad(this);
  if (!result) {
    // Destroy the document
    delete m_document;
    m_document = nullptr;
    return;
  }

  Sprite* sprite = m_document->sprite();
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

  m_document->markAsSaved();
}

base::SharedPtr<FormatOptions> FileOp::sequenceGetFormatOptions() const
{
  return m_seq.format_options;
}

void FileOp::sequenceSetFormatOptions(const base::SharedPtr<FormatOptions>& format_options)
{
  ASSERT(!m_seq.format_options);
  m_seq.format_options = format_options;
}

void FileOp::sequenceSetNColors(int ncolors)
{
  m_seq.palette->resize(ncolors);
}

int FileOp::sequenceGetNColors() const
{
  return m_seq.palette->size();
}

void FileOp::sequenceSetColor(int index, int r, int g, int b)
{
  m_seq.palette->setEntry(index, rgba(r, g, b, 255));
}

void FileOp::sequenceGetColor(int index, int* r, int* g, int* b) const
{
  uint32_t c;

  ASSERT(index >= 0);
  if (index >= 0 && index < m_seq.palette->size())
    c = m_seq.palette->getEntry(index);
  else
    c = rgba(0, 0, 0, 255);     // Black color

  *r = rgba_getr(c);
  *g = rgba_getg(c);
  *b = rgba_getb(c);
}

void FileOp::sequenceSetAlpha(int index, int a)
{
  int c = m_seq.palette->getEntry(index);
  int r = rgba_getr(c);
  int g = rgba_getg(c);
  int b = rgba_getb(c);

  m_seq.palette->setEntry(index, rgba(r, g, b, a));
}

void FileOp::sequenceGetAlpha(int index, int* a) const
{
  ASSERT(index >= 0);
  if (index >= 0 && index < m_seq.palette->size())
    *a = rgba_geta(m_seq.palette->getEntry(index));
  else
    *a = 0;
}

Image* FileOp::sequenceImage(PixelFormat pixelFormat, int w, int h)
{
  Sprite* sprite;

  // Create the image
  if (!m_document) {
    sprite = new Sprite(pixelFormat, w, h, 256);
    try {
      LayerImage* layer = new LayerImage(sprite);

      // Add the layer
      sprite->folder()->addLayer(layer);

      // Done
      createDocument(sprite);
      m_seq.layer = layer;
    }
    catch (...) {
      delete sprite;
      throw;
    }
  }
  else {
    sprite = m_document->sprite();

    if (sprite->pixelFormat() != pixelFormat)
      return nullptr;
  }

  if (m_seq.last_cel) {
    setError("Error: called two times \"fop_sequence_image()\".\n");
    return nullptr;
  }

  // Create a bitmap
  m_seq.image.reset(Image::create(pixelFormat, w, h));
  m_seq.last_cel = new Cel(m_seq.frame++, ImageRef(nullptr));

  return m_seq.image.get();
}

void FileOp::setError(const char *format, ...)
{
  char buf_error[4096];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf_error, sizeof(buf_error), format, ap);
  va_end(ap);

  // Concatenate the new error
  {
    scoped_lock lock(m_mutex);
    m_error += buf_error;
  }
}

void FileOp::setProgress(double progress)
{
  scoped_lock lock(m_mutex);

  if (isSequence()) {
    m_progress =
      m_seq.progress_offset +
      m_seq.progress_fraction*progress;
  }
  else {
    m_progress = progress;
  }

  if (m_progressInterface)
    m_progressInterface->ackFileOpProgress(progress);
}

double FileOp::progress() const
{
  double progress;
  {
    scoped_lock lock(m_mutex);
    progress = m_progress;
  }
  return progress;
}

// Returns true when the file operation has finished, this means, when
// the FileOp::operate() routine ends.
bool FileOp::isDone() const
{
  bool done;
  {
    scoped_lock lock(m_mutex);
    done = m_done;
  }
  return done;
}

bool FileOp::isStop() const
{
  bool stop;
  {
    scoped_lock lock(m_mutex);
    stop = m_stop;
  }
  return stop;
}

FileOp::FileOp(FileOpType type, Context* context)
  : m_type(type)
  , m_format(nullptr)
  , m_context(context)
  , m_document(nullptr)
  , m_progress(0.0)
  , m_progressInterface(nullptr)
  , m_done(false)
  , m_stop(false)
  , m_oneframe(false)
{
  m_seq.palette = nullptr;
  m_seq.image.reset(nullptr);
  m_seq.progress_offset = 0.0f;
  m_seq.progress_fraction = 0.0f;
  m_seq.frame = frame_t(0);
  m_seq.layer = nullptr;
  m_seq.last_cel = nullptr;
}

void FileOp::prepareForSequence()
{
  m_seq.palette = new Palette(frame_t(0), 256);
  m_seq.format_options.reset();
}

} // namespace app
