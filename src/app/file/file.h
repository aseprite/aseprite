/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_FILE_FILE_H_INCLUDED
#define APP_FILE_FILE_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/pixel_format.h"

#include <stdio.h>
#include <string>
#include <vector>

#define FILE_LOAD_SEQUENCE_NONE         0x00000001
#define FILE_LOAD_SEQUENCE_ASK          0x00000002
#define FILE_LOAD_SEQUENCE_YES          0x00000004
#define FILE_LOAD_ONE_FRAME             0x00000008

namespace base {
  class mutex;
}

namespace doc {
  class Document;
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

  class IFileOpProgress
  {
  public:
    virtual ~IFileOpProgress() { }
    virtual void ackFileOpProgress(double progress) = 0;
  };

  // Structure to load & save files.
  struct FileOp {
    FileOpType type;              // Operation type: 0=load, 1=save.
    FileFormat* format;
    void* format_data;            // Custom data for the FileFormat::onLoad/onSave operations.
    Context* context;
    Document* document;           // Loaded document, or document to be saved.
    std::string filename;         // File-name to load/save.

    // Shared fields between threads.
    base::mutex* mutex;           // Mutex to access to the next two fields.
    double progress;              // Progress (1.0 is ready).
    IFileOpProgress* progressInterface;
    std::string error;            // Error string.
    bool done;                    // True if the operation finished.
    bool stop;                    // Force the break of the operation.
    bool oneframe;                // Load just one frame (in formats
                                  // that support animation like
                                  // GIF/FLI/ASE).

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
      SharedPtr<FormatOptions> format_options;
    } seq;

    ~FileOp();

    bool has_error() const {
      return !this->error.empty();
    }

    bool is_sequence() const {
      return !this->seq.filename_list.empty();
    }

    void createDocument(Sprite* spr);
  };

  // Available extensions for each load/save operation.

  void get_readable_extensions(char* buf, int size);
  void get_writable_extensions(char* buf, int size);

  // High-level routines to load/save documents.

  app::Document* load_document(Context* context, const char* filename);
  int save_document(Context* context, doc::Document* document);

  // Low-level routines to load/save documents.

  FileOp* fop_to_load_document(Context* context, const char* filename, int flags);
  FileOp* fop_to_save_document(Context* context, Document* document, const char* fn_format);
  void fop_operate(FileOp* fop, IFileOpProgress* progress);
  void fop_done(FileOp* fop);
  void fop_stop(FileOp* fop);
  void fop_free(FileOp* fop);

  // Does extra post-load processing which may require user intervention.
  void fop_post_load(FileOp* fop);

  void fop_sequence_set_format_options(FileOp* fop, const SharedPtr<FormatOptions>& format_options);
  void fop_sequence_set_color(FileOp* fop, int index, int r, int g, int b);
  void fop_sequence_get_color(FileOp* fop, int index, int *r, int *g, int *b);
  Image* fop_sequence_image(FileOp* fi, PixelFormat pixelFormat, int w, int h);

  void fop_error(FileOp* fop, const char *error, ...);
  void fop_progress(FileOp* fop, double progress);

  double fop_get_progress(FileOp* fop);
  bool fop_is_done(FileOp* fop);
  bool fop_is_stop(FileOp* fop);

} // namespace app

#endif
