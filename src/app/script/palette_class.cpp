// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_palette.h"
#include "app/color.h"
#include "app/file/palette_file.h"
#include "app/res/palettes_loader_delegate.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/security.h"
#include "app/tx.h"
#include "base/fs.h"
#include "doc/palette.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct PaletteObj {
  ObjectId spriteId;
  ObjectId paletteId;

  PaletteObj(Sprite* sprite, Palette* palette)
    : spriteId(sprite ? sprite->id(): 0),
      paletteId(palette ? palette->id(): 0) {
  }

  ~PaletteObj() {
    ASSERT(!paletteId);
  }

  void gc(lua_State* L) {
    if (!spriteId)
      delete this->palette(L);
    paletteId = 0;
  }

  PaletteObj(const PaletteObj&) = delete;
  PaletteObj& operator=(const PaletteObj&) = delete;

  Sprite* sprite(lua_State* L) {
    if (spriteId)
      return check_docobj(L, doc::get<Sprite>(spriteId));
    else
      return nullptr;
  }

  Palette* palette(lua_State* L) {
    return check_docobj(L, doc::get<Palette>(paletteId));
  }
};

int Palette_new(lua_State* L)
{
  if (auto pal2 = may_get_obj<PaletteObj>(L, 1)) {
    push_new<PaletteObj>(L, nullptr, new Palette(*pal2->palette(L)));
  }
  else if (lua_istable(L, 1)) {
    // Palette{ fromFile }
    int type = lua_getfield(L, 1, "fromFile");
    if (type != LUA_TNIL) {
      if (const char* fromFile = lua_tostring(L, -1)) {
        std::string absFn = base::get_absolute_path(fromFile);
        lua_pop(L, 1);

        if (!ask_access(L, absFn.c_str(), FileAccessMode::Read, true))
          return luaL_error(L, "script doesn't have access to open file %s",
                            absFn.c_str());

        Palette* pal = load_palette(absFn.c_str());
        if (pal)
          push_new<PaletteObj>(L, nullptr, pal);
        else
          lua_pushnil(L);
        return 1;
      }
    }
    lua_pop(L, 1);

    // Palette{ fromResource }
    type = lua_getfield(L, 1, "fromResource");
    if (type != LUA_TNIL) {
      if (const char* _id = lua_tostring(L, -1)) {
        std::string id = _id;
        lua_pop(L, 1);

        // TODO Improve this, maybe using an existent set of palettes
        // TODO Centralize the resources management/loading process on-demand (#2059)
        std::map<std::string, std::string> idAndPaths;
        PalettesLoaderDelegate().getResourcesPaths(idAndPaths);

        if (!idAndPaths[id].empty()) {
          std::string absFn = base::get_absolute_path(idAndPaths[id]);

          if (!ask_access(L, absFn.c_str(), FileAccessMode::Read, true))
            return luaL_error(L, "script doesn't have access to open file %s",
                              absFn.c_str());

          Palette* pal = load_palette(absFn.c_str());
          if (pal)
            push_new<PaletteObj>(L, nullptr, pal);
          else
            lua_pushnil(L);
          return 1;
        }
        else {
          return luaL_error(L, "palette resource with ID %s not found", _id);
        }
      }
    }
    lua_pop(L, 1);
  }
  else {
    int ncolors = lua_tointeger(L, 1);
    push_new<PaletteObj>(L, nullptr,
                         new Palette(0, ncolors > 0 ? ncolors: 256));
  }
  return 1;
}

int Palette_gc(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  obj->gc(L);
  obj->~PaletteObj();
  return 0;
}

int Palette_len(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  lua_pushinteger(L, pal->size());
  return 1;
}

int Palette_eq(lua_State* L)
{
  const auto a = get_obj<PaletteObj>(L, 1);
  const auto b = get_obj<PaletteObj>(L, 2);
  lua_pushboolean(L, *a->palette(L) == *b->palette(L));
  return 1;
}

int Palette_resize(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  int ncolors = lua_tointeger(L, 2);
  if (auto sprite = obj->sprite(L)) {
    Palette newPal(*pal);
    newPal.resize(ncolors);

    if (*pal != newPal) {
      Tx tx;
      tx(new cmd::SetPalette(sprite, pal->frame(), &newPal));
      tx.commit();
    }
  }
  else
    pal->resize(ncolors);
  return 1;
}

int Palette_getColor(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  int i = lua_tointeger(L, 2);
  if (i < 0 || i >= int(pal->size()))
    return luaL_error(L, "index out of bounds %d", i);

  doc::color_t docColor = pal->getEntry(i);
  app::Color appColor = app::Color::fromRgb(doc::rgba_getr(docColor),
                                            doc::rgba_getg(docColor),
                                            doc::rgba_getb(docColor),
                                            doc::rgba_geta(docColor));

  push_obj<app::Color>(L, appColor);
  return 1;
}

int Palette_setColor(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  int i = lua_tointeger(L, 2);
  if (i < 0 || i >= int(pal->size()))
    return luaL_error(L, "index out of bounds %d", i);

  doc::color_t docColor = convert_args_into_pixel_color(
    L, 3, doc::IMAGE_RGB);

  if (auto sprite = obj->sprite(L)) {
    // Nothing to do
    if (pal->getEntry(i) == docColor)
      return 0;

    Palette newPal(*pal);
    newPal.setEntry(i, docColor);

    Tx tx;
    tx(new cmd::SetPalette(sprite, pal->frame(), &newPal));
    tx.commit();
  }
  else {
    pal->setEntry(i, docColor);
  }
  return 0;
}

int Palette_get_frame(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  if (auto sprite = obj->sprite(L))
    push_sprite_frame(L, sprite, pal->frame());
  else
    lua_pushnil(L);
  return 1;
}

int Palette_saveAs(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  const char* fn = luaL_checkstring(L, 2);
  if (fn) {
    std::string absFn = base::get_absolute_path(fn);
    if (!ask_access(L, absFn.c_str(), FileAccessMode::Write, true))
      return luaL_error(L, "script doesn't have access to write file %s",
                        absFn.c_str());
    save_palette(absFn.c_str(), pal, pal->size());
  }
  return 0;
}

int Palette_get_frameNumber(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  lua_pushinteger(L, pal->frame()+1);
  return 1;
}

const luaL_Reg Palette_methods[] = {
  { "__gc", Palette_gc },
  { "__len", Palette_len },
  { "__eq", Palette_eq },
  { "resize", Palette_resize },
  { "getColor", Palette_getColor },
  { "setColor", Palette_setColor },
  { "saveAs", Palette_saveAs },
  { nullptr, nullptr }
};

const Property Palette_properties[] = {
  { "frame", Palette_get_frame, nullptr },
  { "frameNumber", Palette_get_frameNumber, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(PaletteObj);
DEF_MTNAME_ALIAS(PaletteObj, Palette);

void register_palette_class(lua_State* L)
{
  using Palette = PaletteObj;
  REG_CLASS(L, Palette);
  REG_CLASS_NEW(L, Palette);
  REG_CLASS_PROPERTIES(L, Palette);
}

void push_sprite_palette(lua_State* L, doc::Sprite* sprite, doc::Palette* palette)
{
  ASSERT(sprite);
  push_new<PaletteObj>(L, sprite, palette);
}

void push_palette(lua_State* L, doc::Palette* palette)
{
  push_new<PaletteObj>(L, nullptr, palette);
}

doc::Palette* get_palette_from_arg(lua_State* L, int index)
{
  auto obj = get_obj<PaletteObj>(L, index);
  return obj->palette(L);
}

} // namespace script
} // namespace app
