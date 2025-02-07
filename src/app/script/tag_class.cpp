// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tag_anidir.h"
#include "app/cmd/set_tag_color.h"
#include "app/cmd/set_tag_name.h"
#include "app/cmd/set_tag_range.h"
#include "app/cmd/set_tag_repeat.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
#include "app/tx.h"
#include "doc/sprite.h"
#include "doc/tag.h"

namespace app { namespace script {

using namespace doc;

namespace {

int Tag_eq(lua_State* L)
{
  const auto a = may_get_docobj<Tag>(L, 1);
  const auto b = may_get_docobj<Tag>(L, 2);
  lua_pushboolean(L, (!a && !b) || (a && b && a->id() == b->id()));
  return 1;
}

int Tag_get_sprite(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  push_docobj(L, tag->owner()->sprite());
  return 1;
}

int Tag_get_fromFrame(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  if (tag->owner()->sprite())
    push_sprite_frame(L, tag->owner()->sprite(), tag->fromFrame());
  else
    lua_pushnil(L);
  return 1;
}

int Tag_get_toFrame(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  if (tag->owner()->sprite())
    push_sprite_frame(L, tag->owner()->sprite(), tag->toFrame());
  else
    lua_pushnil(L);
  return 1;
}

int Tag_get_frames(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  lua_pushinteger(L, tag->frames());
  return 1;
}

int Tag_get_name(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  lua_pushstring(L, tag->name().c_str());
  return 1;
}

int Tag_get_aniDir(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  lua_pushinteger(L, (int)tag->aniDir());
  return 1;
}

int Tag_get_repeats(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  lua_pushinteger(L, (int)tag->repeat());
  return 1;
}

int Tag_set_fromFrame(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  const auto fromFrame = get_frame_number_from_arg(L, 2);
  Tx tx(tag->sprite());
  tx(new cmd::SetTagRange(tag, fromFrame, std::max(fromFrame, tag->toFrame())));
  tx.commit();
  return 0;
}

int Tag_set_toFrame(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  const auto toFrame = get_frame_number_from_arg(L, 2);
  Tx tx(tag->sprite());
  tx(new cmd::SetTagRange(tag, std::min(tag->fromFrame(), toFrame), toFrame));
  tx.commit();
  return 0;
}

int Tag_set_name(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  const char* name = lua_tostring(L, 2);
  if (name) {
    Tx tx(tag->sprite());
    tx(new cmd::SetTagName(tag, name));
    tx.commit();
  }
  return 0;
}

int Tag_set_aniDir(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  const int aniDir = lua_tointeger(L, 2);
  Tx tx(tag->sprite());
  tx(new cmd::SetTagAniDir(tag, (doc::AniDir)aniDir));
  tx.commit();
  return 0;
}

int Tag_set_repeats(lua_State* L)
{
  auto tag = get_docobj<Tag>(L, 1);
  const int repeat = lua_tointeger(L, 2);
  Tx tx(tag->sprite());
  tx(new cmd::SetTagRepeat(tag, repeat));
  tx.commit();
  return 0;
}

const luaL_Reg Tag_methods[] = {
  { "__eq",  Tag_eq  },
  { nullptr, nullptr }
};

const Property Tag_properties[] = {
  { "sprite",     Tag_get_sprite,               nullptr                      },
  { "fromFrame",  Tag_get_fromFrame,            Tag_set_fromFrame            },
  { "toFrame",    Tag_get_toFrame,              Tag_set_toFrame              },
  { "frames",     Tag_get_frames,               nullptr                      },
  { "name",       Tag_get_name,                 Tag_set_name                 },
  { "aniDir",     Tag_get_aniDir,               Tag_set_aniDir               },
  { "repeats",    Tag_get_repeats,              Tag_set_repeats              }, // Cannot be "repeat" because it's a Lua keyword
  { "color",      UserData_get_color<Tag>,      UserData_set_color<Tag>      },
  { "data",       UserData_get_text<Tag>,       UserData_set_text<Tag>       },
  { "properties", UserData_get_properties<Tag>, UserData_set_properties<Tag> },
  { nullptr,      nullptr,                      nullptr                      }
};

} // anonymous namespace

DEF_MTNAME(Tag);

void register_tag_class(lua_State* L)
{
  using Tag = doc::Tag;
  REG_CLASS(L, Tag);
  REG_CLASS_PROPERTIES(L, Tag);
}

}} // namespace app::script
