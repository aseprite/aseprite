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

#include "tinytiff_defs.h"
#include "tinytiff_export.h"
#include "tinytiff_tools.hxx"
#include "tinytiffreader.hxx"
#include "tinytiffwriter.h"


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
  PixelFormat pixelFormat;
  int imageWidth = 100;
  int imageHeight = 100;

  //FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  //FILE* file = handle.get();

  TinyTIFFReaderFile* tiffr = NULL;
  tiffr = TinyTIFFReader_open(fop->filename().c_str());
  if (tiffr) {
    imageWidth = TinyTIFFReader_getWidth(tiffr);
    imageHeight = TinyTIFFReader_getHeight(tiffr);
    const uint16_t samples = TinyTIFFReader_getSamplesPerPixel(tiffr);
    const uint16_t bitspersample = TinyTIFFReader_getBitsPerSample(tiffr, 0);
    
    uint8_t* r;
    uint8_t* g;
    uint8_t* b;
    uint8_t* a;

    if(samples == 1){ // grayscale
      g = (uint8_t*)calloc(imageWidth * imageHeight, bitspersample / 8);
      TinyTIFFReader_getSampleData(tiffr, g, 0);
      fop->sequenceSetHasAlpha(false);
      pixelFormat = IMAGE_GRAYSCALE;
    }if(samples == 2){ // grayscale Alpha
      g = (uint8_t*)calloc(imageWidth * imageHeight, bitspersample / 8);
      a = (uint8_t*)calloc(imageWidth * imageHeight, bitspersample / 8);
      TinyTIFFReader_getSampleData(tiffr, g, 0);
      TinyTIFFReader_getSampleData(tiffr, a, 1);
      fop->sequenceSetHasAlpha(true);
      pixelFormat = IMAGE_GRAYSCALE;
    }if(samples == 3 || samples == 4){ // RGB
      r = (uint8_t*)calloc(imageWidth * imageHeight, bitspersample / 8);
      g = (uint8_t*)calloc(imageWidth * imageHeight, bitspersample / 8);
      b = (uint8_t*)calloc(imageWidth * imageHeight, bitspersample / 8);
      TinyTIFFReader_getSampleData(tiffr, r, 0);
      TinyTIFFReader_getSampleData(tiffr, g, 1);
      TinyTIFFReader_getSampleData(tiffr, b, 2);
      fop->sequenceSetHasAlpha(false);
      pixelFormat = IMAGE_RGB;
    }if(samples == 4){ // RGBA
      a = (uint8_t*)calloc(imageWidth * imageHeight, bitspersample / 8);
      TinyTIFFReader_getSampleData(tiffr, a, 3);
      fop->sequenceSetHasAlpha(true);
    }

    Image* image = fop->sequenceImage(pixelFormat, imageWidth, imageHeight);
    if (!image) {
      fop->setError("file_sequence_image %dx%d\n", imageWidth, imageHeight);
      return false;
    }

    for (int y = 0; y < imageHeight; y++) {
      //uint8_t* src_address = rows_pointer[y];
      uint32_t* dst_address = (uint32_t*)image->getPixelAddress(0, y);
      unsigned int x, R, G, B, A;
      
      if(samples == 1){// grayscale
        for (x=0; x<imageWidth; x++) {
          G = g[y * imageWidth + x];
          *(dst_address++) = graya(G, 255);
        }
      }else if(samples == 2){// grayscale Alpha
        for (x=0; x<imageWidth; x++) {
          G = g[y * imageWidth + x];
          A = a[y * imageWidth + x];
          *(dst_address++) = graya(G, A);
        }
      }else if(samples == 3){ // RGB
        for (x=0; x<imageWidth; x++) {
          R = r[y * imageWidth + x];
          G = g[y * imageWidth + x];
          B = b[y * imageWidth + x];
          *(dst_address++) = rgba(R, G, B, 255);
        }
      }if(samples == 4){
        for (x=0; x<imageWidth; x++) {
          R = r[y * imageWidth + x];
          G = g[y * imageWidth + x];
          B = b[y * imageWidth + x];
          A = a[y * imageWidth + x];
          *(dst_address++) = rgba(R, G, B, A);
        }
      }
    }
  } else {
    return false;
  }
  TinyTIFFReader_close(tiffr);

  return true;
}

#ifdef ENABLE_SAVE

bool TiffFormat::onSave(FileOp* fop)
{
  return false;
}
#endif

} // namespace app
