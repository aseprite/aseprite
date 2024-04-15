// Aseprite
// Copyright (C) 2022-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/render_text.h"

#include "app/font_info.h"
#include "app/ui/skin/skin_theme.h"
#include "base/fs.h"
#include "doc/blend_funcs.h"
#include "doc/blend_internals.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "text/draw_text.h"
#include "text/font_metrics.h"

#ifdef LAF_SKIA
  #include "app/util/shader_helpers.h"
  #include "os/skia/skia_surface.h"
#endif

#include <memory>
#include <stdexcept>

namespace app {

doc::ImageRef render_text(
  const text::FontMgrRef& fontMgr,
  const FontInfo& fontInfo,
  const std::string& text,
  gfx::Color color)
{
  doc::ImageRef image;

  auto* theme = skin::SkinTheme::instance();
  ASSERT(theme);
  if (!theme)
    return nullptr;

  text::FontRef font;

  if (fontInfo.type() == FontInfo::Type::System) {
    // Just in case the typeface is not present in the FontInfo
    auto typeface = fontInfo.findTypeface(fontMgr);

    const text::FontMgrRef fontMgr = theme->fontMgr();
    font = fontMgr->makeFont(typeface);
    if (!fontInfo.useDefaultSize())
      font->setSize(fontInfo.size());
  }
  else {
    const int size = (fontInfo.useDefaultSize() ? 18: fontInfo.size());
    font = theme->getFontByName(fontInfo.name(), size);

    if (!font && fontInfo.type() == FontInfo::Type::File) {
      font = fontMgr->loadTrueTypeFont(fontInfo.name().c_str(), size);
    }
  }

  if (!font)
    return nullptr;

  font->setAntialias(fontInfo.antialias());

  os::Paint paint;
  paint.style(os::Paint::StrokeAndFill);
  paint.color(color);

  gfx::RectF bounds;
  bounds.w = font->measureText(text, &bounds, &paint);

  text::FontMetrics metrics;
  bounds.w += 1;
  bounds.h = font->metrics(&metrics);
  if (bounds.w < 1) bounds.w = 1;
  if (bounds.h < 1) bounds.h = 1;

  image.reset(doc::Image::create(doc::IMAGE_RGB, bounds.w, bounds.h));

#ifdef LAF_SKIA
  sk_sp<SkSurface> skSurface = wrap_docimage_in_sksurface(image.get());
  os::SurfaceRef surface = base::make_ref<os::SkiaSurface>(skSurface);
  if (fontInfo.type() == FontInfo::Type::System) {
    text::draw_text_with_shaper(
      surface.get(), fontMgr, font, text, gfx::PointF(0, 0), &paint);
  }
  else {
    text::draw_text(
      surface.get(), fontMgr, font, text,
      color, gfx::ColorNone, 0, 0, nullptr);
  }
#endif // LAF_SKIA

  return image;
}

} // namespace app
