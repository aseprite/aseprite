// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
// Copyright (C) 2015  Gabriel Rauter
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
#include "app/file/webp_options.h"
#include "app/ini_file.h"
#include "app/pref/preferences.h"
#include "base/convert_to.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "ui/manager.h"

#include "webp_options.xml.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <map>

#include <webp/demux.h>
#include <webp/mux.h>

namespace app {

using namespace base;

class WebPFormat : public FileFormat {

  const char* onGetName() const override {
    return "webp";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("webp");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::WEBP_ANIMATION;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_FRAMES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS |
      FILE_ENCODE_ABSTRACT_IMAGE;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
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
  if (fread(&buf[0], 1, buf.size(), fp) != buf.size()) {
    fop->setError("Error moving the whole WebP file to memory\n");
    return false;
  }

  WebPData webp_data;
  WebPDataInit(&webp_data);
  webp_data.bytes = &buf[0];
  webp_data.size = buf.size();

  WebPAnimDecoderOptions dec_options;
  WebPAnimDecoderOptionsInit(&dec_options);
  dec_options.color_mode = MODE_RGBA;

  WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_options);
  if (dec == nullptr) {
    fop->setError("Error parsing WebP image\n");
    return false;
  }

  WebPAnimInfo anim_info;
  if (!WebPAnimDecoderGetInfo(dec, &anim_info)) {
    fop->setError("Error getting global info about the WebP animation\n");
    return false;
  }

  WebPDecoderConfig config;
  WebPInitDecoderConfig(&config);
  if (WebPGetFeatures(webp_data.bytes, webp_data.size, &config.input)) {
    if (!fop->formatOptions()) {
      auto opts = std::make_shared<WebPOptions>();
      WebPOptions::Type type = WebPOptions::Simple;
      switch (config.input.format) {
        case 0: type = WebPOptions::Simple; break;
        case 1: type = WebPOptions::Lossy; break;
        case 2: type = WebPOptions::Lossless; break;
      }
      opts->setType(type);
      fop->setLoadedFormatOptions(opts);
    }
  }
  else {
    config.input.has_alpha = false;
  }

  const int w = anim_info.canvas_width;
  const int h = anim_info.canvas_height;

  Sprite* sprite = new Sprite(ImageSpec(ColorMode::RGB, w, h), 256);
  LayerImage* layer = new LayerImage(sprite);
  sprite->root()->addLayer(layer);
  sprite->setTotalFrames(anim_info.frame_count);

  for (frame_t f=0; f<anim_info.frame_count; ++f) {
    ImageRef image(Image::create(IMAGE_RGB, w, h));
    Cel* cel = new Cel(f, image);
    layer->addCel(cel);
  }

  bool has_alpha = config.input.has_alpha;
  frame_t f = 0;
  int prev_timestamp = 0;
  while (WebPAnimDecoderHasMoreFrames(dec)) {
    uint8_t* frame_rgba;
    int frame_timestamp = 0;
    if (!WebPAnimDecoderGetNext(dec, &frame_rgba, &frame_timestamp)) {
      fop->setError("Error loading WebP frame\n");
      return false;
    }

    Cel* cel = layer->cel(f);
    if (cel) {
      memcpy(cel->image()->getPixelAddress(0, 0),
             frame_rgba, h*w*sizeof(uint32_t));

      if (!has_alpha) {
        const uint32_t* src = (const uint32_t*)frame_rgba;
        const uint32_t* src_end = src + w*h;
        while (src < src_end) {
          const uint8_t alpha = (*src >> 24) & 0xff;
          if (alpha < 255) {
            has_alpha = true;
            break;
          }
          ++src;
        }
      }
    }

    sprite->setFrameDuration(f, frame_timestamp - prev_timestamp);

    prev_timestamp = frame_timestamp;
    fop->setProgress(double(f) / double(anim_info.frame_count));
    if (fop->isStop())
      break;

    ++f;
  }
  WebPAnimDecoderReset(dec);

  if (!has_alpha)
    layer->configureAsBackground();

  WebPAnimDecoderDelete(dec);

  // Don't use WebPDataClear because webp_data use a std::vector<> data.
  //WebPDataClear(&webp_data);

  if (fop->isStop())
    return false;

  fop->createDocument(sprite);
  return true;
}

#ifdef ENABLE_SAVE

struct WriterData {
  FILE* fp;
  FileOp* fop;
  frame_t f = 0;
  frame_t n;
  double progress = 0.0;

  WriterData(FILE* fp, FileOp* fop, frame_t n)
    : fp(fp), fop(fop), n(n) { }
};

static int progress_report(int percent, const WebPPicture* pic)
{
  auto wd = (WriterData*)pic->user_data;
  FileOp* fop = wd->fop;

  double newProgress = (double(wd->f) + double(percent)/100.0) / double(wd->n);
  wd->progress = std::max(wd->progress, newProgress);
  wd->progress = std::clamp(wd->progress, 0.0, 1.0);

  fop->setProgress(wd->progress);
  if (fop->isStop())
    return false;
  else
    return true;
}

bool WebPFormat::onSave(FileOp* fop)
{
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* fp = handle.get();

  const FileAbstractImage* sprite = fop->abstractImage();
  const int w = sprite->width();
  const int h = sprite->height();

  if (w > WEBP_MAX_DIMENSION ||
      h > WEBP_MAX_DIMENSION) {
    fop->setError("WebP format cannot store %dx%d images. The maximum allowed size is %dx%d\n",
                  w, h, WEBP_MAX_DIMENSION, WEBP_MAX_DIMENSION);
    return false;
  }

  auto opts = fop->formatOptionsForSaving<WebPOptions>();
  WebPConfig config;
  WebPConfigInit(&config);

  switch (opts->type()) {

    case WebPOptions::Simple:
    case WebPOptions::Lossless:
      if (!WebPConfigLosslessPreset(&config,
                                    opts->compression())) {
        fop->setError("Error in WebP configuration\n");
        return false;
      }
      config.image_hint = opts->imageHint();
      break;

    case WebPOptions::Lossy:
      if (!WebPConfigPreset(&config,
                            opts->imagePreset(),
                            static_cast<float>(opts->quality()))) {
        fop->setError("Error in WebP configuration preset\n");
        return false;
      }
      break;
  }

  WebPAnimEncoderOptions enc_options;
  WebPAnimEncoderOptionsInit(&enc_options);
  enc_options.anim_params.loop_count =
    (opts->loop() ? 0:  // 0 = infinite
                    1); // 1 = loop once

  ImageRef image(Image::create(IMAGE_RGB, w, h));

  const doc::frame_t totalFrames = fop->roi().frames();
  WriterData wd(fp, fop, totalFrames);
  WebPPicture pic;
  WebPPictureInit(&pic);
  pic.width = w;
  pic.height = h;
  pic.use_argb = true;
  pic.argb = (uint32_t*)image->getPixelAddress(0, 0);
  pic.argb_stride = w;
  pic.user_data = &wd;
  pic.progress_hook = progress_report;

  WebPAnimEncoder* enc = WebPAnimEncoderNew(w, h, &enc_options);
  int timestamp_ms = 0;
  for (frame_t frame : fop->roi().selectedFrames()) {
    // Render the frame in the bitmap
    clear_image(image.get(), image->maskColor());
    sprite->renderFrame(frame, image.get());

    // Switch R <-> B channels because WebPAnimEncoderAssemble()
    // expects MODE_BGRA pictures.
    {
      LockImageBits<RgbTraits> bits(image.get(), Image::ReadWriteLock);
      auto it = bits.begin(), end = bits.end();
      for (; it != end; ++it) {
        auto c = *it;
        *it = rgba(rgba_getb(c), // Use blue in red channel
                   rgba_getg(c),
                   rgba_getr(c), // Use red in blue channel
                   rgba_geta(c));
      }
    }

    if (!WebPAnimEncoderAdd(enc, &pic, timestamp_ms, &config)) {
      if (!fop->isStop()) {
        fop->setError("Error saving frame %d info\n", frame);
        return false;
      }
      else
        return true;
    }
    timestamp_ms += sprite->frameDuration(frame);

    wd.f++;
  }
  WebPAnimEncoderAdd(enc, nullptr, timestamp_ms, nullptr);

  WebPData webp_data;
  WebPDataInit(&webp_data);
  WebPAnimEncoderAssemble(enc, &webp_data);
  WebPAnimEncoderDelete(enc);

  if (fwrite(webp_data.bytes, 1, webp_data.size, fp) != webp_data.size) {
    fop->setError("Error saving content into file\n");
    return false;
  }

  WebPDataClear(&webp_data);
  return true;
}

#endif  // ENABLE_SAVE

// Shows the WebP configuration dialog.
FormatOptionsPtr WebPFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<WebPOptions>();
#ifdef ENABLE_UI
  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();

      if (pref.isSet(pref.webp.loop))
        opts->setLoop(pref.webp.loop());

      if (pref.isSet(pref.webp.type))
        opts->setType(WebPOptions::Type(pref.webp.type()));

      switch (opts->type()) {
        case WebPOptions::Lossless:
          if (pref.isSet(pref.webp.compression)) opts->setCompression(pref.webp.compression());
          if (pref.isSet(pref.webp.imageHint))   opts->setImageHint(WebPImageHint(pref.webp.imageHint()));
          break;
        case WebPOptions::Lossy:
          if (pref.isSet(pref.webp.quality))     opts->setQuality(pref.webp.quality());
          if (pref.isSet(pref.webp.imagePreset)) opts->setImagePreset(WebPPreset(pref.webp.imagePreset()));
          break;
      }

      if (pref.webp.showAlert()) {
        app::gen::WebpOptions win;

        auto updatePanels = [&win, &opts]{
          int o = base::convert_to<int>(win.type()->getValue());
          opts->setType(WebPOptions::Type(o));
          win.losslessOptions()->setVisible(o == int(WebPOptions::Lossless));
          win.lossyOptions()->setVisible(o == int(WebPOptions::Lossy));

          auto rc = win.bounds();
          win.setBounds(
            gfx::Rect(rc.origin(),
                      win.sizeHint()));

          auto manager = win.manager();
          if (manager)
            manager->invalidateRect(rc); // TODO this should be automatic
          // when a window bounds is modified
        };

        win.loop()->setSelected(opts->loop());
        win.type()->setSelectedItemIndex(int(opts->type()));
        win.compression()->setValue(opts->compression());
        win.imageHint()->setSelectedItemIndex(opts->imageHint());
        win.quality()->setValue(static_cast<int>(opts->quality()));
        win.imagePreset()->setSelectedItemIndex(opts->imagePreset());

        updatePanels();
        win.type()->Change.connect(updatePanels);

        win.openWindowInForeground();

        if (win.closer() == win.ok()) {
          pref.webp.loop(win.loop()->isSelected());
          pref.webp.type(base::convert_to<int>(win.type()->getValue()));
          pref.webp.compression(win.compression()->getValue());
          pref.webp.imageHint(base::convert_to<int>(win.imageHint()->getValue()));
          pref.webp.quality(win.quality()->getValue());
          pref.webp.imagePreset(base::convert_to<int>(win.imagePreset()->getValue()));

          opts->setLoop(pref.webp.loop());
          opts->setType(WebPOptions::Type(pref.webp.type()));
          switch (opts->type()) {
            case WebPOptions::Lossless:
              opts->setCompression(pref.webp.compression());
              opts->setImageHint(WebPImageHint(pref.webp.imageHint()));
              break;
            case WebPOptions::Lossy:
              opts->setQuality(pref.webp.quality());
              opts->setImagePreset(WebPPreset(pref.webp.imagePreset()));
              break;
          }
        }
        else {
          opts.reset();
        }
      }
    }
    catch (const std::exception& e) {
      Console::showException(e);
      return std::shared_ptr<WebPOptions>(nullptr);
    }
  }
#endif // ENABLE_UI
  return opts;
}

} // namespace app
