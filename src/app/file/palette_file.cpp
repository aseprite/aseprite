// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/file_formats_manager.h"
#include "base/path.h"
#include "base/string.h"
#include "doc/cel.h"
#include "doc/file/col_file.h"
#include "doc/file/gpl_file.h"
#include "doc/file/pal_file.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include <cstring>

namespace app {

using namespace doc;

std::string get_readable_palette_extensions()
{
  std::string buf = get_readable_extensions();
  buf += ",col,gpl,pal";
  return buf;
}

std::string get_writable_palette_extensions()
{
  std::string buf = get_writable_extensions();
  buf += ",col,gpl,pal";
  return buf;
}

Palette* load_palette(const char *filename)
{
  std::string ext = base::string_to_lower(base::get_file_extension(filename));
  Palette* pal = NULL;

  if (ext == "col") {
    pal = doc::file::load_col_file(filename);
  }
  else if (ext == "gpl") {
    pal = doc::file::load_gpl_file(filename);
  }
  else if (ext == "pal") {
    pal = doc::file::load_pal_file(filename);
  }
  else {
    FileFormat* ff = FileFormatsManager::instance()->getFileFormatByExtension(ext.c_str());
    if (ff && ff->support(FILE_SUPPORT_LOAD)) {
      base::UniquePtr<FileOp> fop(
        FileOp::createLoadDocumentOperation(
          nullptr, filename,
          FILE_LOAD_SEQUENCE_NONE |
          FILE_LOAD_ONE_FRAME));

      if (fop && !fop->hasError()) {
        fop->operate(nullptr);
        fop->postLoad();

        if (fop->document() &&
            fop->document()->sprite() &&
            fop->document()->sprite()->palette(frame_t(0))) {
          pal = new Palette(
            *fop->document()->sprite()->palette(frame_t(0)));
        }

        delete fop->releaseDocument();
        fop->done();
      }
    }
  }

  if (pal)
    pal->setFilename(filename);

  return pal;
}

bool save_palette(const char *filename, const Palette* pal, int columns)
{
  std::string ext = base::string_to_lower(base::get_file_extension(filename));
  bool success = false;

  if (ext == "col") {
    success = doc::file::save_col_file(pal, filename);
  }
  else if (ext == "gpl") {
    success = doc::file::save_gpl_file(pal, filename);
  }
  else if (ext == "pal") {
    success = doc::file::save_pal_file(pal, filename);
  }
  else {
    FileFormat* ff = FileFormatsManager::instance()->getFileFormatByExtension(ext.c_str());
    if (ff && ff->support(FILE_SUPPORT_SAVE)) {
      int w = (columns > 0 ? columns: pal->size());
      int h = (pal->size() / w) + (pal->size() % w > 0 ? 1: 0);

      app::Context tmpContext;
      doc::Document* doc = tmpContext.documents().add(
        w, h, (pal->size() <= 256 ? doc::ColorMode::INDEXED:
                                    doc::ColorMode::RGB), pal->size());

      Sprite* sprite = doc->sprite();
      doc->sprite()->setPalette(pal, false);

      Layer* layer = sprite->folder()->getFirstLayer();
      Image* image = layer->cel(frame_t(0))->image();

      int x, y, c;
      for (y=c=0; y<h; ++y) {
        for (x=0; x<w; ++x, ++c) {
          if (doc->colorMode() == doc::ColorMode::INDEXED)
            image->putPixel(x, y, c);
          else
            image->putPixel(x, y, pal->entry(c));
        }
      }

      doc->setFilename(filename);
      success = (save_document(&tmpContext, doc) == 0);

      doc->close();
      delete doc;
    }
  }

  return success;
}

}
