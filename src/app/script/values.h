// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_VALUES_H_INCLUDED
#define APP_SCRIPT_VALUES_H_INCLUDED
#pragma once

#ifdef ENABLE_SCRIPTING

extern "C" struct lua_State;

namespace app {
namespace script {

template<typename T>
T get_value_from_lua(lua_State* L, int index);

template<typename T>
void push_value_to_lua(lua_State* L, const T& value);

} // namespace script
} // namespace app

#endif  // ENABLE_SCRIPTING

#endif
