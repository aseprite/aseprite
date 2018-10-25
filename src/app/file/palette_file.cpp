// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/file_formats_manager.h"
#include "base/fs.h"
#include "base/string.h"
#include "dio/detect_format.h"
#include "doc/cel.h"
#include "doc/file/col_file.h"
#include "doc/file/gpl_file.h"
#include "doc/file/hex_file.h"
#include "doc/file/pal_file.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include <cstring>

namespace app {

using namespace doc;

static const char* palExts[] = { "col", "gpl", "hex", "pal" };

base::paths get_readable_palette_extensions()
{
  base::paths paths = get_readable_extensions();
  for (const char* s : palExts)
    paths.push_back(s);
  return paths;
}

base::paths get_writable_palette_extensions()
{
  base::paths paths = get_writable_extensions();
  for (const char* s : palExts)
    paths.push_back(s);
  return paths;
}

Palette* load_palette(const char* filename)
{
  dio::FileFormat dioFormat = dio::detect_format(filename);
  Palette* pal = nullptr;

  switch (dioFormat) {

    case dio::FileFormat::COL_PALETTE:
      pal = doc::file::load_col_file(filename);
      break;

    case dio::FileFormat::GPL_PALETTE:
      pal = doc::file::load_gpl_file(filename);
      break;

    case dio::FileFormat::HEX_PALETTE:
      pal = doc::file::load_hex_file(filename);
      break;

    case dio::FileFormat::PAL_PALETTE:
      pal = doc::file::load_pal_file(filename);
      break;

    default: {
      FileFormat* ff = FileFormatsManager::instance()->getFileFormat(dioFormat);
      if (!ff || !ff->support(FILE_SUPPORT_LOAD))
        break;

      std::unique_ptr<FileOp> fop(
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
      break;
    }
  }

  if (pal)
    pal->setFilename(filename);

  return pal;
}

bool save_palette(const char* filename, const Palette* pal, int columns)
{
  dio::FileFormat dioFormat = dio::detect_format_by_file_extension(filename);
  bool success = false;

  switch (dioFormat) {

    case dio::FileFormat::COL_PALETTE:
      success = doc::file::save_col_file(pal, filename);
      break;

    case dio::FileFormat::GPL_PALETTE:
      success = doc::file::save_gpl_file(pal, filename);
      break;

    case dio::FileFormat::HEX_PALETTE:
      success = doc::file::save_hex_file(pal, filename);
      break;

    case dio::FileFormat::PAL_PALETTE:
      success = doc::file::save_pal_file(pal, filename);
      break;

    default: {
      FileFormat* ff = FileFormatsManager::instance()->getFileFormat(dioFormat);
      if (!ff || !ff->support(FILE_SUPPORT_SAVE))
        break;

      int w = (columns > 0 ? MID(0, columns, pal->size()): pal->size());
      int h = (pal->size() / w) + (pal->size() % w > 0 ? 1: 0);

      Context tmpContext;
      Doc* doc = tmpContext.documents().add(
        new Doc(Sprite::createBasicSprite(
                  ImageSpec((pal->size() <= 256 ? doc::ColorMode::INDEXED:
                                                  doc::ColorMode::RGB),
                            w, h), pal->size())));

      Sprite* sprite = doc->sprite();
      doc->sprite()->setPalette(pal, false);

      LayerImage* layer = static_cast<LayerImage*>(sprite->root()->firstLayer());
      layer->configureAsBackground();

      Image* image = layer->cel(frame_t(0))->image();
      image->clear(0);

      int x, y, c;
      for (y=c=0; y<h; ++y) {
        for (x=0; x<w; ++x) {
          if (doc->colorMode() == doc::ColorMode::INDEXED)
            image->putPixel(x, y, c);
          else
            image->putPixel(x, y, pal->entry(c));

          if (++c == pal->size())
            goto done;
        }
      }
    done:;

      doc->setFilename(filename);
      success = (save_document(&tmpContext, doc) == 0);

      doc->close();
      delete doc;
      break;
    }
  }

  return success;
}

}
