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
#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/resizable_parallel_runner.h>
#include <jxl/resizable_parallel_runner_cxx.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/thread_parallel_runner_cxx.h>
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
           FILE_SUPPORT_GRAY | FILE_SUPPORT_GRAYA | FILE_SUPPORT_FRAMES |
           FILE_ENCODE_ABSTRACT_IMAGE;
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
  auto colorMode = ColorMode::RGB;
  bool is_opaque = false;
  bool is_gray = false;

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
        is_opaque = (info.num_extra_channels == 0);
        is_gray = (info.num_color_channels < 3);
        colorMode = (is_gray ? ColorMode::GRAYSCALE : ColorMode::RGB);
        format.num_channels = (is_gray ? 1 : 3) + (is_opaque ? 0 : 1);

        if (!sprite) {
          sprite = std::make_unique<doc::Sprite>(doc::ImageSpec(colorMode, w, h), 256);
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

        if (buffer_size != w * h * format.num_channels) {
          fop->setError("Invalid out buffer size %d %d\n",
                        static_cast<int>(buffer_size),
                        static_cast<int>(w * h * format.num_channels));
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
        if (!sprite || !layer) {
          fop->setError("Invalid decoder state, full image before basic info\n");
          return false;
        }

        doc::ImageRef image(doc::Image::create((PixelFormat)colorMode, w, h));
        doc::Cel* cel = new doc::Cel(frames++, image);
        layer->addCel(cel);

        const auto* src = pixels.data();
        switch (colorMode) {
          case doc::ColorMode::RGB:
            for (int y = 0; y < h; ++y) {
              auto* dst = (uint32_t*)image.get()->getPixelAddress(0, y);
              if (is_opaque) {
                for (int x = 0; x < w; ++x, ++dst, src += 3)
                  *dst = doc::rgba(src[0], src[1], src[2], 255);
              }
              else {
                for (int x = 0; x < w; ++x, ++dst, src += 4)
                  *dst = doc::rgba(src[0], src[1], src[2], src[3]);
              }
            }
            break;

          case ColorMode::GRAYSCALE:
            for (int y = 0; y < h; ++y) {
              auto* dst = (uint16_t*)image.get()->getPixelAddress(0, y);
              if (is_opaque) {
                for (int x = 0; x < w; ++x, ++dst, ++src)
                  *dst = doc::graya(*src, 255);
              }
              else {
                for (int x = 0; x < w; ++x, ++dst, src += 2)
                  *dst = doc::graya(src[0], src[1]);
              }
            }
            break;
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

  // Configure layer as background for opaque images
  if (layer && is_opaque)
    layer->configureAsBackground();

  fop->createDocument(sprite.release());
  return true;
}

#ifdef ENABLE_SAVE

bool JpegXlFormat::onSave(FileOp* fop)
{
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* fp = handle.get();

  const FileAbstractImage* sprite = fop->abstractImageToSave();
  const size_t w = sprite->width();
  const size_t h = sprite->height();
  const auto colorMode = sprite->spec().colorMode();
  const bool is_opaque = sprite->isOpaque();
  const bool is_gray = (colorMode == ColorMode::GRAYSCALE);

  JxlEncoderPtr enc = JxlEncoderMake(nullptr);
  JxlThreadParallelRunnerPtr runner =
    JxlThreadParallelRunnerMake(nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());

  auto res = JxlEncoderSetParallelRunner(enc.get(), JxlThreadParallelRunner, runner.get());
  if (res != JXL_ENC_SUCCESS) {
    fop->setError("JxlEncoderSetParallelRunner failed\n");
    return false;
  }

  JxlPixelFormat format = { 0, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };
  format.num_channels = (is_gray ? 1 : 3) + (is_opaque ? 0 : 1);

  JxlBasicInfo info;
  JxlEncoderInitBasicInfo(&info);
  info.xsize = w;
  info.ysize = h;
  info.bits_per_sample = 8;
  info.exponent_bits_per_sample = 0;
  info.num_color_channels = (is_gray ? 1 : 3);
  info.num_extra_channels = (is_opaque ? 0 : 1);
  info.alpha_bits = (is_opaque ? 0 : 8);
  info.alpha_exponent_bits = 0;
  info.alpha_premultiplied = JXL_FALSE;
  info.exponent_bits_per_sample = 0;
  info.uses_original_profile = JXL_TRUE;
  info.have_animation = JXL_TRUE; // Always true to save the frame duration
  info.animation.tps_numerator = 1000;
  info.animation.tps_denominator = 1;

  res = JxlEncoderSetBasicInfo(enc.get(), &info);
  if (res != JXL_ENC_SUCCESS) {
    fop->setError("JxlEncoderSetBasicInfo failed\n");
    return false;
  }

  JxlColorEncoding color_encoding = {};
  JxlColorEncodingSetToSRGB(&color_encoding, is_gray ? JXL_TRUE : JXL_FALSE);
  res = JxlEncoderSetColorEncoding(enc.get(), &color_encoding);
  // TODO Use JxlEncoderSetICCProfile(enc.get(), const uint8_t* icc_profile, size_t size);
  if (res != JXL_ENC_SUCCESS) {
    fop->setError("JxlEncoderSetColorEncoding failed\n");
    return false;
  }

  JxlEncoderFrameSettings* frame_settings = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);
  if (!frame_settings) {
    fop->setError("JxlEncoderFrameSettingsCreate failed\n");
    return false;
  }
  JxlEncoderSetFrameLossless(frame_settings, JXL_TRUE);
  if (!is_opaque)
    JxlEncoderFrameSettingsSetOption(frame_settings, JXL_ENC_FRAME_SETTING_STRIP_ALPHA, JXL_FALSE);

  JxlFrameHeader frame_header;
  JxlEncoderInitFrameHeader(&frame_header);

  ImageRef image(Image::create((PixelFormat)colorMode, w, h));

  base::buffer pixels;
  size_t buffer_size = w * h * format.num_channels;
  pixels.resize(buffer_size);

  for (frame_t frame : fop->roi().framesSequence()) {
    clear_image(image.get(), image->maskColor());
    sprite->renderFrame(frame, fop->roi().frameBounds(frame), image.get());

    frame_header.is_last = (frame == fop->roi().toFrame() ? JXL_TRUE : JXL_FALSE);
    frame_header.duration = sprite->frameDuration(frame);

    JxlEncoderSetFrameHeader(frame_settings, &frame_header);

    auto* dst = pixels.data();
    switch (colorMode) {
      case doc::ColorMode::RGB:
        for (int y = 0; y < h; ++y) {
          auto* src = (uint32_t*)image.get()->getPixelAddress(0, y);

          if (is_opaque) {
            for (int x = 0; x < w; ++x, ++src) {
              *(dst++) = doc::rgba_getr(*src);
              *(dst++) = doc::rgba_getg(*src);
              *(dst++) = doc::rgba_getb(*src);
            }
          }
          else {
            for (int x = 0; x < w; ++x, ++src) {
              *(dst++) = doc::rgba_getr(*src);
              *(dst++) = doc::rgba_getg(*src);
              *(dst++) = doc::rgba_getb(*src);
              *(dst++) = doc::rgba_geta(*src);
            }
          }
        }
        break;

      case ColorMode::GRAYSCALE:
        for (int y = 0; y < h; ++y) {
          auto* src = (uint16_t*)image.get()->getPixelAddress(0, y);

          if (is_opaque) {
            for (int x = 0; x < w; ++x, ++src) {
              *(dst++) = doc::graya_getv(*src);
            }
          }
          else {
            for (int x = 0; x < w; ++x, ++src) {
              *(dst++) = doc::graya_getv(*src);
              *(dst++) = doc::graya_geta(*src);
            }
          }
        }
        break;
    }

    res = JxlEncoderAddImageFrame(frame_settings,
                                  &format,
                                  static_cast<const void*>(pixels.data()),
                                  pixels.size());
    if (res != JXL_ENC_SUCCESS) {
      fop->setError("JxlEncoderAddImageFrame failed. Error=%d\n", res);
      return false;
    }
  }
  JxlEncoderCloseInput(enc.get());

  base::buffer jxl(64);
  uint8_t* next_out = jxl.data();
  size_t avail_out = jxl.size() - (next_out - jxl.data());
  JxlEncoderStatus process_result = JXL_ENC_NEED_MORE_OUTPUT;
  while (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
    process_result = JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);
    if (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
      size_t offset = next_out - jxl.data();
      jxl.resize(jxl.size() * 2);
      next_out = jxl.data() + offset;
      avail_out = jxl.size() - offset;
    }
  }
  if (process_result != JXL_ENC_SUCCESS) {
    fop->setError("JxlEncoderProcessOutput failed\n");
    return false;
  }
  jxl.resize(next_out - jxl.data());

  size_t size = fwrite(jxl.data(), 1, jxl.size(), fp);
  if (size != jxl.size()) {
    fop->setError("Error saving JPEG XL compressed data\n");
    return false;
  }

  return true;
}

#endif // ENABLE_SAVE

} // namespace app
