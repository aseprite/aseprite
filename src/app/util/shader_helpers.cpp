// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/shader_helpers.h"

#include "base/exception.h"
#include "doc/image.h"
#include "fmt/format.h"

#if LAF_SKIA

  #include "os/skia/skia_surface.h"

  #include "include/core/SkSurface.h"
  #if SK_ENABLE_SKSL
    #include "src/core/SkRuntimeEffectPriv.h"
  #endif

namespace app {

  #if SK_ENABLE_SKSL

sk_sp<SkRuntimeEffect> make_shader(const char* code)
{
  SkRuntimeEffect::Options options;

  // Allow usage of private functions like $hsl_to_rgb without a SkSL
  // compilation error at runtime.
  SkRuntimeEffectPriv::AllowPrivateAccess(&options);

  auto result = SkRuntimeEffect::MakeForShader(SkString(code), options);
  if (!result.errorText.isEmpty()) {
    std::string error = fmt::format("Error compiling shader.\nError: {}\n",
                                    result.errorText.c_str());
    LOG(ERROR, error.c_str());
    std::printf("%s", error.c_str());
    throw base::Exception(error);
  }
  return result.effect;
}

sk_sp<SkRuntimeEffect> make_blender(const char* code)
{
  SkRuntimeEffect::Options options;

  // Allow usage of private functions like $hsl_to_rgb without a SkSL
  // compilation error at runtime.
  SkRuntimeEffectPriv::AllowPrivateAccess(&options);

  auto result = SkRuntimeEffect::MakeForBlender(SkString(code), options);
  if (!result.errorText.isEmpty()) {
    std::string error = fmt::format("Error compiling blender.\nError: {}\n",
                                    result.errorText.c_str());
    LOG(ERROR, error.c_str());
    std::printf("%s", error.c_str());
    throw base::Exception(error);
  }
  return result.effect;
}

  #endif // SK_ENABLE_SKSL

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

      return SkImages::RasterFromData(get_skimageinfo_for_docimage(img), skData, img->rowBytes());
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

sk_sp<SkSurface> wrap_docimage_in_sksurface(const doc::Image* img)
{
  return SkSurfaces::WrapPixels(get_skimageinfo_for_docimage(img),
                                (void*)img->getPixelAddress(0, 0),
                                img->rowBytes());
}

os::SurfaceRef wrap_docimage_in_surface(const doc::Image* img)
{
  return os::SurfaceRef(new os::SkiaSurface(wrap_docimage_in_sksurface(img)));
}

} // namespace app

#endif // LAF_SKIA
