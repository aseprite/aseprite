// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "base/file_handle.h"

#define QOI_NO_STDIO
#define QOI_IMPLEMENTATION
#include "qoi.h"

namespace app {

using namespace base;

class QoiFormat : public FileFormat {
  const char* onGetName() const override {
    return "qoi";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("qoi");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::QOI_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_SEQUENCES |
      FILE_ENCODE_ABSTRACT_IMAGE;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateQoiFormat()
{
  return new QoiFormat;
}

bool QoiFormat::onLoad(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();

  fseek(f, 0, SEEK_END);
  auto size = ftell(f);
  if (size <= 0)
    return false;
  fseek(f, 0, SEEK_SET);

  auto data = QOI_MALLOC(size);
  if (!data)
    return false;

  qoi_desc desc;
  auto bytes_read = fread(data, 1, size, f);
  auto pixels = qoi_decode(data, bytes_read, &desc, 0);
  QOI_FREE(data);
  if (!pixels)
    return false;

  ImageRef image = fop->sequenceImage(IMAGE_RGB,
                                      desc.width,
                                      desc.height);
  if (!image)
    return false;

  auto src = (const uint8_t*)pixels;
  for (int y=0; y<desc.height; ++y) {
    auto dst = (uint32_t*)image->getPixelAddress(0, y);
    switch (desc.channels) {
      case 4:
        for (int x=0; x<desc.width; ++x, ++dst) {
          *dst = doc::rgba(src[0], src[1], src[2], src[3]);
          src += 4;
        }
        break;
      case 3:
        for (int x=0; x<desc.width; ++x, ++dst) {
          *dst = doc::rgba(src[0], src[1], src[2], 255);
          src += 3;
        }
        break;
    }
  }

  QOI_FREE(pixels);

  if (desc.channels == 4)
    fop->sequenceSetHasAlpha(true);

  // Setup the color space.
  gfx::ColorSpaceRef colorSpace = nullptr;
  switch (desc.colorspace) {
    case QOI_SRGB:
      colorSpace = gfx::ColorSpace::MakeSRGB();
      break;
    case QOI_LINEAR:
      colorSpace = gfx::ColorSpace::MakeNone();
      break;
  }
  if (colorSpace &&
      fop->document()->sprite()->colorSpace()->type() == gfx::ColorSpace::None) {
    fop->setEmbeddedColorProfile();
    fop->document()->sprite()->setColorSpace(colorSpace);
    fop->document()->notifyColorSpaceChanged();
  }

  if (ferror(handle.get())) {
    fop->setError("Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

#ifdef ENABLE_SAVE

bool QoiFormat::onSave(FileOp* fop)
{
  const FileAbstractImage* img = fop->abstractImage();
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();
  doc::ImageRef image = img->getScaledImage();

  qoi_desc desc;
  desc.width = img->width();
  desc.height = img->height();
  desc.channels = (img->needAlpha() ? 4: 3);

  if (img->osColorSpace() &&
      img->osColorSpace()->isSRGB()) {
    desc.colorspace = QOI_SRGB;
  }
  else {
    // TODO support or warn about the color space
    desc.colorspace = QOI_LINEAR;
  }

  auto pixels = (uint8_t*)QOI_MALLOC(desc.width * desc.height * desc.channels);
  if (!pixels)
    return false;

  auto dst = pixels;
  for (int y=0; y<desc.height; ++y) {
    auto src = (uint32_t*)image->getPixelAddress(0, y);
    switch (desc.channels) {
      case 4:
        for (int x=0; x<desc.width; ++x, ++src) {
          uint32_t c = *src;
          dst[0] = doc::rgba_getr(c);
          dst[1] = doc::rgba_getg(c);
          dst[2] = doc::rgba_getb(c);
          dst[3] = doc::rgba_geta(c);
          dst += 4;
        }
        break;
      case 3:
        for (int x=0; x<desc.width; ++x, ++src) {
          uint32_t c = *src;
          dst[0] = doc::rgba_getr(c);
          dst[1] = doc::rgba_getg(c);
          dst[2] = doc::rgba_getb(c);
          dst += 3;
        }
        break;
    }
  }

  int size = 0;
  auto encoded = qoi_encode(pixels, &desc, &size);
  QOI_FREE(pixels);
  if (!encoded)
    return false;

  fwrite(encoded, 1, size, f);
  QOI_FREE(encoded);

  if (ferror(handle.get())) {
    fop->setError("Error writing file.\n");
    return false;
  }
  else {
    return true;
  }
}

#endif  // ENABLE_SAVE

} // namespace app
