// Aseprite
// Copyright (c) 2018-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/pref/preferences.h"
#include "base/cfile.h"
#include "base/clamp.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "ui/window.h"

namespace app {

using namespace base;

class TiffFormat : public FileFormat {

  const char* onGetName() const override {
    return "tiff";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("tiff");
    exts.push_back("tif");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::TIFF_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_GRAYA |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS |
      FILE_SUPPORT_PALETTE_WITH_ALPHA;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateTiffFormat()
{
  return new TiffFormat;
}

bool TiffFormat::onLoad(FileOp* fop)
{
  PixelFormat pixelFormat = IMAGE_RGB;
  int imageWidth = 100;
  int imageHeight = 100;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* file = handle.get();

  Image* image = fop->sequenceImage(pixelFormat, imageWidth, imageHeight);
  if (!image) {
    fop->setError("file_sequence_image %dx%d\n", imageWidth, imageHeight);
    return false;
  }
  
  for (int y = 0; y < imageHeight; y++) {
    //uint8_t* src_address = rows_pointer[y];
    uint32_t* dst_address = (uint32_t*)image->getPixelAddress(0, y);
    unsigned int x, r, g, b, a;

    for (x=0; x<imageWidth; x++) {
      r = 255;
      g = 0;
      b = 0;
      a = 255;
      *(dst_address++) = rgba(r, g, b, a);
    }
  }

  return true;
}

#ifdef ENABLE_SAVE

bool TiffFormat::onSave(FileOp* fop)
{
  return false;
}
#endif

} // namespace app
