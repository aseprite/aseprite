// Aseprite
// Copyright (C) 2022-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/render_text.h"

#include "app/fonts/font_info.h"
#include "app/fonts/fonts.h"
#include "base/fs.h"
#include "doc/blend_funcs.h"
#include "doc/blend_internals.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "os/paint.h"
#include "text/draw_text.h"
#include "text/font_metrics.h"
#include "text/text_blob.h"

#ifdef LAF_SKIA
  #include "app/util/shader_helpers.h"
  #include "os/skia/skia_helpers.h"
  #include "os/skia/skia_surface.h"
#endif

#include <cmath>
#include <memory>
#include <stdexcept>

namespace app {

namespace {

class MeasureHandler : public text::TextBlob::RunHandler {
public:
  MeasureHandler() : m_bounds(0, 0, 1, 1) {}

  const gfx::RectF& bounds() const { return m_bounds; }

  // text::TextBlob::RunHandler impl
  void commitRunBuffer(text::TextBlob::RunInfo& info) override
  {
    ASSERT(info.font);
    if (info.clusters && info.glyphCount > 0) {
      for (int i = 0; i < info.glyphCount; ++i)
        m_bounds |= info.getGlyphBounds(i);
    }
  }

private:
  gfx::RectF m_bounds;
};

} // anonymous namespace

gfx::Size get_text_blob_required_size(const text::TextBlobRef& blob)
{
  ASSERT(blob != nullptr);

  gfx::RectF bounds(0, 0, 1, 1);
  blob->visitRuns([&bounds](text::TextBlob::RunInfo& run) {
    for (int i = 0; i < run.glyphCount; ++i) {
      bounds |= run.getGlyphBounds(i);
    }
  });
  if (bounds.w < 1)
    bounds.w = 1;
  if (bounds.h < 1)
    bounds.h = 1;
  return gfx::Size(std::ceil(bounds.w), std::ceil(bounds.h));
}

doc::ImageRef render_text_blob(const text::TextBlobRef& blob, gfx::Color color)
{
  ASSERT(blob != nullptr);

  os::Paint paint;
  // TODO offer Stroke, StrokeAndFill, and Fill styles
  paint.style(os::Paint::Fill);
  paint.color(color);

  gfx::Size blobSize = get_text_blob_required_size(blob);

  doc::ImageRef image(doc::Image::create(doc::IMAGE_RGB, blobSize.w, blobSize.h));

#ifdef LAF_SKIA
  sk_sp<SkSurface> skSurface = wrap_docimage_in_sksurface(image.get());
  os::SurfaceRef surface = base::make_ref<os::SkiaSurface>(skSurface);
  text::draw_text(surface.get(), blob, gfx::PointF(0, 0), &paint);
#endif // LAF_SKIA

  return image;
}

doc::ImageRef render_text(const FontInfo& fontInfo, const std::string& text, gfx::Color color)
{
  Fonts* fonts = Fonts::instance();
  ASSERT(fonts);
  if (!fonts)
    return nullptr;

  const text::FontRef font = fonts->fontFromInfo(fontInfo);
  if (!font)
    return nullptr;

  const text::FontMgrRef fontMgr = fonts->fontMgr();
  ASSERT(fontMgr);

  os::Paint paint;
  paint.style(os::Paint::StrokeAndFill);
  paint.color(color);

  // We have to measure all text runs which might use different
  // fonts (e.g. if the given font is not enough to shape other code
  // points/languages).
  MeasureHandler handler;
  text::ShaperFeatures features;
  features.ligatures = fontInfo.ligatures();
  text::TextBlobRef blob = text::TextBlob::MakeWithShaper(fontMgr, font, text, &handler, features);
  if (!blob)
    return nullptr;

  gfx::RectF bounds = handler.bounds();
  if (bounds.w < 1)
    bounds.w = 1;
  if (bounds.h < 1)
    bounds.h = 1;

  doc::ImageRef image(doc::Image::create(doc::IMAGE_RGB, bounds.w, bounds.h));

#ifdef LAF_SKIA
  // Wrap the doc::Image into a os::Surface to render the text
  sk_sp<SkSurface> skSurface = wrap_docimage_in_sksurface(image.get());
  os::SurfaceRef surface = base::make_ref<os::SkiaSurface>(skSurface);
  text::draw_text(surface.get(), blob, gfx::PointF(0, 0), &paint);
#endif // LAF_SKIA

  return image;
}

} // namespace app
