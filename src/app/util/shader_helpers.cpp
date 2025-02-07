// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#if SK_ENABLE_SKSL

  #include "app/util/shader_helpers.h"

  #include "base/exception.h"
  #include "doc/image.h"
  #include "fmt/format.h"

  #include "include/effects/SkRuntimeEffect.h"

namespace app {

sk_sp<SkRuntimeEffect> make_shader(const char* code)
{
  auto result = SkRuntimeEffect::MakeForShader(SkString(code));
  if (!result.errorText.isEmpty()) {
    std::string error = fmt::format("Error compiling shader.\nError: {}\n",
                                    result.errorText.c_str());
    LOG(ERROR, error.c_str());
    std::printf("%s", error.c_str());
    throw base::Exception(error);
  }
  return result.effect;
}

SkImageInfo get_skimageinfo_for_docimage(const doc::Image* img)
{
  switch (img->colorMode()) {
    case doc::ColorMode::RGB:
      return SkImageInfo::Make(img->width(),
                               img->height(),
                               kRGBA_8888_SkColorType,
                               kUnpremul_SkAlphaType);

    case doc::ColorMode::GRAYSCALE:
      // We use kR8G8_unorm_SkColorType to access gray and alpha
      return SkImageInfo::Make(img->width(),
                               img->height(),
                               kR8G8_unorm_SkColorType,
                               kOpaque_SkAlphaType);

    case doc::ColorMode::INDEXED: {
      // We use kAlpha_8_SkColorType to access to the index value through the alpha channel
      return SkImageInfo::Make(img->width(),
                               img->height(),
                               kAlpha_8_SkColorType,
                               kUnpremul_SkAlphaType);
    }
  }
  return SkImageInfo();
}

sk_sp<SkImage> make_skimage_for_docimage(const doc::Image* img)
{
  switch (img->colorMode()) {
    case doc::ColorMode::RGB:
    case doc::ColorMode::GRAYSCALE:
    case doc::ColorMode::INDEXED:   {
      auto skData = SkData::MakeWithoutCopy((const void*)img->getPixelAddress(0, 0),
                                            img->rowBytes() * img->height());

      return SkImage::MakeRasterData(get_skimageinfo_for_docimage(img), skData, img->rowBytes());
    }
  }
  return nullptr;
}

std::unique_ptr<SkCanvas> make_skcanvas_for_docimage(const doc::Image* img)
{
  return SkCanvas::MakeRasterDirect(get_skimageinfo_for_docimage(img),
                                    (void*)img->getPixelAddress(0, 0),
                                    img->rowBytes());
}

} // namespace app

#endif // SK_ENABLE_SKSL
