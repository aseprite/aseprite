// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "base/string.h"
#include "os/paint.h"

#ifdef ENABLE_UI

namespace app {
namespace script {

using Paint = os::Paint;

namespace {

static std::map<std::string, os::BlendMode> stringToBlendMode = {
  { "clear",      os::BlendMode::Clear },
  { "src",        os::BlendMode::Src },
  { "dst",        os::BlendMode::Dst },
  { "srcover",    os::BlendMode::SrcOver },
  { "dstover",    os::BlendMode::DstOver },
  { "srcin",      os::BlendMode::SrcIn },
  { "dstin",      os::BlendMode::DstIn },
  { "srcout",     os::BlendMode::SrcOut },
  { "dstout",     os::BlendMode::DstOut },
  { "srcatop",    os::BlendMode::SrcATop },
  { "dstatop",    os::BlendMode::DstATop },
  { "xor",        os::BlendMode::Xor },
  { "plus",       os::BlendMode::Plus },
  { "modulate",   os::BlendMode::Modulate },
  { "screen",     os::BlendMode::Screen },
  { "overlay",    os::BlendMode::Overlay },
  { "darken",     os::BlendMode::Darken },
  { "lighten",    os::BlendMode::Lighten },
  { "colordodge", os::BlendMode::ColorDodge },
  { "colorburn",  os::BlendMode::ColorBurn },
  { "hardlight",  os::BlendMode::HardLight },
  { "softlight",  os::BlendMode::SoftLight },
  { "difference", os::BlendMode::Difference },
  { "exclusion",  os::BlendMode::Exclusion },
  { "multiply",   os::BlendMode::Multiply },
  { "hue",        os::BlendMode::Hue },
  { "saturation", os::BlendMode::Saturation },
  { "color",      os::BlendMode::Color },
  { "luminosity", os::BlendMode::Luminosity },
};

int Paint_gc(lua_State* L)
{
  auto p = get_obj<Paint>(L, 1);
  p->~Paint();
  return 0;
}

int Paint_new(lua_State* L)
{
  push_new<Paint>(L);
  return 1;
}

int Paint_get_blendMode(lua_State* L)
{
  auto p = get_obj<Paint>(L, 1);
  auto bm = p->blendMode();
  for (const auto& kv : stringToBlendMode) {
    if (kv.second == bm) {
      lua_pushstring(L, kv.first.c_str());
      return 1;
    }
  }
  return 0;
}

int Paint_set_blendMode(lua_State* L)
{
  auto p = get_obj<Paint>(L, 1);
  if (auto str = lua_tostring(L, 2))
    p->blendMode(stringToBlendMode[base::string_to_lower(str)]);
  return 0;
}

int Paint_get_alpha(lua_State* L)
{
  auto p = get_obj<Paint>(L, 1);
  lua_pushnumber(L, p->skPaint().getAlphaf());
  return 1;
}

int Paint_set_alpha(lua_State* L)
{
  auto p = get_obj<Paint>(L, 1);
  const float alpha = lua_tonumber(L, 2);
  p->skPaint().setAlphaf(alpha);
  return 0;
}

const luaL_Reg Paint_methods[] = {
  { "__gc", Paint_gc },
  { nullptr, nullptr }
};

const Property Paint_properties[] = {
  { "blendMode", Paint_get_blendMode, Paint_set_blendMode },
  { "alpha", Paint_get_alpha, Paint_set_alpha },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Paint);

void register_paint_class(lua_State* L)
{
  REG_CLASS(L, Paint);
  REG_CLASS_NEW(L, Paint);
  REG_CLASS_PROPERTIES(L, Paint);
}

} // namespace script
} // namespace app

#endif // ENABLE_UI
