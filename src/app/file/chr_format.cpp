// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "base/cfile.h"
#include "base/file_handle.h"

namespace app {

using namespace base;

class ChrFormat : public FileFormat {
  const char* onGetName() const override { return "chr"; }

  void onGetExtensions(base::paths& exts) const override
  {
    exts.push_back("chr");
  }

  dio::FileFormat onGetDioFormat() const override
  {
    return dio::FileFormat::CHR_IMAGE;
  }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_SUPPORT_INDEXED |
           FILE_SUPPORT_SEQUENCES | FILE_CHR_LIMITATIONS;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateChrFormat()
{
  return new ChrFormat;
}

bool ChrFormat::onLoad(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();
  int x, y, sx = 0, sy = 0;
  uint8_t chr[2][8], c;

  ImageRef image = fop->sequenceImageToLoad(IMAGE_INDEXED, 128, 128);
  if (!image) {
    return false;
  }

  fop->sequenceSetNColors(4);
  fop->sequenceSetColor(0, 0, 0, 0);
  fop->sequenceSetColor(1, 0xff, 0, 0);
  fop->sequenceSetColor(2, 0, 0xff, 0);
  fop->sequenceSetColor(3, 0, 0, 0xff);

  clear_image(image.get(), 0);

  while (fread(chr, 1, 0x10, f) > 0 && sy < 0x10) {
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
        c = chr[0][y] >> (7 - x) & 1;
        c |= (chr[1][y] >> (7 - x) & 1) << 1;
        put_pixel(image.get(), sx * 8 + x, sy * 8 + y, c);
      }
    }
    if (++sx == 0x10) {
      sx = 0;
      sy++;
    }
  }

  if (ferror(f)) {
    fop->setError("Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

#ifdef ENABLE_SAVE
bool ChrFormat::onSave(FileOp* fop)
{
  const ImageRef image = fop->sequenceImageToSave();
  FileHandle handle(
    open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();
  int x, y, sx, sy;
  color_t c;
  uint8_t chr[2][8];

  for (sy = 0; sy < image->height() / 8; sy++) {
    for (sx = 0; sx < image->width() / 8; sx++) {
      memset(chr, 0, 0x10);
      for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
          c = image->getPixel(sx * 8 + x, sy * 8 + y);
          chr[0][y] |= (c & 1) << (7 - x);
          chr[1][y] |= (c >> 1 & 1) << (7 - x);
        }
      }
      fwrite(chr, 1, 0x10, f);
    }
    fop->setProgress((float)sy / (float)(image->height() / 8));
  }

  if (ferror(f)) {
    fop->setError("Error writing file.\n");
    return false;
  }
  else {
    return true;
  }
}
#endif

}  // namespace app
