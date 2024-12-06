// Aseprite
// Copyright (c) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/modules/palettes.h"
#include "app/script/luacpp.h"
#include "app/util/clipboard.h"
#include "clip/clip.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/tileset.h"
#include "engine.h"

#include "docobj.h"
#include "security.h"

namespace app { namespace script {

namespace {

struct Clipboard {};

#define CLIPBOARD_GATE(mode)                                                                       \
  if (!ask_access(L, nullptr, mode, ResourceType::Clipboard))                                      \
    return luaL_error(L,                                                                           \
                      "the script doesn't have %s access to the clipboard",                        \
                      (mode) == FileAccessMode::Read ? "read" : "write");

int Clipboard_clear(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  if (!clip::clear())
    return luaL_error(L, "failed to clear the clipboard");

  return 0;
}

int Clipboard_hasText(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  lua_pushboolean(L, clip::has(clip::text_format()));
  return 1;
}

int Clipboard_hasImage(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  lua_pushboolean(L, clip::has(clip::image_format()));
  return 1;
}

int Clipboard_get_image(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  doc::Image* image = nullptr;
  doc::Mask* mask = nullptr;
  doc::Palette* palette = nullptr;
  doc::Tileset* tileset = nullptr;
  const bool result =
    app::Clipboard::instance()->getNativeBitmap(&image, &mask, &palette, &tileset);

  if (image == nullptr) {
    lua_pushnil(L);
    return 1;
  }

  if (!result)
    return luaL_error(L, "failed to get image from clipboard");

  push_image(L, image);
  return 1;
}

int Clipboard_get_text(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  std::string str;
  if (clip::get_text(str))
    lua_pushstring(L, str.c_str());
  else
    lua_pushnil(L);

  return 1;
}

int Clipboard_set_image(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  auto* image = may_get_image_from_arg(L, 2);
  if (!image)
    return luaL_error(L, "invalid image");

  const bool result = app::Clipboard::instance()->setNativeBitmap(image,
                                                                  nullptr,
                                                                  get_current_palette(),
                                                                  nullptr,
                                                                  image->maskColor());

  if (!result)
    return luaL_error(L, "failed to set image to clipboard");

  return 0;
}

int Clipboard_set_text(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  const char* text = lua_tostring(L, 2);
  if (!clip::set_text(text ? text : ""))
    return luaL_error(L, "failed to set the clipboard text to '%s'", text);

  return 0;
}

int Clipboard_get_content(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  doc::Image* image = nullptr;
  doc::Mask* mask = nullptr;
  doc::Palette* palette = nullptr;
  doc::Tileset* tileset = nullptr;
  const bool bitmapResult =
    app::Clipboard::instance()->getNativeBitmap(&image, &mask, &palette, &tileset);

  std::string text;
  const bool clipResult = !bitmapResult ? clip::get_text(text) : false;

  lua_createtable(L, 0, 5);

  if (bitmapResult && image)
    push_image(L, image);
  else
    lua_pushnil(L);
  lua_setfield(L, -2, "image");

  if (bitmapResult && mask)
    push_docobj<Mask>(L, mask);
  else
    lua_pushnil(L);
  lua_setfield(L, -2, "selection");

  if (bitmapResult && palette)
    push_palette(L, palette);
  else
    lua_pushnil(L);
  lua_setfield(L, -2, "palette");

  if (bitmapResult && tileset)
    push_docobj<Tileset>(L, tileset);
  else
    lua_pushnil(L);
  lua_setfield(L, -2, "tileset");

  if (clipResult)
    lua_pushstring(L, text.c_str());
  else
    lua_pushnil(L);
  lua_setfield(L, -2, "text");

  return 1;
}

int Clipboard_set_content(lua_State* L)
{
  CLIPBOARD_GATE(FileAccessMode::Read);

  doc::Image* image = nullptr;
  doc::Mask* mask = nullptr;
  doc::Palette* palette = nullptr;
  doc::Tileset* tileset = nullptr;
  std::optional<std::string> text = std::nullopt;

  if (!lua_istable(L, 2))
    return luaL_error(L, "clipboard content must be a table");

  int type = lua_getfield(L, 2, "image");
  if (type != LUA_TNIL) {
    image = may_get_image_from_arg(L, -1);
    if (!image)
      return luaL_error(L, "invalid image provided");
  }
  lua_pop(L, 1);

  type = lua_getfield(L, 2, "selection");
  if (type != LUA_TNIL) {
    mask = get_mask_from_arg(L, -1);
    if (!mask)
      return luaL_error(L, "invalid selection provided");
  }
  lua_pop(L, 1);

  type = lua_getfield(L, 2, "palette");
  if (type != LUA_TNIL) {
    palette = may_get_docobj<Palette>(L, -1);
    if (!palette)
      return luaL_error(L, "invalid palette provided");
  }
  lua_pop(L, 1);

  type = lua_getfield(L, 2, "tileset");
  if (type != LUA_TNIL) {
    tileset = may_get_docobj<Tileset>(L, -1);
    if (!tileset)
      return luaL_error(L, "invalid tileset provided");
  }
  lua_pop(L, 1);

  type = lua_getfield(L, 2, "text");
  if (type != LUA_TNIL) {
    const char* tableText = lua_tostring(L, -1);
    if (tableText != nullptr && strlen(tableText) > 0)
      text = std::string(tableText);
    else
      return luaL_error(L, "invalid text provided");
  }
  lua_pop(L, 1);

  if (!image && !mask && !palette && !tileset && !text.has_value()) {
    // If we aren't provided anything valid in the table, clear the clipboard.
    return Clipboard_clear(L);
  }

  if (image != nullptr && text.has_value())
    return luaL_error(L, "can't set both image and text at the same time");

  if (image &&
      !app::Clipboard::instance()->setNativeBitmap(image,
                                                   mask,
                                                   palette ? palette : get_current_palette(),
                                                   tileset,
                                                   image ? image->maskColor() : -1))
    return luaL_error(L, "failed to set data to clipboard");

  if (text.has_value() && !clip::set_text(*text))
    return luaL_error(L, "failed to set the clipboard text to '%s'", (*text).c_str());

  return 0;
}

const luaL_Reg Clipboard_methods[] = {
  { "clear", Clipboard_clear },
  { nullptr, nullptr         }
};

const Property Clipboard_properties[] = {
  { "hasText",  Clipboard_hasText,     nullptr               },
  { "hasImage", Clipboard_hasImage,    nullptr               },
  { "text",     Clipboard_get_text,    Clipboard_set_text    },
  { "image",    Clipboard_get_image,   Clipboard_set_image   },
  { "content",  Clipboard_get_content, Clipboard_set_content },
  { nullptr,    nullptr,               nullptr               }
};

} // anonymous namespace

DEF_MTNAME(Clipboard);

void register_clipboard_classes(lua_State* L)
{
  REG_CLASS(L, Clipboard);
  REG_CLASS_PROPERTIES(L, Clipboard);
}

void push_app_clipboard(lua_State* L)
{
  push_new<Clipboard>(L);
}

}} // namespace app::script
