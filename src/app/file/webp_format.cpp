// Aseprite
// Copyright (C) 2015 Gabriel Rauter
// Copyright (C) 2015-2016 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/console.h"
#include "app/context.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/file/webp_options.h"
#include "app/ini_file.h"
#include "base/file_handle.h"
#include "base/convert_to.h"
#include "doc/doc.h"

#include "webp_options.xml.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <map>

// Include webp libraries
#include <webp/decode.h>
#include <webp/encode.h>

namespace app {

using namespace base;

class WebPFormat : public FileFormat {

  const char* onGetName() const override { return "webp"; }
  const char* onGetExtensions() const override { return "webp"; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif

base::SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) override;
};

FileFormat* CreateWebPFormat()
{
  return new WebPFormat;
}

const char* getDecoderErrorMessage(VP8StatusCode statusCode)
{
  switch (statusCode) {
    case VP8_STATUS_OK: return ""; break;
    case VP8_STATUS_OUT_OF_MEMORY: return "out of memory"; break;
    case VP8_STATUS_INVALID_PARAM: return "invalid parameters"; break;
    case VP8_STATUS_BITSTREAM_ERROR: return "bitstream error"; break;
    case VP8_STATUS_UNSUPPORTED_FEATURE: return "unsupported feature"; break;
    case VP8_STATUS_SUSPENDED: return "suspended"; break;
    case VP8_STATUS_USER_ABORT: return "user aborted"; break;
    case VP8_STATUS_NOT_ENOUGH_DATA: return "not enough data"; break;
    default: return "unknown error"; break;
  }
}

bool WebPFormat::onLoad(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* fp = handle.get();

  long len = 0;
  if (fseek(fp, 0, SEEK_END) == 0) {
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
  }

  if (len < 4) {
    fop->setError("The specified file is not a WebP file\n");
    return false;
  }

  std::vector<uint8_t> buf(len);
  uint8_t* data = &buf.front();

  if (!fread(data, sizeof(uint8_t), len, fp)) {
    fop->setError("Error moving the whole WebP file to memory\n");
    return false;
  }

  WebPDecoderConfig config;
  if (!WebPInitDecoderConfig(&config)) {
    fop->setError("WebP decoder cannot load this webp file version\n");
    return false;
  }

  if (WebPGetFeatures(data, len, &config.input) != VP8_STATUS_OK) {
    fop->setError("Bad bitstream in WebP file\n");
    return false;
  }

  fop->sequenceSetHasAlpha(config.input.has_alpha != 0);

  Image* image = fop->sequenceImage(IMAGE_RGB, config.input.width, config.input.height);

  config.output.colorspace = MODE_RGBA;
  config.output.u.RGBA.rgba = (uint8_t*)image->getPixelAddress(0, 0);
  config.output.u.RGBA.stride = config.input.width * sizeof(uint32_t);
  config.output.u.RGBA.size = config.input.width * config.input.height * sizeof(uint32_t);
  config.output.is_external_memory = 1;

  WebPIDecoder* idec = WebPIDecode(NULL, 0, &config);
  if (idec == NULL) {
    fop->setError("Error creating WebP decoder\n");
    return false;
  }

  long bytes_remaining = len;
  long bytes_read = std::max(4l, len/100l);

  while (bytes_remaining > 0) {
    VP8StatusCode status = WebPIAppend(idec, data, bytes_read);
    if (status == VP8_STATUS_OK ||
        status == VP8_STATUS_SUSPENDED) {
      bytes_remaining -= bytes_read;
      data += bytes_read;
      if (bytes_remaining < bytes_read)
        bytes_read = bytes_remaining;
      fop->setProgress(1.0f - ((float)std::max(bytes_remaining, 0l)/(float)len));
    }
    else {
      fop->setError("Error decoding WebP data: %s\n", getDecoderErrorMessage(status));
      WebPIDelete(idec);
      WebPFreeDecBuffer(&config.output);
      return false;
    }
    if (fop->isStop())
      break;
  }

  if (!fop->formatOptions()) {
    base::SharedPtr<WebPOptions> webPOptions(new WebPOptions());
    webPOptions->setLossless(std::min(config.input.format - 1, 1));
    fop->setFormatOptions(webPOptions);
  }

  WebPIDelete(idec);
  WebPFreeDecBuffer(&config.output);
  return true;
}

#ifdef ENABLE_SAVE

struct WriterData {
  FILE* fp;
  FileOp* fop;
};

class ScopedWebPPicture {
public:
  ScopedWebPPicture(WebPPicture& pic) : m_pic(pic) {
  }
  ~ScopedWebPPicture() {
    WebPPictureFree(&m_pic);
  }
private:
  WebPPicture& m_pic;
};

const char* getEncoderErrorMessage(WebPEncodingError errorCode) {
  switch (errorCode) {
  case VP8_ENC_OK: return ""; break;
  case VP8_ENC_ERROR_OUT_OF_MEMORY: return "memory error allocating objects"; break;
  case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY: return "memory error while flushing bits"; break;
  case VP8_ENC_ERROR_NULL_PARAMETER: return "a pointer parameter is NULL"; break;
  case VP8_ENC_ERROR_INVALID_CONFIGURATION: return "configuration is invalid"; break;
  case VP8_ENC_ERROR_BAD_DIMENSION: return "picture has invalid width/height"; break;
  case VP8_ENC_ERROR_PARTITION0_OVERFLOW: return "partition is bigger than 512k"; break;
  case VP8_ENC_ERROR_PARTITION_OVERFLOW: return "partition is bigger than 16M"; break;
  case VP8_ENC_ERROR_BAD_WRITE: return "error while flushing bytes"; break;
  case VP8_ENC_ERROR_FILE_TOO_BIG: return "file is bigger than 4G"; break;
  case VP8_ENC_ERROR_USER_ABORT: return "abort request by user"; break;
  case VP8_ENC_ERROR_LAST: return "abort request by user"; break;
  default: return ""; break;
  }
}

#if WEBP_ENCODER_ABI_VERSION < 0x0203
#define MAX_LEVEL 9
// Mapping between -z level and -m / -q parameter settings.
static const struct {
  uint8_t method_;
  uint8_t quality_;
} kLosslessPresets[MAX_LEVEL + 1] = {
  { 0,  0 }, { 1, 20 }, { 2, 25 }, { 3, 30 }, { 3, 50 },
  { 4, 50 }, { 4, 75 }, { 4, 90 }, { 5, 90 }, { 6, 100 }
};
int WebPConfigLosslessPreset(WebPConfig* config, int level) {
  if (config == NULL || level < 0 || level > MAX_LEVEL) return 0;
  config->lossless = 1;
  config->method = kLosslessPresets[level].method_;
  config->quality = kLosslessPresets[level].quality_;
  return 1;
}
#endif

static int ProgressReport(int percent, const WebPPicture* const pic)
{
  FileOp* fop = ((WriterData*)pic->custom_ptr)->fop;
  fop->setProgress((double)percent/(double)100);
  if (fop->isStop())
    return false;
  else
    return true;
}

static int FileWriter(const uint8_t* data, size_t data_size, const WebPPicture* const pic)
{
  return (data_size ? (fwrite(data, data_size, 1, ((WriterData*)pic->custom_ptr)->fp) == 1) : 1);
}

bool WebPFormat::onSave(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "wb"));
  FILE* fp = handle.get();

  WriterData wd = { fp, fop };

  const Image* image = fop->sequenceImage();
  if (image->width() > WEBP_MAX_DIMENSION ||
      image->height() > WEBP_MAX_DIMENSION) {
    fop->setError("WebP format cannot store %dx%d images. The maximum allowed size is %dx%d\n",
                  image->width(), image->height(),
                  WEBP_MAX_DIMENSION, WEBP_MAX_DIMENSION);
    return false;
  }

  base::SharedPtr<WebPOptions> webp_options = fop->formatOptions();

  WebPConfig config;

  if (webp_options->lossless()) {
    if (!(WebPConfigInit(&config) &&
          WebPConfigLosslessPreset(&config, webp_options->getMethod()))) {
      fop->setError("Error in WebP configuration\n");
      return false;
    }
    config.image_hint = webp_options->getImageHint();
  }
  else {
    if (!WebPConfigPreset(&config, webp_options->getImagePreset(), static_cast<float>(webp_options->getQuality()))) {
      fop->setError("Error in WebP configuration preset\n");
      return false;
    }
  }

  if (!WebPValidateConfig(&config)) {
    fop->setError("Error validating WebP encoder configuration\n");
    return false;
  }

  WebPPicture pic;
  if (!WebPPictureInit(&pic)) {
    fop->setError("Error encoding WebP picture, version mismatch\n");
    return false;
  }

  pic.width = image->width();
  pic.height = image->height();
  if (webp_options->lossless()) {
    pic.use_argb = true;
  }

  if (!WebPPictureAlloc(&pic)) {
    fop->setError("Not enough memory to allocate a WebP picture\n");
    return false;
  }

  ScopedWebPPicture scopedPic(pic); // Calls WebPPictureFree automatically

  if (!WebPPictureImportRGBA(&pic, (uint8_t*)image->getPixelAddress(0, 0), image->width() * sizeof(uint32_t))) {
    fop->setError("Error converting RGBA data into a WebP picture\n");
    return false;
  }

  pic.writer = FileWriter;
  pic.custom_ptr = &wd;
  pic.progress_hook = ProgressReport;

  if (!WebPEncode(&config, &pic)) {
    fop->setError("Error encoding image into WebP: %s\n",
                  getEncoderErrorMessage(pic.error_code));
    return false;
  }

  return true;
}

#endif  // ENABLE_SAVE

// Shows the WebP configuration dialog.
base::SharedPtr<FormatOptions> WebPFormat::onGetFormatOptions(FileOp* fop)
{
  base::SharedPtr<WebPOptions> webp_options;
  if (fop->document()->getFormatOptions())
    webp_options = base::SharedPtr<WebPOptions>(fop->document()->getFormatOptions());

  if (!webp_options)
    webp_options.reset(new WebPOptions);

  // Non-interactive mode
  if (!fop->context() ||
      !fop->context()->isUIAvailable())
    return webp_options;

  try {
    // Configuration parameters
    webp_options->setQuality(get_config_int("WEBP", "Quality", webp_options->getQuality()));
    webp_options->setMethod(get_config_int("WEBP", "Compression", webp_options->getMethod()));
    webp_options->setImageHint(get_config_int("WEBP", "ImageHint", webp_options->getImageHint()));
    webp_options->setImagePreset(get_config_int("WEBP", "ImagePreset", webp_options->getImagePreset()));

    // Load the window to ask to the user the WebP options he wants.

    app::gen::WebpOptions win;
    win.lossless()->setSelected(webp_options->lossless());
    win.lossy()->setSelected(!webp_options->lossless());
    win.quality()->setValue(static_cast<int>(webp_options->getQuality()));
    win.compression()->setValue(webp_options->getMethod());
    win.imageHint()->setSelectedItemIndex(webp_options->getImageHint());
    win.imagePreset()->setSelectedItemIndex(webp_options->getImagePreset());

    win.openWindowInForeground();

    if (win.closer() == win.ok()) {
      webp_options->setQuality(win.quality()->getValue());
      webp_options->setMethod(win.compression()->getValue());
      webp_options->setLossless(win.lossless()->isSelected());
      webp_options->setImageHint(base::convert_to<int>(win.imageHint()->getValue()));
      webp_options->setImagePreset(base::convert_to<int>(win.imagePreset()->getValue()));

      set_config_int("WEBP", "Quality", webp_options->getQuality());
      set_config_int("WEBP", "Compression", webp_options->getMethod());
      set_config_int("WEBP", "ImageHint", webp_options->getImageHint());
      set_config_int("WEBP", "ImagePreset", webp_options->getImagePreset());
    }
    else {
      webp_options.reset(NULL);
    }

    return webp_options;
  }
  catch (std::exception& e) {
    Console::showException(e);
    return base::SharedPtr<WebPOptions>(0);
  }
}

} // namespace app
