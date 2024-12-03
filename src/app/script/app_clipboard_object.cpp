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
#include "engine.h"

namespace app { namespace script {

namespace {

struct Clipboard {};

int Clipboard_clear(lua_State* L)
{
  if (!clip::clear())
    return luaL_error(L, "failed to clear the clipboard");

  return 1;
}

int Clipboard_is_text(lua_State* L)
{
  lua_pushboolean(L, clip::has(clip::text_format()));
  return 1;
}

int Clipboard_is_image(lua_State* L)
{
  lua_pushboolean(L, clip::has(clip::image_format()));
  return 1;
}

int Clipboard_is_empty(lua_State* L)
{
  // Using clip::has(clip::empty_format()) had inconsistent results, might as well avoid false
  // positives by just checking the two formats we support
  lua_pushboolean(L, !clip::has(clip::image_format()) && !clip::has(clip::text_format()));
  return 1;
}

int Clipboard_get_image(lua_State* L)
{
  doc::Image* image = nullptr;
  doc::Mask* mask = nullptr;
  doc::Palette* palette = nullptr;
  doc::Tileset* tileset = nullptr;
  const bool result =
    app::Clipboard::instance()->getNativeBitmap(&image, &mask, &palette, &tileset);

  // TODO: If we get a tileset, should we convert it to an image?
  if (image == nullptr || !result)
    return luaL_error(L, "failed to get image from clipboard");

  push_image(L, image);
  return 1;
}

int Clipboard_get_text(lua_State* L)
{
  std::string str;
  if (!clip::get_text(str))
    return luaL_error(L, "failed to get text from clipboard");

  lua_pushstring(L, str.c_str());
  return 1;
}

int Clipboard_set_image(lua_State* L)
{
  auto* image = may_get_image_from_arg(L, 2);
  if (!image)
    return luaL_error(L, "invalid image");

  const bool result = app::Clipboard::instance()->setNativeBitmap(
    image,
    nullptr,
    get_current_palette(), // TODO: Not sure if there's any way to get the palette from the image
    nullptr,
    image->maskColor() // TODO: Unsure if this is sufficient.
  );

  if (!result)
    return luaL_error(L, "failed to set image to clipboard");

  return 0;
}

int Clipboard_set_text(lua_State* L)
{
  const char* text = lua_tostring(L, 2);
  if (text != NULL && strlen(text) > 0 && !clip::set_text(text))
    return luaL_error(L, "failed to set the clipboard text to '%s'", text);

  return 0;
}

const luaL_Reg Clipboard_methods[] = {
  { "clear", Clipboard_clear },
  { nullptr, nullptr         }
};

const Property Clipboard_properties[] = {
  { "isText",  Clipboard_is_text,   nullptr             },
  { "isImage", Clipboard_is_image,  nullptr             },
  { "isEmpty", Clipboard_is_empty,  nullptr             },
  { "text",    Clipboard_get_text,  Clipboard_set_text  },
  { "image",   Clipboard_get_image, Clipboard_set_image },
  { nullptr,   nullptr,             nullptr             }
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
