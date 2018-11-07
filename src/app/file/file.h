// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_FILE_H_INCLUDED
#define APP_FILE_FILE_H_INCLUDED
#pragma once

#include "base/mutex.h"
#include "base/paths.h"
#include "base/shared_ptr.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/pixel_format.h"
#include "doc/selected_frames.h"

#include <cstdio>
#include <string>

// Flags for FileOp::createLoadDocumentOperation()
#define FILE_LOAD_SEQUENCE_NONE         0x00000001
#define FILE_LOAD_SEQUENCE_ASK          0x00000002
#define FILE_LOAD_SEQUENCE_ASK_CHECKBOX 0x00000004
#define FILE_LOAD_SEQUENCE_YES          0x00000008
#define FILE_LOAD_ONE_FRAME             0x00000010
#define FILE_LOAD_DATA_FILE             0x00000020

namespace doc {
  class FrameTag;
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
  class Doc;
  class FileFormat;
  class FormatOptions;

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
              const std::string& sliceName,
              const std::string& frameTagName,
              const doc::SelectedFrames& selFrames,
              const bool adjustByFrameTag);

    const Doc* document() const { return m_document; }
    doc::Slice* slice() const { return m_slice; }
    doc::FrameTag* frameTag() const { return m_frameTag; }
    doc::frame_t fromFrame() const { return m_selFrames.firstFrame(); }
    doc::frame_t toFrame() const { return m_selFrames.lastFrame(); }
    const doc::SelectedFrames& selectedFrames() const { return m_selFrames; }

    doc::frame_t frames() const {
      return (doc::frame_t)m_selFrames.size();
    }

  private:
    const Doc* m_document;
    doc::Slice* m_slice;
    doc::FrameTag* m_frameTag;
    doc::SelectedFrames m_selFrames;
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
                                               int flags);

    static FileOp* createSaveDocumentOperation(const Context* context,
                                               const FileOpROI& roi,
                                               const std::string& filename,
                                               const std::string& filenameFormat,
                                               const bool ignoreEmptyFrames);

    ~FileOp();

    bool isSequence() const { return !m_seq.filename_list.empty(); }
    bool isOneFrame() const { return m_oneframe; }
    bool preserveColorProfile() const { return m_preserveColorProfile; }

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

    void createDocument(Sprite* spr);
    void operate(IFileOpProgress* progress = nullptr);

    void done();
    void stop();
    bool isDone() const;
    bool isStop() const;

    // Does extra post-load processing which may require user intervention.
    void postLoad();

    // Special options specific to the file format.
    base::SharedPtr<FormatOptions> formatOptions() const;
    void setFormatOptions(const base::SharedPtr<FormatOptions>& opts);

    // Helpers for file decoder/encoder (FileFormat) with
    // FILE_SUPPORT_SEQUENCES flag.
    void sequenceSetNColors(int ncolors);
    int sequenceGetNColors() const;
    void sequenceSetColor(int index, int r, int g, int b);
    void sequenceGetColor(int index, int* r, int* g, int* b) const;
    void sequenceSetAlpha(int index, int a);
    void sequenceGetAlpha(int index, int* a) const;
    Image* sequenceImage(PixelFormat pixelFormat, int w, int h);
    const Image* sequenceImage() const { return m_seq.image.get(); }
    bool sequenceGetHasAlpha() const {
      return m_seq.has_alpha;
    }
    void sequenceSetHasAlpha(bool hasAlpha) {
      m_seq.has_alpha = hasAlpha;
    }
    int sequenceFlags() const {
      return m_seq.flags;
    }

    const std::string& error() const { return m_error; }
    void setError(const char *error, ...);
    bool hasError() const { return !m_error.empty(); }

    double progress() const;
    void setProgress(double progress);

    void getFilenameList(base::paths& output) const;

    void setEmbeddedColorProfile() { m_embeddedColorProfile = true; }
    bool hasEmbeddedColorProfile() const { return m_embeddedColorProfile; }

  private:
    FileOp();                   // Undefined
    FileOp(FileOpType type, Context* context);

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
    mutable base::mutex m_mutex; // Mutex to access to the next two fields.
    double m_progress;          // Progress (1.0 is ready).
    IFileOpProgress* m_progressInterface;
    std::string m_error;        // Error string.
    bool m_done;                // True if the operation finished.
    bool m_stop;                // Force the break of the operation.
    bool m_oneframe;            // Load just one frame (in formats
                                // that support animation like
                                // GIF/FLI/ASE).
    bool m_ignoreEmpty;

    // Return if we've to save/embed the color space of the document
    // in the file.
    bool m_preserveColorProfile;

    // True if the file contained a color profile when it was loaded.
    bool m_embeddedColorProfile;

    base::SharedPtr<FormatOptions> m_formatOptions;

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
      // Flags after the user choose what to do with the sequence.
      int flags;
    } m_seq;

    void prepareForSequence();
  };

  // Available extensions for each load/save operation.
  base::paths get_readable_extensions();
  base::paths get_writable_extensions();

  // High-level routines to load/save documents.
  Doc* load_document(Context* context, const std::string& filename);
  int save_document(Context* context, Doc* document);

  // Returns true if the given filename contains a file extension that
  // can be used to save only static images (i.e. animations are saved
  // as sequence of files).
  bool is_static_image_format(const std::string& filename);

} // namespace app

#endif
