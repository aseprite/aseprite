// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/graphics_context.h"

#include "app/color.h"
#include "app/color_utils.h"
#include "app/modules/palettes.h"
#include "app/script/blend_mode.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/ui/skin/skin_theme.h"
#include "app/util/conversion_to_surface.h"
#include "os/draw_text.h"
#include "os/surface.h"
#include "os/system.h"

#include <algorithm>

#ifdef ENABLE_UI

namespace app {
namespace script {

void GraphicsContext::fillText(const std::string& text, int x, int y)
{
  os::draw_text(m_surface.get(), m_font.get(),
                text, m_paint.color(), 0, x, y, nullptr);
}

gfx::Size GraphicsContext::measureText(const std::string& text) const
{
  return os::draw_text(nullptr, m_font.get(), text,
                       0, 0, 0, 0, nullptr).size();
}

void GraphicsContext::drawImage(const doc::Image* img, int x, int y)
{
  convert_image_to_surface(
    img,
    get_current_palette(),
    m_surface.get(),
    0, 0,
    x, y,
    img->width(), img->height());
}

void GraphicsContext::drawImage(const doc::Image* img,
                                const gfx::Rect& srcRc,
                                const gfx::Rect& dstRc)
{
  if (srcRc.isEmpty() || dstRc.isEmpty())
    return;                     // Do nothing for empty rectangles

  static os::SurfaceRef tmpSurface = nullptr;
  if (!tmpSurface ||
      tmpSurface->width() < srcRc.w ||
      tmpSurface->height() < srcRc.h) {
    tmpSurface = os::instance()->makeRgbaSurface(
      std::max(srcRc.w, (tmpSurface ? tmpSurface->width(): 0)),
      std::max(srcRc.h, (tmpSurface ? tmpSurface->height(): 0)));
  }
  if (tmpSurface) {
    convert_image_to_surface(
      img,
      get_current_palette(),
      tmpSurface.get(),
      srcRc.x, srcRc.y,
      0, 0,
      srcRc.w, srcRc.h);

    m_surface->drawSurface(tmpSurface.get(), gfx::Rect(0, 0, srcRc.w, srcRc.h),
                           dstRc, os::Sampling(), &m_paint);
  }
}

void GraphicsContext::drawThemeImage(const std::string& partId, const gfx::Point& pt)
{
  if (auto theme = skin::SkinTheme::instance()) {
    skin::SkinPartPtr part = (m_uiscale > 1 ? theme->getUnscaledPartById(partId):
                                              theme->getPartById(partId));
    if (part && part->bitmap(0)) {
      auto bmp = part->bitmap(0);
      m_surface->drawRgbaSurface(bmp, pt.x, pt.y);
    }
  }
}

void GraphicsContext::drawThemeRect(const std::string& partId, const gfx::Rect& rc)
{
  if (auto theme = skin::SkinTheme::instance()) {
    skin::SkinPartPtr part = (m_uiscale > 1 ? theme->getUnscaledPartById(partId):
                                              theme->getPartById(partId));
    if (part && part->bitmap(0)) {
      ui::Graphics g(nullptr, m_surface, 0, 0);

      // TODO Copy code from Theme::paintLayer()

      // 9-slices
      if (!part->slicesBounds().isEmpty()) {
        if (m_uiscale > 1)
          theme->drawRectUsingUnscaledSheet(&g, rc, part.get(), true);
        else
          theme->drawRect(&g, rc, part.get(), true);
      }
      else {
        ui::IntersectClip clip(&g, rc);
        if (clip) {
          auto bmp = part->bitmap(0);
          // Horizontal line
          if (rc.w > part->spriteBounds().w) {
            for (int x=rc.x; x<rc.x2(); x+=part->spriteBounds().w) {
              g.drawRgbaSurface(
                bmp,
                x, rc.y+rc.h/2-part->spriteBounds().h/2);
            }
          }
          // Vertical line
          else {
            for (int y=rc.y; y<rc.y2(); y+=part->spriteBounds().h) {
              g.drawRgbaSurface(
                bmp,
                rc.x+rc.w/2-part->spriteBounds().w/2, y);
            }
          }
        }
      }
    }
  }
}

void GraphicsContext::stroke()
{
  m_paint.style(os::Paint::Stroke);
  m_surface->drawPath(m_path, m_paint);
}

void GraphicsContext::fill()
{
  m_paint.style(os::Paint::Fill);
  m_surface->drawPath(m_path, m_paint);
}

namespace {

int GraphicsContext_gc(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->~GraphicsContext();
  return 0;
}

int GraphicsContext_save(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->save();
  return 0;
}

int GraphicsContext_restore(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->restore();
  return 0;
}

int GraphicsContext_clip(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->clip();
  return 0;
}

int GraphicsContext_strokeRect(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const gfx::Rect rc = convert_args_into_rect(L, 2);
  gc->strokeRect(rc);
  return 0;
}

int GraphicsContext_fillRect(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const gfx::Rect rc = convert_args_into_rect(L, 2);
  gc->fillRect(rc);
  return 0;
}

int GraphicsContext_fillText(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  if (const char* text = lua_tostring(L, 2)) {
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    gc->fillText(text, x, y);
  }
  return 0;
}

int GraphicsContext_measureText(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  if (const char* text = lua_tostring(L, 2)) {
    push_obj(L, gc->measureText(text));
    return 1;
  }
  return 0;
}

int GraphicsContext_drawImage(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  if (const doc::Image* img = may_get_image_from_arg(L, 2)) {
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);

    if (lua_gettop(L) >= 9) {
      int w = lua_tointeger(L, 5);
      int h = lua_tointeger(L, 6);
      int dx = lua_tointeger(L, 7);
      int dy = lua_tointeger(L, 8);
      int dw = lua_tointeger(L, 9);
      int dh = lua_tointeger(L, 10);
      gc->drawImage(img, gfx::Rect(x, y, w, h), gfx::Rect(dx, dy, dw, dh));
    }
    else if (lua_gettop(L) >= 3) {
      const auto srcRect = may_get_obj<gfx::Rect>(L, 3);
      const auto dstRect = may_get_obj<gfx::Rect>(L, 4);
      if (srcRect && dstRect)
        gc->drawImage(img, *srcRect, *dstRect);
      else {
        gc->drawImage(img, x, y);
      }
    }
  }
  return 0;
}

int GraphicsContext_theme(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  push_app_theme(L, gc->uiscale());
  return 1;
}

int GraphicsContext_drawThemeImage(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  if (const char* id = lua_tostring(L, 2)) {
    const gfx::Point pt = convert_args_into_point(L, 3);
    gc->drawThemeImage(id, pt);
  }
  return 0;
}

int GraphicsContext_drawThemeRect(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  if (const char* id = lua_tostring(L, 2)) {
    const gfx::Rect rc = convert_args_into_rect(L, 3);
    gc->drawThemeRect(id, rc);
  }
  return 0;
}

int GraphicsContext_beginPath(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->beginPath();
  lua_pushvalue(L, 1);
  return 1;
}

int GraphicsContext_closePath(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->closePath();
  lua_pushvalue(L, 1);
  return 1;
}

int GraphicsContext_moveTo(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  float x = lua_tonumber(L, 2);
  float y = lua_tonumber(L, 3);
  gc->moveTo(x, y);
  lua_pushvalue(L, 1);
  return 1;
}

int GraphicsContext_lineTo(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  float x = lua_tonumber(L, 2);
  float y = lua_tonumber(L, 3);
  gc->lineTo(x, y);
  lua_pushvalue(L, 1);
  return 1;
}

int GraphicsContext_cubicTo(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  float cp1x = lua_tonumber(L, 2);
  float cp1y = lua_tonumber(L, 3);
  float cp2x = lua_tonumber(L, 4);
  float cp2y = lua_tonumber(L, 5);
  float x = lua_tonumber(L, 6);
  float y = lua_tonumber(L, 7);
  gc->cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
  lua_pushvalue(L, 1);
  return 1;
}

int GraphicsContext_oval(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const gfx::Rect rc = convert_args_into_rect(L, 2);
  gc->oval(rc);
  return 0;
}

int GraphicsContext_rect(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const gfx::Rect rc = convert_args_into_rect(L, 2);
  gc->rect(rc);
  return 0;
}

int GraphicsContext_roundedRect(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const gfx::Rect rc = convert_args_into_rect(L, 2);
  const float rx = lua_tonumber(L, 3);
  const float ry = (lua_gettop(L) >= 4 ? lua_tonumber(L, 4): rx);
  gc->roundedRect(rc, rx, ry);
  return 0;
}

int GraphicsContext_stroke(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->stroke();
  lua_pushvalue(L, 1);
  return 1;
}

int GraphicsContext_fill(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->fill();
  lua_pushvalue(L, 1);
  return 1;
}

int GraphicsContext_get_width(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  lua_pushinteger(L, gc->width());
  return 1;
}

int GraphicsContext_get_height(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  lua_pushinteger(L, gc->height());
  return 1;
}

int GraphicsContext_get_antialias(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  lua_pushboolean(L, gc->antialias());
  return 1;
}

int GraphicsContext_set_antialias(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const bool antialias = lua_toboolean(L, 2);
  gc->antialias(antialias);
  return 1;
}

int GraphicsContext_get_color(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  push_obj(L, color_utils::color_from_ui(gc->color()));
  return 1;
}

int GraphicsContext_set_color(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const app::Color color = convert_args_into_color(L, 2);
  gc->color(color_utils::color_for_ui(color));
  return 1;
}

int GraphicsContext_get_strokeWidth(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  lua_pushnumber(L, gc->strokeWidth());
  return 1;
}

int GraphicsContext_set_strokeWidth(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const float strokeWidth = lua_tonumber(L, 2);
  gc->strokeWidth(strokeWidth);
  return 1;
}

int GraphicsContext_get_blendMode(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  lua_pushinteger(
    L, int(base::convert_to<app::script::BlendMode>(gc->blendMode())));
  return 0;
}

int GraphicsContext_set_blendMode(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  gc->blendMode(base::convert_to<os::BlendMode>(
                  app::script::BlendMode(lua_tointeger(L, 2))));
  return 0;
}

int GraphicsContext_get_opacity(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  lua_pushinteger(L, gc->opacity());
  return 1;
}

int GraphicsContext_set_opacity(lua_State* L)
{
  auto gc = get_obj<GraphicsContext>(L, 1);
  const int opacity = lua_tointeger(L, 2);
  gc->opacity(std::clamp(opacity, 0, 255));
  return 0;
}

const luaL_Reg GraphicsContext_methods[] = {
  { "__gc", GraphicsContext_gc },
  { "save", GraphicsContext_save },
  { "restore", GraphicsContext_restore },
  { "clip", GraphicsContext_clip },
  { "strokeRect", GraphicsContext_strokeRect },
  { "fillRect", GraphicsContext_fillRect },
  { "fillText", GraphicsContext_fillText },
  { "measureText", GraphicsContext_measureText },
  { "drawImage", GraphicsContext_drawImage },
  { "drawThemeImage", GraphicsContext_drawThemeImage },
  { "drawThemeRect", GraphicsContext_drawThemeRect },
  { "beginPath", GraphicsContext_beginPath },
  { "closePath", GraphicsContext_closePath },
  { "moveTo", GraphicsContext_moveTo },
  { "lineTo", GraphicsContext_lineTo },
  { "cubicTo", GraphicsContext_cubicTo },
  { "oval", GraphicsContext_oval },
  { "rect", GraphicsContext_rect },
  { "roundedRect", GraphicsContext_roundedRect },
  { "stroke", GraphicsContext_stroke },
  { "fill", GraphicsContext_fill },
  { nullptr, nullptr }
};

const Property GraphicsContext_properties[] = {
  { "width", GraphicsContext_get_width, nullptr },
  { "height", GraphicsContext_get_height, nullptr },
  { "theme", GraphicsContext_theme, nullptr },
  { "antialias", GraphicsContext_get_antialias, GraphicsContext_set_antialias },
  { "color", GraphicsContext_get_color, GraphicsContext_set_color },
  { "strokeWidth", GraphicsContext_get_strokeWidth, GraphicsContext_set_strokeWidth },
  { "blendMode", GraphicsContext_get_blendMode, GraphicsContext_set_blendMode },
  { "opacity", GraphicsContext_get_opacity, GraphicsContext_set_opacity },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(GraphicsContext);

void register_graphics_context_class(lua_State* L)
{
  REG_CLASS(L, GraphicsContext);
  REG_CLASS_PROPERTIES(L, GraphicsContext);
}

} // namespace script
} // namespace app

#endif // ENABLE_UI
