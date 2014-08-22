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
#include "raster/cel.h"
#include "raster/file/col_file.h"
#include "raster/file/gpl_file.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"

namespace app {

using namespace raster;

void get_readable_palette_extensions(char* buf, int size)
{
  get_readable_extensions(buf, size);
  strcat(buf, ",col,gpl");
}

void get_writable_palette_extensions(char* buf, int size)
{
  get_writable_extensions(buf, size);
  strcat(buf, ",col,gpl");
}

Palette* load_palette(const char *filename)
{
  std::string ext = base::string_to_lower(base::get_file_extension(filename));
  Palette* pal = NULL;

  if (ext == "col") {
    pal = raster::file::load_col_file(filename);
  }
  else if (ext == "gpl") {
    pal = raster::file::load_gpl_file(filename);
  }
  else {
    FileFormat* ff = FileFormatsManager::instance()->getFileFormatByExtension(ext.c_str());
    if (ff->support(FILE_SUPPORT_LOAD)) {
      FileOp* fop = fop_to_load_document(NULL, filename,
        FILE_LOAD_SEQUENCE_NONE |
        FILE_LOAD_ONE_FRAME);
      if (fop && !fop->has_error()) {
        fop_operate(fop, NULL);
        fop_post_load(fop);

        if (fop->document &&
            fop->document->sprite() &&
            fop->document->sprite()->getPalette(FrameNumber(0))) {
          pal = new Palette(
            *fop->document->sprite()->getPalette(FrameNumber(0)));

          // TODO remove this line when support for palettes with less
          // than 256 colors is added.
          pal->resize(Palette::MaxColors);
        }

        delete fop->document;
        fop_done(fop);
      }
    }
  }

  if (pal)
    pal->setFilename(filename);

  return pal;
}

bool save_palette(const char *filename, Palette* pal)
{
  std::string ext = base::string_to_lower(base::get_file_extension(filename));
  bool success = false;

  if (ext == "col") {
    success = raster::file::save_col_file(pal, filename);
  }
  else if (ext == "gpl") {
    success = raster::file::save_gpl_file(pal, filename);
  }
  else {
    FileFormat* ff = FileFormatsManager::instance()->getFileFormatByExtension(ext.c_str());
    if (ff->support(FILE_SUPPORT_SAVE)) {
      app::Context tmpContext;
      doc::Document* doc = tmpContext.documents().add(
        16, 16, doc::ColorMode::INDEXED,
        Palette::MaxColors);

      Sprite* sprite = doc->sprite();
      doc->sprite()->setPalette(pal, false);

      LayerImage* layer = dynamic_cast<LayerImage*>(sprite->folder()->getFirstLayer());
      Image* image = layer->getCel(FrameNumber(0))->image();

      int x, y, c;
      for (y=c=0; y<16; y++)
        for (x=0; x<16; x++)
          image->putPixel(x, y, c++);

      doc->setFilename(filename);
      success = (save_document(&tmpContext, doc) == 0);

      doc->close();
      delete doc;
    }
  }

  return success;
}

}
