// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_FILE_H_INCLUDED
#define APP_FILE_FILE_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/doc.h"
#include "app/file/file_op_config.h"
#include "app/file/format_options.h"
#include "app/pref/preferences.h"
#include "base/paths.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/pixel_format.h"
#include "doc/selected_frames.h"
#include "os/color_space.h"

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>

// Flags for FileOp::createLoadDocumentOperation()
#define FILE_LOAD_SEQUENCE_NONE         0x00000001
#define FILE_LOAD_SEQUENCE_ASK          0x00000002
#define FILE_LOAD_SEQUENCE_ASK_CHECKBOX 0x00000004
#define FILE_LOAD_SEQUENCE_YES          0x00000008
#define FILE_LOAD_ONE_FRAME             0x00000010
#define FILE_LOAD_DATA_FILE             0x00000020
#define FILE_LOAD_CREATE_PALETTE        0x00000040

namespace doc {
  class Tag;
}

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class LayerImage;
  class Palette;
  class Slice;
  class Sprite;
}

namespace app {

  class Context;
  class FileFormat;

  using namespace doc;

  // File operations.
  typedef enum {
    FileOpLoad,
    FileOpSave
  } FileOpType;

  class IFileOpProgress {
  public:
    virtual ~IFileOpProgress() { }
    virtual void ackFileOpProgress(double progress) = 0;
  };

  class FileOpROI {             // Region of interest
  public:
    FileOpROI();
    FileOpROI(const Doc* doc,
              const gfx::Rect& bounds,
              const std::string& sliceName,
              const std::string& tagName,
              const doc::SelectedFrames& selFrames,
              const bool adjustByTag);

    const Doc* document() const { return m_document; }
    const gfx::Rect& bounds() const { return m_bounds; }
    doc::Slice* slice() const { return m_slice; }
    doc::Tag* tag() const { return m_tag; }
    doc::frame_t fromFrame() const { return m_selFrames.firstFrame(); }
    doc::frame_t toFrame() const { return m_selFrames.lastFrame(); }
    const doc::SelectedFrames& selectedFrames() const { return m_selFrames; }

    doc::frame_t frames() const {
      return (doc::frame_t)m_selFrames.size();
    }

  private:
    const Doc* m_document;
    gfx::Rect m_bounds;
    doc::Slice* m_slice;
    doc::Tag* m_tag;
    doc::SelectedFrames m_selFrames;
  };

  // Used by file formats with FILE_ENCODE_ABSTRACT_IMAGE flag, to
  // encode a sprite with an intermediate transformation on-the-fly
  // (e.g. resizing).
  class FileAbstractImage {
  public:
    virtual ~FileAbstractImage() { }

    virtual int width() const { return spec().width(); }
    virtual int height() const { return spec().height(); }

    virtual const doc::ImageSpec& spec() const = 0;
    virtual os::ColorSpaceRef osColorSpace() const = 0;
    virtual bool needAlpha() const = 0;
    virtual bool isOpaque() const = 0;
    virtual int frames() const = 0;
    virtual int frameDuration(doc::frame_t frame) const = 0;

    virtual const doc::Palette* palette(doc::frame_t frame) const = 0;
    virtual doc::PalettesList palettes() const = 0;

    virtual const doc::ImageRef getScaledImage() const = 0;

    // In case the file format can encode scanline by scanline
    // (e.g. PNG format).
    virtual const uint8_t* getScanline(int y) const = 0;

    // In case that the encoder needs full frame renders (or compare
    // between frames), e.g. GIF format.
    virtual void renderFrame(const doc::frame_t frame, doc::Image* dst) const = 0;
  };

  // Structure to load & save files.
  //
  // TODO This class do to many things. There should be a previous
  // instance (class) to verify what the user want to do with the
  // sequence of files, and the result of that operation should be the
  // input of this one.
  class FileOp {
  public:
    static FileOp* createLoadDocumentOperation(Context* context,
                                               const std::string& filename,
                                               const int flags,
                                               const FileOpConfig* config = nullptr);

    static FileOp* createSaveDocumentOperation(const Context* context,
                                               const FileOpROI& roi,
                                               const std::string& filename,
                                               const std::string& filenameFormat,
                                               const bool ignoreEmptyFrames);

    static bool checkIfFormatSupportResizeOnTheFly(const std::string& filename);

    ~FileOp();

    bool isSequence() const { return !m_seq.filename_list.empty(); }
    bool isOneFrame() const { return m_oneframe; }
    bool preserveColorProfile() const { return m_config.preserveColorProfile; }

    const std::string& filename() const { return m_filename; }
    const base::paths& filenames() const { return m_seq.filename_list; }
    Context* context() const { return m_context; }
    Doc* document() const { return m_document; }
    Doc* releaseDocument() {
      Doc* doc = m_document;
      m_document = nullptr;
      return doc;
    }

    const FileOpROI& roi() const { return m_roi; }

    // Creates a new document with the given sprite.
    void createDocument(Sprite* spr);
    void operate(IFileOpProgress* progress = nullptr);

    void done();
    void stop();
    bool isDone() const;
    bool isStop() const;

    // Does extra post-load processing which may require user intervention.
    void postLoad();

    // Special options specific to the file format.
    FormatOptionsPtr formatOptions() const {
      return m_formatOptions;
    }

    // Options to save the document. This function doesn't return
    // nullptr.
    template<typename T>
    std::shared_ptr<T> formatOptionsForSaving() const {
      auto opts = std::dynamic_pointer_cast<T>(m_formatOptions);
      if (!opts)
        opts = std::make_shared<T>();
      ASSERT(opts);
      return opts;
    }

    bool hasFormatOptionsOfDocument() const {
      return (m_document->formatOptions() != nullptr);
    }

    // Options from the document when it was loaded. This function
    // doesn't return nullptr.
    template<typename T>
    std::shared_ptr<T> formatOptionsOfDocument() const {
      // We use the dynamic cast because the document format options
      // could be an instance of another class than T.
      auto opts = std::dynamic_pointer_cast<T>(m_document->formatOptions());
      if (!opts) {
        // If the document doesn't have format options (or the type
        // doesn't matches T), we create default format options for
        // this file.
        opts = std::make_shared<T>();
      }
      ASSERT(opts);
      return opts;
    }

    void setLoadedFormatOptions(const FormatOptionsPtr& opts);

    // Helpers for file decoder/encoder (FileFormat) with
    // FILE_SUPPORT_SEQUENCES flag.
    void sequenceSetNColors(int ncolors);
    int sequenceGetNColors() const;
    void sequenceSetColor(int index, int r, int g, int b);
    void sequenceGetColor(int index, int* r, int* g, int* b) const;
    void sequenceSetAlpha(int index, int a);
    void sequenceGetAlpha(int index, int* a) const;
    ImageRef sequenceImage(PixelFormat pixelFormat, int w, int h);
    const ImageRef sequenceImage() const { return m_seq.image; }
    const Palette* sequenceGetPalette() const { return m_seq.palette; }
    bool sequenceGetHasAlpha() const {
      return m_seq.has_alpha;
    }
    void sequenceSetHasAlpha(bool hasAlpha) {
      m_seq.has_alpha = hasAlpha;
    }
    int sequenceFlags() const {
      return m_seq.flags;
    }

    // Can be used to encode sequences/static files (e.g. png files)
    // or animations (e.g. gif) resizing the result on the fly.
    FileAbstractImage* abstractImage();
    void setOnTheFlyScale(const gfx::PointF& scale);

    const std::string& error() const { return m_error; }
    void setError(const char *error, ...);
    bool hasError() const { return !m_error.empty(); }
    void setIncompatibilityError(const std::string& msg);
    bool hasIncompatibilityError() const { return !m_incompatibilityError.empty(); }

    double progress() const;
    void setProgress(double progress);

    void getFilenameList(base::paths& output) const;

    void setEmbeddedColorProfile() { m_embeddedColorProfile = true; }
    bool hasEmbeddedColorProfile() const { return m_embeddedColorProfile; }

    void setEmbeddedGridBounds() { m_embeddedGridBounds = true; }
    bool hasEmbeddedGridBounds() const { return m_embeddedGridBounds; }

    bool newBlend() const { return m_config.newBlend; }
    const FileOpConfig& config() const { return m_config; }

  private:
    FileOp();                   // Undefined
    FileOp(FileOpType type,
           Context* context,
           const FileOpConfig* config);

    FileOpType m_type;          // Operation type: 0=load, 1=save.
    FileFormat* m_format;
    Context* m_context;
    // TODO this should be a shared pointer (and we should remove
    //      releaseDocument() member function)
    Doc* m_document;            // Loaded document, or document to be saved.
    std::string m_filename;     // File-name to load/save.
    std::string m_dataFilename; // File-name for a special XML .aseprite-data where extra sprite data can be stored
    FileOpROI m_roi;

    // Shared fields between threads.
    mutable std::mutex m_mutex; // Mutex to access to the next two fields.
    double m_progress;          // Progress (1.0 is ready).
    IFileOpProgress* m_progressInterface;
    std::string m_error;        // Error string.
    std::string m_incompatibilityError; // Incompatibility error string.
    bool m_done;                // True if the operation finished.
    bool m_stop;                // Force the break of the operation.
    bool m_oneframe;            // Load just one frame (in formats
                                // that support animation like
                                // GIF/FLI/ASE).
    bool m_createPaletteFromRgba;
    bool m_ignoreEmpty;

    // True if the file contained a color profile when it was loaded.
    bool m_embeddedColorProfile;

    // True if the file contained a the grid bounds inside.
    bool m_embeddedGridBounds;

    FileOpConfig m_config;

    // Options
    FormatOptionsPtr m_formatOptions;

    // Data for sequences.
    struct {
      base::paths filename_list;  // All file names to load/save.
      Palette* palette;           // Palette of the sequence.
      ImageRef image;             // Image to be saved/loaded.
      // For the progress bar.
      double progress_offset;      // Progress offset from the current frame.
      double progress_fraction;    // Progress fraction for one frame.
      // To load sequences.
      frame_t frame;
      bool has_alpha;
      LayerImage* layer;
      Cel* last_cel;
      int duration;
      // Flags after the user choose what to do with the sequence.
      int flags;
    } m_seq;

    class FileAbstractImageImpl;
    std::unique_ptr<FileAbstractImageImpl> m_abstractImage;

    void prepareForSequence();
    void makeAbstractImage();
    void makeDirectories();
  };

  // Available extensions for each load/save operation.
  base::paths get_readable_extensions();
  base::paths get_writable_extensions(const int requiredFormatFlag = 0);

  // High-level routines to load/save documents.
  Doc* load_document(Context* context, const std::string& filename);
  int save_document(Context* context, Doc* document);

  // Returns true if the given filename contains a file extension that
  // can be used to save only static images (i.e. animations are saved
  // as sequence of files).
  bool is_static_image_format(const std::string& filename);

} // namespace app

#endif
