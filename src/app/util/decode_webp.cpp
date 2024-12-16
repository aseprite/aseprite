// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/decode_webp.h"

#include "os/system.h"

#include <webp/demux.h>
#include <webp/mux.h>

namespace app { namespace util {

static WEBP_CSP_MODE colorMode()
{
  auto surface = os::instance()->makeSurface(1, 1);
  os::SurfaceFormatData fd;
  surface->getFormat(&fd);
  return (fd.redShift == 0 ? MODE_RGBA : MODE_BGRA);
}

os::SurfaceRef decode_webp(const uint8_t* buf, uint32_t len)
{
  // I've considered using the FileFormatsManager here but we don't have a file
  // in this case just the bytes. At some point maye we should refactor this or
  // the WepFormat class to avoid duplicated logic.
  WebPData webp_data;
  WebPDataInit(&webp_data);
  webp_data.bytes = &buf[0];
  webp_data.size = len;

  WebPAnimDecoderOptions dec_options;
  WebPAnimDecoderOptionsInit(&dec_options);
  dec_options.color_mode = colorMode();

  WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_options);
  if (dec == nullptr) {
    // Error parsing WebP image
    return nullptr;
  }

  WebPAnimInfo anim_info;
  if (!WebPAnimDecoderGetInfo(dec, &anim_info)) {
    // Error getting global info about the WebP animation
    return nullptr;
  }

  WebPDecoderConfig config;
  WebPInitDecoderConfig(&config);
  auto status = WebPGetFeatures(webp_data.bytes, webp_data.size, &config.input);
  if (status != VP8_STATUS_OK) {
    // Error getting WebP features.
    return nullptr;
  }

  const int w = anim_info.canvas_width;
  const int h = anim_info.canvas_height;

  if (anim_info.frame_count <= 0)
    return nullptr;

  auto surface = os::instance()->makeSurface(w, h);

  // We just want the first frame, so we don't iterate.
  WebPAnimDecoderHasMoreFrames(dec);

  uint8_t* frame_rgba;
  int frame_timestamp = 0;
  if (!WebPAnimDecoderGetNext(dec, &frame_rgba, &frame_timestamp)) {
    // Error loading WebP frame
    return nullptr;
  }

  const uint32_t* src = (const uint32_t*)frame_rgba;
  for (int y = 0; y < h; ++y, src += w) {
    memcpy(surface->getData(0, y), src, w * sizeof(uint32_t));
  }

  WebPAnimDecoderReset(dec);

  WebPAnimDecoderDelete(dec);

  return surface;
}

}} // namespace app::util
