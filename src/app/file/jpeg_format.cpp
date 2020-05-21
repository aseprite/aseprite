// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/pref/preferences.h"
#include "base/clamp.h"
#include "base/file_handle.h"
#include "base/memory.h"
#include "doc/doc.h"

#include <algorithm>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>

#include "jpeg_options.xml.h"

#include "jpeglib.h"

namespace app {

using namespace base;

class JpegFormat : public FileFormat {
  // Data for JPEG files
  class JpegOptions : public FormatOptions {
  public:
    JpegOptions() : quality(1.0f) { }    // 1.0 maximum quality.
    float quality;
  };

  const char* onGetName() const override {
    return "jpeg";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("jpeg");
    exts.push_back("jpg");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::JPEG_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
  gfx::ColorSpacePtr loadColorSpace(FileOp* fop, jpeg_decompress_struct* dinfo);
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
  void saveColorSpace(FileOp* fop, jpeg_compress_struct* cinfo,
                      const gfx::ColorSpace* colorSpace);
#endif

  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
};

FileFormat* CreateJpegFormat()
{
  return new JpegFormat;
}

struct error_mgr {
  struct jpeg_error_mgr head;
  jmp_buf setjmp_buffer;
  FileOp* fop;
};

static void error_exit(j_common_ptr cinfo)
{
  // Display the message.
  (*cinfo->err->output_message)(cinfo);

  // Return control to the setjmp point.
  longjmp(((struct error_mgr *)cinfo->err)->setjmp_buffer, 1);
}

static void output_message(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  // Format the message.
  (*cinfo->err->format_message)(cinfo, buffer);

  // Put in the log file if.
  LOG(ERROR) << "JPEG: \"" << buffer << "\"\n";

  // Leave the message for the application.
  ((struct error_mgr *)cinfo->err)->fop->setError("%s\n", buffer);
}

// Some code to read color spaces from jpeg files is from Skia
// (SkJpegCodec.cpp) by Google Inc.
static constexpr uint32_t kMarkerMaxSize = 65533;
static constexpr uint32_t kICCMarker = JPEG_APP0 + 2;
static constexpr uint32_t kICCMarkerHeaderSize = 14;
static constexpr uint32_t kICCAvailDataPerMarker = (kMarkerMaxSize - kICCMarkerHeaderSize);
static constexpr uint8_t kICCSig[] = { 'I', 'C', 'C', '_', 'P', 'R', 'O', 'F', 'I', 'L', 'E', '\0' };

static bool is_icc_marker(jpeg_marker_struct* marker)
{
  if (kICCMarker != marker->marker ||
      marker->data_length < kICCMarkerHeaderSize) {
    return false;
  }
  else
    return !memcmp(marker->data, kICCSig, sizeof(kICCSig));
}

bool JpegFormat::onLoad(FileOp* fop)
{
  struct jpeg_decompress_struct dinfo;
  struct error_mgr jerr;
  JDIMENSION num_scanlines;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  int c;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* file = handle.get();

  // Initialize the JPEG decompression object with error handling.
  jerr.fop = fop;
  dinfo.err = jpeg_std_error(&jerr.head);

  jerr.head.error_exit = error_exit;
  jerr.head.output_message = output_message;

  // Establish the setjmp return context for error_exit to use.
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&dinfo);
    return false;
  }

  jpeg_create_decompress(&dinfo);

  // Specify data source for decompression.
  jpeg_stdio_src(&dinfo, file);

  // Instruct jpeg library to save the markers that we care
  // about. Since the color profile will not change, we can skip this
  // step on rewinds.
  jpeg_save_markers(&dinfo, kICCMarker, 0xFFFF);

  // Read file header, set default decompression parameters.
  jpeg_read_header(&dinfo, true);

  if (dinfo.jpeg_color_space == JCS_GRAYSCALE)
    dinfo.out_color_space = JCS_GRAYSCALE;
  else
    dinfo.out_color_space = JCS_RGB;

  // Start decompressor.
  jpeg_start_decompress(&dinfo);

  // Create the image.
  Image* image = fop->sequenceImage(
    (dinfo.out_color_space == JCS_RGB ? IMAGE_RGB:
                                        IMAGE_GRAYSCALE),
    dinfo.output_width,
    dinfo.output_height);
  if (!image) {
    jpeg_destroy_decompress(&dinfo);
    return false;
  }

  // Create the buffer.
  buffer_height = dinfo.rec_outbuf_height;
  buffer = (JSAMPARRAY)base_malloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    jpeg_destroy_decompress(&dinfo);
    return false;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = (JSAMPROW)base_malloc(sizeof(JSAMPLE) *
                                      dinfo.output_width * dinfo.output_components);
    if (!buffer[c]) {
      for (c--; c>=0; c--)
        base_free(buffer[c]);
      base_free(buffer);
      jpeg_destroy_decompress(&dinfo);
      return false;
    }
  }

  // Generate a grayscale palette if is necessary.
  if (image->pixelFormat() == IMAGE_GRAYSCALE)
    for (c=0; c<256; c++)
      fop->sequenceSetColor(c, c, c, c);

  // Read each scan line.
  while (dinfo.output_scanline < dinfo.output_height) {
    num_scanlines = jpeg_read_scanlines(&dinfo, buffer, buffer_height);

    // RGB
    if (image->pixelFormat() == IMAGE_RGB) {
      uint8_t* src_address;
      uint32_t* dst_address;
      int x, y, r, g, b;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((uint8_t**)buffer)[y];
        dst_address = (uint32_t*)image->getPixelAddress(0, dinfo.output_scanline-1+y);

        for (x=0; x<image->width(); x++) {
          r = *(src_address++);
          g = *(src_address++);
          b = *(src_address++);
          *(dst_address++) = rgba(r, g, b, 255);
        }
      }
    }
    // Grayscale
    else {
      uint8_t* src_address;
      uint16_t* dst_address;
      int x, y;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((uint8_t**)buffer)[y];
        dst_address = (uint16_t*)image->getPixelAddress(0, dinfo.output_scanline-1+y);

        for (x=0; x<image->width(); x++)
          *(dst_address++) = graya(*(src_address++), 255);
      }
    }

    fop->setProgress((float)(dinfo.output_scanline+1) / (float)(dinfo.output_height));
    if (fop->isStop())
      break;
  }

  // Read color space
  gfx::ColorSpacePtr colorSpace = loadColorSpace(fop, &dinfo);
  if (colorSpace)
    fop->setEmbeddedColorProfile();
  else { // sRGB is the default JPG color space.
    colorSpace = gfx::ColorSpace::MakeSRGB();
  }
  if (colorSpace &&
      fop->document()->sprite()->colorSpace()->type() == gfx::ColorSpace::None) {
    fop->document()->sprite()->setColorSpace(colorSpace);
    fop->document()->notifyColorSpaceChanged();
  }

  for (c=0; c<(int)buffer_height; c++)
    base_free(buffer[c]);
  base_free(buffer);

  jpeg_finish_decompress(&dinfo);
  jpeg_destroy_decompress(&dinfo);

  return true;
}

// ICC profiles may be stored using a sequence of multiple markers.  We obtain the ICC profile
// in two steps:
//   (1) Discover all ICC profile markers and verify that they are numbered properly.
//   (2) Copy the data from each marker into a contiguous ICC profile.
gfx::ColorSpacePtr JpegFormat::loadColorSpace(FileOp* fop, jpeg_decompress_struct* dinfo)
{
  // Note that 256 will be enough storage space since each markerIndex is stored in 8-bits.
  jpeg_marker_struct* markerSequence[256];
  memset(markerSequence, 0, sizeof(markerSequence));
  uint8_t numMarkers = 0;
  size_t totalBytes = 0;

  // Discover any ICC markers and verify that they are numbered properly.
  for (jpeg_marker_struct* marker = dinfo->marker_list; marker; marker = marker->next) {
    if (is_icc_marker(marker)) {
      // Verify that numMarkers is valid and consistent.
      if (0 == numMarkers) {
        numMarkers = marker->data[13];
        if (0 == numMarkers) {
          fop->setError("ICC Profile Error: numMarkers must be greater than zero.\n");
          return nullptr;
        }
      }
      else if (numMarkers != marker->data[13]) {
        fop->setError("ICC Profile Error: numMarkers must be consistent.\n");
        return nullptr;
      }

      // Verify that the markerIndex is valid and unique.  Note that zero is not
      // a valid index.
      uint8_t markerIndex = marker->data[12];
      if (markerIndex == 0 || markerIndex > numMarkers) {
        fop->setError("ICC Profile Error: markerIndex is invalid.\n");
        return nullptr;
      }
      if (markerSequence[markerIndex]) {
        fop->setError("ICC Profile Error: Duplicate value of markerIndex.\n");
        return nullptr;
      }
      markerSequence[markerIndex] = marker;
      ASSERT(marker->data_length >= kICCMarkerHeaderSize);
      totalBytes += marker->data_length - kICCMarkerHeaderSize;
    }
  }

  if (0 == totalBytes) {
    // No non-empty ICC profile markers were found.
    return nullptr;
  }

  // Combine the ICC marker data into a contiguous profile.
  std::vector<uint8_t> iccData(totalBytes);
  uint8_t* dst = &iccData[0];
  for (uint32_t i = 1; i <= numMarkers; i++) {
    jpeg_marker_struct* marker = markerSequence[i];
    if (!marker) {
      fop->setError("ICC Profile Error: Missing marker %d of %d.\n", i, numMarkers);
      return nullptr;
    }

    uint8_t* src = ((uint8_t*)marker->data) + kICCMarkerHeaderSize;
    size_t bytes = marker->data_length - kICCMarkerHeaderSize;
    memcpy(dst, src, bytes);
    dst = dst + bytes;
  }

  return gfx::ColorSpace::MakeICC(std::move(iccData));
}

#ifdef ENABLE_SAVE

bool JpegFormat::onSave(FileOp* fop)
{
  struct jpeg_compress_struct cinfo;
  struct error_mgr jerr;
  const Image* image = fop->sequenceImage();
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  const auto jpeg_options = std::static_pointer_cast<JpegOptions>(fop->formatOptions());
  const int qualityValue = (int)base::clamp(100.0f * jpeg_options->quality, 0.f, 100.f);

  int c;

  LOG("JPEG: Saving with options: quality=%d\n", qualityValue);

  // Open the file for write in it.
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* file = handle.get();

  // Allocate and initialize JPEG compression object.
  jerr.fop = fop;
  cinfo.err = jpeg_std_error(&jerr.head);
  jpeg_create_compress(&cinfo);

  // SPECIFY data destination file.
  jpeg_stdio_dest(&cinfo, file);

  // SET parameters for compression.
  cinfo.image_width = image->width();
  cinfo.image_height = image->height();

  if (image->pixelFormat() == IMAGE_GRAYSCALE) {
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
  }
  else {
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
  }

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, qualityValue, true);
  cinfo.dct_method = JDCT_ISLOW;
  cinfo.smoothing_factor = 0;

  // START compressor.
  jpeg_start_compress(&cinfo, true);

  // Save color space
  if (fop->preserveColorProfile() &&
      fop->document()->sprite()->colorSpace())
    saveColorSpace(fop, &cinfo, fop->document()->sprite()->colorSpace().get());

  // CREATE the buffer.
  buffer_height = 1;
  buffer = (JSAMPARRAY)base_malloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    fop->setError("Not enough memory for the buffer.\n");
    jpeg_destroy_compress(&cinfo);
    return false;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = (JSAMPROW)base_malloc(sizeof(JSAMPLE) *
                                      cinfo.image_width * cinfo.num_components);
    if (!buffer[c]) {
      fop->setError("Not enough memory for buffer scanlines.\n");
      for (c--; c>=0; c--)
        base_free(buffer[c]);
      base_free(buffer);
      jpeg_destroy_compress(&cinfo);
      return false;
    }
  }

  // Write each scan line.
  while (cinfo.next_scanline < cinfo.image_height) {
    // RGB
    if (image->pixelFormat() == IMAGE_RGB) {
      uint32_t* src_address;
      uint8_t* dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = (uint32_t*)image->getPixelAddress(0, cinfo.next_scanline+y);
        dst_address = ((uint8_t**)buffer)[y];

        for (x=0; x<image->width(); ++x) {
          c = *(src_address++);
          *(dst_address++) = rgba_getr(c);
          *(dst_address++) = rgba_getg(c);
          *(dst_address++) = rgba_getb(c);
        }
      }
    }
    // Grayscale.
    else {
      uint16_t* src_address;
      uint8_t* dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = (uint16_t*)image->getPixelAddress(0, cinfo.next_scanline+y);
        dst_address = ((uint8_t**)buffer)[y];
        for (x=0; x<image->width(); ++x)
          *(dst_address++) = graya_getv(*(src_address++));
      }
    }
    jpeg_write_scanlines(&cinfo, buffer, buffer_height);

    fop->setProgress((float)(cinfo.next_scanline+1) / (float)(cinfo.image_height));
  }

  // Destroy all data.
  for (c=0; c<(int)buffer_height; c++)
    base_free(buffer[c]);
  base_free(buffer);

  // Finish compression.
  jpeg_finish_compress(&cinfo);

  // Release JPEG compression object.
  jpeg_destroy_compress(&cinfo);

  // All fine.
  return true;
}

void JpegFormat::saveColorSpace(FileOp* fop, jpeg_compress_struct* cinfo,
                                const gfx::ColorSpace* colorSpace)
{
  if (!colorSpace || colorSpace->type() != gfx::ColorSpace::ICC)
    return;

  size_t iccSize = colorSpace->iccSize();
  auto iccData = (const uint8_t*)colorSpace->iccData();
  if (!iccSize || !iccData)
    return;

  std::vector<uint8_t> markerData(kMarkerMaxSize);
  int markerIndex = 1;
  int numMarkers =
    (iccSize / kICCAvailDataPerMarker) +
    (iccSize % kICCAvailDataPerMarker > 0 ? 1: 0);

  // ICC profile too big to fit in JPEG markers (64kb*255 ~= 16mb)
  if (numMarkers > 255) {
    fop->setError("ICC profile is too big to enter in the JPEG file.\n");
    return;
  }

  while (iccSize > 0) {
    const size_t n = std::min<int>(iccSize, kICCAvailDataPerMarker);

    ASSERT(n > 0);
    ASSERT(n < kICCAvailDataPerMarker);

    // Marker Header
    std::copy(kICCSig, kICCSig+sizeof(kICCSig), &markerData[0]);
    markerData[sizeof(kICCSig)  ] = markerIndex;
    markerData[sizeof(kICCSig)+1] = numMarkers;

    // Marker Data
    std::copy(iccData, iccData+n, &markerData[kICCMarkerHeaderSize]);

    jpeg_write_marker(cinfo, kICCMarker, &markerData[0], kICCMarkerHeaderSize + n);

    ++markerIndex;
    iccSize -= n;
    iccData += n;
  }
}

#endif  // ENABLE_SAVE

// Shows the JPEG configuration dialog.
FormatOptionsPtr JpegFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<JpegOptions>();
#ifdef ENABLE_UI
  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();

      if (pref.isSet(pref.jpeg.quality))
        opts->quality = pref.jpeg.quality();

      if (pref.jpeg.showAlert()) {
        app::gen::JpegOptions win;
        win.quality()->setValue(int(opts->quality * 10.0f));
        win.openWindowInForeground();

        if (win.closer() == win.ok()) {
          pref.jpeg.quality(float(win.quality()->getValue()) / 10.0f);
          pref.jpeg.showAlert(!win.dontShow()->isSelected());

          opts->quality = pref.jpeg.quality();
        }
        else {
          opts.reset();
        }
      }
    }
    catch (std::exception& e) {
      Console::showException(e);
      return std::shared_ptr<JpegOptions>(0);
    }
  }
#endif // ENABLE_UI
  return opts;
}

} // namespace app
