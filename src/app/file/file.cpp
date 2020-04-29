// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file.h"

#include "app/cmd/convert_color_profile.h"
#include "app/color_spaces.h"
#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file_data.h"
#include "app/file/file_format.h"
#include "app/file/file_formats_manager.h"
#include "app/file/format_options.h"
#include "app/file/split_filename.h"
#include "app/filename_formatter.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/optional_alert.h"
#include "app/ui/status_bar.h"
#include "base/fs.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/string.h"
#include "dio/detect_format.h"
#include "doc/doc.h"
#include "fmt/format.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/alert.h"
#include "ui/listitem.h"
#include "ui/system.h"
#include "ver/info.h"

#include "ask_for_color_profile.xml.h"
#include "open_sequence.xml.h"

#include <cstring>
#include <cstdarg>

namespace app {

using namespace base;

base::paths get_readable_extensions()
{
  base::paths paths;
  for (const FileFormat* format : *FileFormatsManager::instance()) {
    if (format->support(FILE_SUPPORT_LOAD))
      format->getExtensions(paths);
  }
  return paths;
}

base::paths get_writable_extensions()
{
  base::paths paths;
  for (const FileFormat* format : *FileFormatsManager::instance()) {
    if (format->support(FILE_SUPPORT_SAVE))
      format->getExtensions(paths);
  }
  return paths;
}

Doc* load_document(Context* context, const std::string& filename)
{
  /* TODO add a option to configure what to do with the sequence */
  std::unique_ptr<FileOp> fop(
    FileOp::createLoadDocumentOperation(
      context, filename,
      FILE_LOAD_CREATE_PALETTE |
      FILE_LOAD_SEQUENCE_NONE));
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

  Doc* document = fop->releaseDocument();
  if (document && context)
    document->setContext(context);

  return document;
}

int save_document(Context* context, Doc* document)
{
  std::unique_ptr<FileOp> fop(
    FileOp::createSaveDocumentOperation(
      context,
      FileOpROI(document, "", "", SelectedFrames(), false),
      document->filename(), "",
      false));
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

bool is_static_image_format(const std::string& filename)
{
  // Get the format through the extension of the filename
  FileFormat* format =
    FileFormatsManager::instance()
    ->getFileFormat(dio::detect_format_by_file_extension(filename));

  return (format && format->support(FILE_SUPPORT_SEQUENCES));
}

FileOpROI::FileOpROI()
  : m_document(nullptr)
  , m_slice(nullptr)
  , m_tag(nullptr)
{
}

FileOpROI::FileOpROI(const Doc* doc,
                     const std::string& sliceName,
                     const std::string& tagName,
                     const doc::SelectedFrames& selFrames,
                     const bool adjustByTag)
  : m_document(doc)
  , m_slice(nullptr)
  , m_tag(nullptr)
  , m_selFrames(selFrames)
{
  if (doc) {
    if (!sliceName.empty())
      m_slice = doc->sprite()->slices().getByName(sliceName);

    // Don't allow exporting frame tags with empty names
    if (!tagName.empty())
      m_tag = doc->sprite()->tags().getByName(tagName);

    if (m_tag) {
      if (m_selFrames.empty())
        m_selFrames.insert(m_tag->fromFrame(), m_tag->toFrame());
      else if (adjustByTag)
        m_selFrames.displace(m_tag->fromFrame());

      m_selFrames =
        m_selFrames.filter(std::max(0, m_tag->fromFrame()),
                           std::min(m_tag->toFrame(), doc->sprite()->lastFrame()));
    }
    // All frames if selected frames is empty
    else if (m_selFrames.empty())
      m_selFrames.insert(0, doc->sprite()->lastFrame());
  }
}

// static
FileOp* FileOp::createLoadDocumentOperation(Context* context,
                                            const std::string& filename,
                                            const int flags,
                                            const FileOpConfig* config)
{
  std::unique_ptr<FileOp> fop(
    new FileOp(FileOpLoad, context, config));
  if (!fop)
    return nullptr;

  LOG("FILE: Loading file \"%s\"\n", filename.c_str());

  // Does file exist?
  if (!base::is_file(filename)) {
    fop->setError("File not found: \"%s\"\n", filename.c_str());
    goto done;
  }

  // Get the format through the extension of the filename
  fop->m_format = FileFormatsManager::instance()->getFileFormat(
    dio::detect_format(filename));
  if (!fop->m_format ||
      !fop->m_format->support(FILE_SUPPORT_LOAD)) {
    fop->setError("%s can't load \"%s\" file (\"%s\")\n", get_app_name(),
                  filename.c_str(), base::get_file_extension(filename).c_str());
    goto done;
  }

  // Use the "sequence" interface
  if (fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
    fop->prepareForSequence();
    fop->m_seq.flags = flags;

    // At the moment we want load just one file (the one specified in filename)
    fop->m_seq.filename_list.push_back(filename);

    // If the user wants to load the whole sequence
    if (!(flags & FILE_LOAD_SEQUENCE_NONE)) {
      std::string left, right;
      int c, width, start_from;
      char buf[512];

      // First of all, we must generate the list of files to load in the
      // sequence...

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

#ifdef ENABLE_UI
      // TODO add a better dialog to edit file-names
      if ((flags & FILE_LOAD_SEQUENCE_ASK) &&
          context &&
          context->isUIAvailable() &&
          fop->m_seq.filename_list.size() > 1) {
        app::gen::OpenSequence window;
        window.repeat()->setVisible(flags & FILE_LOAD_SEQUENCE_ASK_CHECKBOX ? true: false);

        for (const auto& fn : fop->m_seq.filename_list) {
          auto item = new ui::ListItem(base::get_file_name(fn));
          item->setSelected(true);
          window.files()->addChild(item);
        }

        window.files()->Change.connect(
          [&window]{
            window.agree()->setEnabled(
              window.files()->getSelectedChild() != nullptr);
          });

        window.openWindowInForeground();

        // If the user selected the "do the same for other files"
        // checkbox, we've to save what the user want to do for the
        // following files.
        if (window.repeat()->isSelected()) {
          if (window.closer() == window.agree())
            fop->m_seq.flags = FILE_LOAD_SEQUENCE_YES;
          else
            fop->m_seq.flags = FILE_LOAD_SEQUENCE_NONE;
        }

        if (window.closer() == window.agree()) {
          // If the user replies "Agree", we load the selected files.
          base::paths list;

          auto it = window.files()->children().begin();
          auto end = window.files()->children().end();
          for (const auto& fn : fop->m_seq.filename_list) {
            ASSERT(it != end);
            if (it == end)
              break;
            if ((*it)->isSelected())
              list.push_back(fn);
            ++it;
          }

          ASSERT(!list.empty());
          fop->m_seq.filename_list = list;
        }
        else {
          // If the user replies "Skip", we need just one file name
          // (the first one).
          if (fop->m_seq.filename_list.size() > 1) {
            fop->m_seq.filename_list.erase(fop->m_seq.filename_list.begin()+1,
                                           fop->m_seq.filename_list.end());
          }
        }
      }
#endif // ENABLE_UI
    }
  }
  else {
    fop->m_filename = filename;
  }

  // Load just one frame
  if (flags & FILE_LOAD_ONE_FRAME)
    fop->m_oneframe = true;

  if (flags & FILE_LOAD_CREATE_PALETTE)
    fop->m_createPaletteFromRgba = true;

  // Does data file exist?
  if (flags & FILE_LOAD_DATA_FILE) {
    std::string dataFilename = base::replace_extension(filename, "aseprite-data");
    if (base::is_file(dataFilename))
      fop->m_dataFilename = dataFilename;
  }

done:;
  return fop.release();
}

// static
FileOp* FileOp::createSaveDocumentOperation(const Context* context,
                                            const FileOpROI& roi,
                                            const std::string& filename,
                                            const std::string& filenameFormatArg,
                                            const bool ignoreEmptyFrames)
{
  std::unique_ptr<FileOp> fop(
    new FileOp(FileOpSave, const_cast<Context*>(context), nullptr));

  // Document to save
  fop->m_document = const_cast<Doc*>(roi.document());
  fop->m_roi = roi;
  fop->m_ignoreEmpty = ignoreEmptyFrames;

  // Get the extension of the filename (in lower case)
  LOG("FILE: Saving document \"%s\"\n", filename.c_str());

  // Check for read-only attribute
  if (base::has_readonly_attr(filename)) {
    fop->setError("Error saving \"%s\" file, it's read-only",
                  filename.c_str());
    return fop.release();
  }

  // Get the format through the extension of the filename
  fop->m_format = FileFormatsManager::instance()->getFileFormat(
    dio::detect_format_by_file_extension(filename));
  if (!fop->m_format ||
      !fop->m_format->support(FILE_SUPPORT_SAVE)) {
    fop->setError("%s can't save \"%s\" file (\"%s\")\n", get_app_name(),
                  filename.c_str(), base::get_file_extension(filename).c_str());
    return fop.release();
  }

  // Warnings
  std::string warnings;
  bool fatal = false;

  // Check image type support
  // TODO add support to automatically convert the image to a supported format
  switch (fop->m_document->sprite()->pixelFormat()) {

    case IMAGE_RGB:
      if (!(fop->m_format->support(FILE_SUPPORT_RGB))) {
        warnings += "<<- " + Strings::alerts_file_format_rgb_mode();
        fatal = true;
      }

      if (!(fop->m_format->support(FILE_SUPPORT_RGBA)) &&
          fop->m_document->sprite()->needAlpha()) {

        warnings += "<<- " + Strings::alerts_file_format_alpha_channel();
      }
      break;

    case IMAGE_GRAYSCALE:
      if (!(fop->m_format->support(FILE_SUPPORT_GRAY))) {
        warnings += "<<- " + Strings::alerts_file_format_grayscale_mode();
        fatal = true;
      }
      if (!(fop->m_format->support(FILE_SUPPORT_GRAYA)) &&
          fop->m_document->sprite()->needAlpha()) {

        warnings += "<<- " + Strings::alerts_file_format_alpha_channel();
      }
      break;

    case IMAGE_INDEXED:
      if (!(fop->m_format->support(FILE_SUPPORT_INDEXED))) {
        warnings += "<<- " + Strings::alerts_file_format_indexed_mode();
        fatal = true;
      }
      break;
  }

  // Frames support
  if (fop->m_roi.frames() > 1) {
    if (!fop->m_format->support(FILE_SUPPORT_FRAMES) &&
        !fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
      warnings += "<<- " + Strings::alerts_file_format_frames();
    }
  }

  // Layers support
  if (fop->m_document->sprite()->root()->layersCount() > 1) {
    if (!(fop->m_format->support(FILE_SUPPORT_LAYERS))) {
      warnings += "<<- " + Strings::alerts_file_format_layers();
    }
  }

  // Palettes support
  if (fop->m_document->sprite()->getPalettes().size() > 1) {
    if (!fop->m_format->support(FILE_SUPPORT_PALETTES) &&
        !fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
      warnings += "<<- " + Strings::alerts_file_format_palette_changes();
    }
  }

  // Check frames support
  if (!fop->m_document->sprite()->tags().empty()) {
    if (!fop->m_format->support(FILE_SUPPORT_TAGS)) {
      warnings += "<<- " + Strings::alerts_file_format_tags();
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
    if (!fop->m_format->support(FILE_SUPPORT_RGBA) ||
        !fop->m_format->support(FILE_SUPPORT_INDEXED) ||
        fop->document()->colorMode() == ColorMode::INDEXED) {
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
  }

  // Show the confirmation alert
  if (!warnings.empty()) {
#ifdef ENABLE_UI
    // Interative
    if (context && context->isUIAvailable()) {
      int ret = OptionalAlert::show(
        Preferences::instance().saveFile.showFileFormatDoesntSupportAlert,
        1, // Yes is the default option when the alert dialog is disabled
        fmt::format(
          (fatal ? Strings::alerts_file_format_doesnt_support_error():
                   Strings::alerts_file_format_doesnt_support_warning()),
          fop->m_format->name(),
          warnings));

      // Operation can't be done (by fatal error) or the user cancel
      // the operation
      if ((fatal) || (ret != 1))
        return nullptr;
    }
    // No interactive & fatal error?
    else
#endif // ENABLE_UI
    if (fatal) {
      fop->setError(warnings.c_str());
      return fop.release();
    }
  }

  // Use the "sequence" interface.
  if (fop->m_format->support(FILE_SUPPORT_SEQUENCES)) {
    fop->prepareForSequence();

    std::string fn = filename;
    std::string fn_format = filenameFormatArg;
    if (fn_format.empty()) {
      fn_format = get_default_filename_format(
        fn,
        true,                       // With path
        (fop->m_roi.frames() > 1),  // Has frames
        false,                      // Doesn't have layers
        false);                     // Doesn't have tags
    }

    Sprite* spr = fop->m_document->sprite();
    frame_t outputFrame = 0;

    for (frame_t frame : fop->m_roi.selectedFrames()) {
      Tag* innerTag = (fop->m_roi.tag() ? fop->m_roi.tag(): spr->tags().innerTag(frame));
      Tag* outerTag = (fop->m_roi.tag() ? fop->m_roi.tag(): spr->tags().outerTag(frame));
      FilenameInfo fnInfo;
      fnInfo
        .filename(fn)
        .sliceName(fop->m_roi.slice() ? fop->m_roi.slice()->name(): "")
        .innerTagName(innerTag ? innerTag->name(): "")
        .outerTagName(outerTag ? outerTag->name(): "")
        .frame(outputFrame)
        .tagFrame(innerTag ? frame - innerTag->fromFrame():
                             outputFrame);

      fop->m_seq.filename_list.push_back(
        filename_formatter(fn_format, fnInfo));

      ++outputFrame;
    }

#ifdef ENABLE_UI
    if (context && context->isUIAvailable() &&
        fop->m_seq.filename_list.size() > 1 &&
        OptionalAlert::show(
          Preferences::instance().saveFile.showExportAnimationInSequenceAlert,
          1,
          fmt::format(
            Strings::alerts_export_animation_in_sequence(),
            int(fop->m_seq.filename_list.size()),
            base::get_file_name(fop->m_seq.filename_list[0]),
            base::get_file_name(fop->m_seq.filename_list[1]))) != 1) {
      return nullptr;
    }
#endif // ENABLE_UI
  }
  else
    fop->m_filename = filename;

  // Configure output format?
  if (fop->m_format->support(FILE_SUPPORT_GET_FORMAT_OPTIONS)) {
    auto opts = fop->m_format->askUserForFormatOptions(fop.get());

    // Does the user cancelled the operation?
    if (!opts)
      return nullptr;

    fop->m_formatOptions = opts;
    fop->m_document->setFormatOptions(opts);
  }

  // Does data file exist?
  std::string dataFilename = base::replace_extension(filename, "aseprite-data");
  if (base::is_file(dataFilename))
    fop->m_dataFilename = dataFilename;

  return fop.release();
}

// Executes the file operation: loads or saves the sprite.
//
// It can be called from a different thread of the one used
// by FileOp::createLoadDocumentOperation() or createSaveDocumentOperation().
//
// After this function you must to mark the FileOp as "done" calling
// FileOp::done() function.
//
// TODO refactor this code
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
      gfx::Size canvasSize(0, 0);

      // TODO setPalette for each frame???
      auto add_image = [&]() {
        canvasSize |= m_seq.image->size();

        m_seq.last_cel->data()->setImage(m_seq.image);
        m_seq.layer->addCel(m_seq.last_cel);

        if (m_document->sprite()->palette(frame)
            ->countDiff(m_seq.palette, NULL, NULL) > 0) {
          m_seq.palette->setFrame(frame);
          m_document->sprite()->setPalette(m_seq.palette, true);
        }

        old_image = m_seq.image.get();
        m_seq.image.reset();
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
      if (m_document) {
        // Configure the layer as the 'Background'
        if (!m_seq.has_alpha)
          m_seq.layer->configureAsBackground();

        // Set the final canvas size (as the bigger loaded
        // frame/image).
        m_document->sprite()->setSize(canvasSize.w,
                                      canvasSize.h);

        // Set the frames range
        m_document->sprite()->setTotalFrames(frame);

        // Sets special options from the specific format (e.g. BMP
        // file can contain the number of bits per pixel).
        m_document->setFormatOptions(m_formatOptions);
      }
    }
    // Direct load from one file.
    else {
      // Call the "load" procedure.
      if (!m_format->load(this)) {
        setError("Error loading sprite from file \"%s\"\n",
                 m_filename.c_str());
      }
    }

    // Load special data from .aseprite-data file
    if (m_document &&
        m_document->sprite()  &&
        !m_dataFilename.empty()) {
      try {
        load_aseprite_data_file(m_dataFilename,
                                m_document,
                                m_config.defaultSliceColor);
      }
      catch (const std::exception& ex) {
        setError("Error loading data file: %s\n", ex.what());
      }
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
      render.setNewBlend(m_config.newBlend);

      frame_t outputFrame = 0;
      for (frame_t frame : m_roi.selectedFrames()) {
        // Draw the "frame" in "m_seq.image"
        if (m_roi.slice()) {
          const SliceKey* key = m_roi.slice()->getByFrame(frame);
          if (!key || key->isEmpty())
            continue;           // Skip frame because there is no slice key

          m_seq.image.reset(
            Image::create(sprite->pixelFormat(),
                          key->bounds().w,
                          key->bounds().h));

          render.renderSprite(
            m_seq.image.get(), sprite, frame,
            gfx::Clip(gfx::Point(0, 0), key->bounds()));
        }
        else {
          render.renderSprite(m_seq.image.get(), sprite, frame);
        }

        bool save = true;

        // Check if we have to ignore empty frames
        if (m_ignoreEmpty &&
            !sprite->isOpaque() &&
            doc::is_empty_image(m_seq.image.get())) {
          save = false;
        }

        if (save) {
          // Setup the palette.
          sprite->palette(frame)->copyColorsTo(m_seq.palette);

          // Setup the filename to be used.
          m_filename = m_seq.filename_list[outputFrame];

          // Make directories
          {
            std::string dir = base::get_file_path(m_filename);
            try {
              if (!base::is_directory(dir))
                base::make_all_directories(dir);
            }
            catch (const std::exception& ex) {
              // Ignore errors and make the delegate fail
              setError("Error creating directory \"%s\"\n%s",
                       dir.c_str(), ex.what());
            }
          }

          // Call the "save" procedure... did it fail?
          if (!m_format->save(this)) {
            setError("Error saving frame %d in the file \"%s\"\n",
                     outputFrame+1, m_filename.c_str());
            break;
          }
        }

        m_seq.progress_offset += m_seq.progress_fraction;
        ++outputFrame;
      }

      m_filename = *m_seq.filename_list.begin();

      // Destroy the image
      m_seq.image.reset();
    }
    // Direct save to a file.
    else {
      // Call the "save" procedure.
      if (!m_format->save(this)) {
        setError("Error saving the sprite in the file \"%s\"\n",
                 m_filename.c_str());
      }
    }

    // Save special data from .aseprite-data file
    if (m_document &&
        m_document->sprite() &&
        !hasError() &&
        !m_dataFilename.empty()) {
      try {
        save_aseprite_data_file(m_dataFilename, m_document);
      }
      catch (const std::exception& ex) {
        setError("Error loading data file: %s\n", ex.what());
      }
    }
#else
    setError(
      fmt::format("Save operation is not supported in trial version.\n"
                  "Go to {} and get the full-version.",
                  get_app_download_url()).c_str());
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
  m_document = new Doc(spr);
}

void FileOp::postLoad()
{
  if (m_document == NULL)
    return;

  // Set the filename.
  std::string fn;
  if (isSequence())
    fn = m_seq.filename_list.begin()->c_str();
  else
    fn = m_filename.c_str();
  m_document->setFilename(fn);

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
    if (m_createPaletteFromRgba &&
        sprite->pixelFormat() == IMAGE_RGB &&
        sprite->getPalettes().size() <= 1 &&
        sprite->palette(frame_t(0))->isBlack()) {
      std::shared_ptr<Palette> palette(
        render::create_palette_from_sprite(
          sprite, frame_t(0), sprite->lastFrame(), true,
          nullptr, nullptr, m_config.newBlend));

      sprite->resetPalettes();
      sprite->setPalette(palette.get(), false);
    }
  }

  // What to do with the sprite color profile?
  gfx::ColorSpacePtr spriteCS = sprite->colorSpace();
  app::gen::ColorProfileBehavior behavior =
    app::gen::ColorProfileBehavior::DISABLE;

  if (m_config.preserveColorProfile) {
    // Embedded color profile
    if (this->hasEmbeddedColorProfile()) {
      behavior = m_config.filesWithProfile;
      if (behavior == app::gen::ColorProfileBehavior::ASK) {
#ifdef ENABLE_UI
        if (m_context && m_context->isUIAvailable()) {
          app::gen::AskForColorProfile window;
          window.spriteWithoutProfile()->setVisible(false);
          window.openWindowInForeground();
          auto c = window.closer();
          if (c == window.embedded())
            behavior = app::gen::ColorProfileBehavior::EMBEDDED;
          else if (c == window.convert())
            behavior = app::gen::ColorProfileBehavior::CONVERT;
          else if (c == window.assign())
            behavior = app::gen::ColorProfileBehavior::ASSIGN;
          else
            behavior = app::gen::ColorProfileBehavior::DISABLE;
        }
        else
#endif // ENABLE_UI
        {
          behavior = app::gen::ColorProfileBehavior::EMBEDDED;
        }
      }
    }
    // Missing color space
    else {
      behavior = m_config.missingProfile;
      if (behavior == app::gen::ColorProfileBehavior::ASK) {
#ifdef ENABLE_UI
        if (m_context && m_context->isUIAvailable()) {
          app::gen::AskForColorProfile window;
          window.spriteWithProfile()->setVisible(false);
          window.embedded()->setVisible(false);
          window.convert()->setVisible(false);
          window.openWindowInForeground();
          if (window.closer() == window.assign()) {
            behavior = app::gen::ColorProfileBehavior::ASSIGN;
          }
          else {
            behavior = app::gen::ColorProfileBehavior::DISABLE;
          }
        }
        else
#endif // ENABLE_UI
        {
          behavior = app::gen::ColorProfileBehavior::ASSIGN;
        }
      }
    }
  }

  switch (behavior) {

    case app::gen::ColorProfileBehavior::DISABLE:
      sprite->setColorSpace(gfx::ColorSpace::MakeNone());
      m_document->notifyColorSpaceChanged();
      break;

    case app::gen::ColorProfileBehavior::EMBEDDED:
      // Do nothing, just keep the current sprite's color sprite
      break;

    case app::gen::ColorProfileBehavior::CONVERT: {
      // Convert to the working color profile
      auto gfxCS = m_config.workingCS;
      if (!gfxCS->nearlyEqual(*spriteCS))
        cmd::convert_color_profile(sprite, gfxCS);
      break;
    }

    case app::gen::ColorProfileBehavior::ASSIGN: {
      // Convert to the working color profile
      auto gfxCS = m_config.workingCS;
      sprite->setColorSpace(gfxCS);
      m_document->notifyColorSpaceChanged();
      break;
    }
  }

  m_document->markAsSaved();
}

void FileOp::setLoadedFormatOptions(const FormatOptionsPtr& opts)
{
  // This assert can fail when we load a sequence of files.
  // TODO what we should do, keep the first or the latest format options?
  //ASSERT(!m_formatOptions);
  m_formatOptions = opts;
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
    sprite = new Sprite(ImageSpec((ColorMode)pixelFormat, w, h), 256);
    try {
      LayerImage* layer = new LayerImage(sprite);

      // Add the layer
      sprite->root()->addLayer(layer);

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
    setError("Error: called two times FileOp::sequenceImage()\n");
    return nullptr;
  }

  // Create a bitmap
  m_seq.image.reset(Image::create(pixelFormat, w, h));
  m_seq.last_cel = new Cel(m_seq.frame++, ImageRef(nullptr));

  return m_seq.image.get();
}

void FileOp::setError(const char *format, ...)
{
  char buf_error[4096];         // TODO possible stack overflow
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf_error, sizeof(buf_error), format, ap);
  va_end(ap);

  // Concatenate the new error
  {
    scoped_lock lock(m_mutex);
    // Add a newline char automatically if it's needed
    if (!m_error.empty() && m_error.back() != '\n')
      m_error.push_back('\n');
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

void FileOp::getFilenameList(base::paths& output) const
{
  if (isSequence()) {
    output = m_seq.filename_list;
  }
  else {
    output.push_back(m_filename);
  }
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

FileOp::FileOp(FileOpType type,
               Context* context,
               const FileOpConfig* config)
  : m_type(type)
  , m_format(nullptr)
  , m_context(context)
  , m_document(nullptr)
  , m_progress(0.0)
  , m_progressInterface(nullptr)
  , m_done(false)
  , m_stop(false)
  , m_oneframe(false)
  , m_createPaletteFromRgba(false)
  , m_ignoreEmpty(false)
  , m_embeddedColorProfile(false)
  , m_embeddedGridBounds(false)
{
  if (config)
    m_config = *config;
  else if (ui::is_ui_thread())
    m_config.fillFromPreferences();
  else {
    LOG(VERBOSE, "FILE: Using a file operation with default configuration\n");
  }

  m_seq.palette = nullptr;
  m_seq.image.reset();
  m_seq.progress_offset = 0.0f;
  m_seq.progress_fraction = 0.0f;
  m_seq.frame = frame_t(0);
  m_seq.layer = nullptr;
  m_seq.last_cel = nullptr;
  m_seq.flags = 0;
}

void FileOp::prepareForSequence()
{
  m_seq.palette = new Palette(frame_t(0), 256);
  m_formatOptions.reset();
}

} // namespace app
