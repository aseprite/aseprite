// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_FILE_H_INCLUDED
#define APP_FILE_FILE_H_INCLUDED
#pragma once

#include "base/mutex.h"
#include "base/shared_ptr.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/pixel_format.h"

#include <cstdio>
#include <string>
#include <vector>

// Flags for FileOp::createLoadDocumentOperation()
#define FILE_LOAD_SEQUENCE_NONE         0x00000001
#define FILE_LOAD_SEQUENCE_ASK          0x00000002
#define FILE_LOAD_SEQUENCE_YES          0x00000004
#define FILE_LOAD_ONE_FRAME             0x00000008

namespace doc {
  class Document;
  class FrameTag;
}

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class LayerImage;
  class Palette;
  class Sprite;
}

namespace app {
  class Context;
  class Document;
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
    FileOpROI(const app::Document* doc,
              const std::string& frameTagName,
              const doc::frame_t fromFrame,
              const doc::frame_t toFrame);

    const app::Document* document() const { return m_document; }
    doc::FrameTag* frameTag() const { return m_frameTag; }
    doc::frame_t fromFrame() const { return m_fromFrame; }
    doc::frame_t toFrame() const { return m_toFrame; }

    doc::frame_t frames() const {
      ASSERT(m_fromFrame >= 0);
      ASSERT(m_toFrame >= 0);
      return (m_toFrame - m_fromFrame + 1);
    }

  private:
    const app::Document* m_document;
    doc::FrameTag* m_frameTag;
    doc::frame_t m_fromFrame;
    doc::frame_t m_toFrame;
  };

  // Structure to load & save files.
  class FileOp {
  public:
    static FileOp* createLoadDocumentOperation(Context* context,
                                               const char* filename,
                                               int flags);

    static FileOp* createSaveDocumentOperation(const Context* context,
                                               const FileOpROI& roi,
                                               const char* filename,
                                               const char* filenameFormat);

    ~FileOp();

    bool isSequence() const { return !m_seq.filename_list.empty(); }
    bool isOneFrame() const { return m_oneframe; }

    const std::string& filename() const { return m_filename; }
    Context* context() const { return m_context; }
    Document* document() const { return m_document; }
    Document* releaseDocument() {
      Document* doc = m_document;
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

    const std::string& error() const { return m_error; }
    void setError(const char *error, ...);
    bool hasError() const { return !m_error.empty(); }

    double progress() const;
    void setProgress(double progress);

    void getFilenameList(std::vector<std::string>& output) const;

  private:
    FileOp();                   // Undefined
    FileOp(FileOpType type, Context* context);

    FileOpType m_type;          // Operation type: 0=load, 1=save.
    FileFormat* m_format;
    Context* m_context;
    // TODO this should be a shared pointer (and we should remove
    //      releaseDocument() member function)
    Document* m_document;       // Loaded document, or document to be saved.
    std::string m_filename;     // File-name to load/save.
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

    base::SharedPtr<FormatOptions> m_formatOptions;

    // Data for sequences.
    struct {
      std::vector<std::string> filename_list; // All file names to load/save.
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
    } m_seq;

    void prepareForSequence();
  };

  // Available extensions for each load/save operation.
  std::string get_readable_extensions();
  std::string get_writable_extensions();

  // High-level routines to load/save documents.
  app::Document* load_document(Context* context, const char* filename);
  int save_document(Context* context, doc::Document* document);

  // Returns true if the given filename contains a file extension that
  // can be used to save only static images (i.e. animations are saved
  // as sequence of files).
  bool is_static_image_format(const std::string& filename);

} // namespace app

#endif
