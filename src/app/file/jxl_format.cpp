// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "base/buffer.h"
#include "base/file_handle.h"
#include "base/file_size.h"
#include "doc/doc.h"

#include <jxl/codestream_header.h>
#include <jxl/decode.h>
#include <jxl/decode_cxx.h>
#include <jxl/resizable_parallel_runner.h>
#include <jxl/resizable_parallel_runner_cxx.h>
#include <jxl/types.h>

namespace app {

using namespace base;

class JpegXlFormat : public FileFormat {
  const char* onGetName() const override { return "jxl"; }

  void onGetExtensions(base::paths& exts) const override { exts.push_back("jxl"); }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::JPEG_XL_ANIMATION; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_SUPPORT_RGB | FILE_SUPPORT_RGBA |
           FILE_SUPPORT_FRAMES | FILE_ENCODE_ABSTRACT_IMAGE;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateJpegXlFormat()
{
  return new JpegXlFormat;
}

bool JpegXlFormat::onLoad(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* fp = handle.get();

  JxlResizableParallelRunnerPtr runner = JxlResizableParallelRunnerMake(nullptr);
  JxlDecoderPtr dec = JxlDecoderMake(nullptr);

  auto res = JxlDecoderSubscribeEvents(
    dec.get(),
    JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FRAME | JXL_DEC_FULL_IMAGE);

  if (res != JXL_DEC_SUCCESS) {
    fop->setError("JxlDecoderSubscribeEvents failed\n");
    return false;
  }

  res = JxlDecoderSetParallelRunner(dec.get(), JxlResizableParallelRunner, runner.get());
  if (res != JXL_DEC_SUCCESS) {
    fop->setError("JxlDecoderSetParallelRunner failed\n");
    return false;
  }

  base::buffer pixels;
  base::buffer icc_profile;
  size_t w = 0;
  size_t h = 0;

  JxlBasicInfo info;
  JxlPixelFormat format = { 4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };

  size_t size = base::file_size(fp);
  base::buffer jxl(size);
  size = fread(jxl.data(), 1, size, fp);

  JxlDecoderSetInput(dec.get(), jxl.data(), size);
  JxlDecoderCloseInput(dec.get());

  JxlDecoderSetUnpremultiplyAlpha(dec.get(), JXL_TRUE);

  doc::frame_t frames = 0;
  float tick_duration_msecs = 0.001f;

  // Create sprite with one layer
  std::unique_ptr<doc::Sprite> sprite;
  doc::LayerImage* layer = nullptr;

  for (;;) {
    JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

    switch (status) {
      case JXL_DEC_ERROR: fop->setError("Decoder error\n"); return false;

      case JXL_DEC_NEED_MORE_INPUT:
        fop->setError("Error, already provided all input\n");
        return false;

      case JXL_DEC_BASIC_INFO: {
        res = JxlDecoderGetBasicInfo(dec.get(), &info);
        if (res != JXL_DEC_SUCCESS) {
          fop->setError("JxlDecoderGetBasicInfo failed\n");
          return false;
        }
        w = info.xsize;
        h = info.ysize;
        if (!sprite) {
          sprite = std::make_unique<doc::Sprite>(doc::ImageSpec(ColorMode::RGB, w, h), 256);
          layer = new doc::LayerImage(sprite.get());
          sprite->root()->addLayer(layer);
        }

        if (info.animation.tps_numerator > 0 && info.animation.tps_denominator > 0) {
          const float ticks_per_second = (info.animation.tps_numerator /
                                          info.animation.tps_denominator);
          tick_duration_msecs = 1000.0f / ticks_per_second;
        }

        JxlResizableParallelRunnerSetThreads(runner.get(),
                                             JxlResizableParallelRunnerSuggestThreads(w, h));
        break;
      }

      case JXL_DEC_COLOR_ENCODING: {
        // Get the ICC color profile of the pixel data
        size_t icc_size;
        if (JXL_DEC_SUCCESS !=
            JxlDecoderGetICCProfileSize(dec.get(), JXL_COLOR_PROFILE_TARGET_DATA, &icc_size)) {
          fop->setError("JxlDecoderGetICCProfileSize failed\n");
          return false;
        }
        icc_profile.resize(icc_size);
        res = JxlDecoderGetColorAsICCProfile(dec.get(),
                                             JXL_COLOR_PROFILE_TARGET_DATA,
                                             icc_profile.data(),
                                             icc_profile.size());
        if (res != JXL_DEC_SUCCESS) {
          fop->setError("JxlDecoderGetColorAsICCProfile failed\n");
          return false;
        }
        break;
      }

      case JXL_DEC_NEED_IMAGE_OUT_BUFFER: {
        size_t buffer_size;
        res = JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size);
        if (res != JXL_DEC_SUCCESS) {
          fop->setError("JxlDecoderImageOutBufferSize failed\n");
          return false;
        }

        if (buffer_size != w * h * 4) {
          fop->setError("Invalid out buffer size %d %d\n",
                        static_cast<int>(buffer_size),
                        static_cast<int>(w * h * 4));
          return false;
        }

        pixels.resize(buffer_size);

        res = JxlDecoderSetImageOutBuffer(dec.get(), &format, pixels.data(), buffer_size);
        if (res != JXL_DEC_SUCCESS) {
          fop->setError("JxlDecoderSetImageOutBuffer failed\n");
          return false;
        }
        break;
      }

      case JXL_DEC_FRAME: {
        JxlFrameHeader frame;
        res = JxlDecoderGetFrameHeader(dec.get(), &frame);
        if (res == JXL_DEC_SUCCESS) {
          sprite->setTotalFrames(frames + 1);
          sprite->setFrameDuration(frames, frame.duration * tick_duration_msecs);
        }
        break;
      }

      case JXL_DEC_FULL_IMAGE: {
        ImageRef image(Image::create(IMAGE_RGB, w, h));
        Cel* cel = new Cel(frames++, image);
        layer->addCel(cel);

        auto* src = pixels.data();
        for (int y = 0; y < h; ++y) {
          auto* dst = (uint32_t*)image.get()->getPixelAddress(0, y);
          for (int x = 0; x < w; ++x, ++dst, src += 4)
            *dst = doc::rgba(src[0], src[1], src[2], src[3]);
        }
        break;
      }

      case JXL_DEC_SUCCESS:
        // Done.
        goto done;
        break;

      default: fop->setError("Unknown decoder status\n"); return false;
    }

    if (fop->isStop())
      return false;
  }

done:;

  if (!sprite)
    return false;

  // TODO configure layer as background for opaque images
  // layer->configureAsBackground();

  fop->createDocument(sprite.release());
  return true;
}

#ifdef ENABLE_SAVE

bool JpegXlFormat::onSave(FileOp* fop)
{
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* fp = handle.get();

  // TODO

  return false;
}

#endif // ENABLE_SAVE

} // namespace app
