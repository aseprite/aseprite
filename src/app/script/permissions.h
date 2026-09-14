// Aseprite
// Copyright (C) 2021-present  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_SECURITY_H_INCLUDED
#define APP_SCRIPT_SECURITY_H_INCLUDED
#pragma once

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "base/debug.h"

#include <string_view>

namespace app::script {

enum class Permission : uint8_t {
  Unknown,
  Network,
  SpriteRead,
  SpriteWrite,
  IORead,
  IOWrite,
  ClipboardRead,
  ClipboardWrite,
  TemporaryFile,
  Preferences,
  Debug,
  Bytecode,
  Execute,
  LoadLib,
};

using namespace std::string_view_literals;
constexpr std::string_view permission_to_string(const Permission p)
{
  switch (p) {
    case Permission::Network:        return "network"sv;
    case Permission::SpriteRead:     return "sprite_read"sv;
    case Permission::SpriteWrite:    return "sprite_write"sv;
    case Permission::IORead:         return "io_read"sv;
    case Permission::IOWrite:        return "io_write"sv;
    case Permission::ClipboardRead:  return "clipboard_read"sv;
    case Permission::ClipboardWrite: return "clipboard_write"sv;
    case Permission::TemporaryFile:  return "temporary_file"sv;
    case Permission::Preferences:    return "preferences"sv;
    case Permission::Debug:          return "debug"sv;
    case Permission::Bytecode:       return "bytecode"sv;
    case Permission::Execute:        return "execute"sv;
    case Permission::LoadLib:        return "loadlib"sv;
    default:                         {
      ASSERT(true);
      return "unknown"sv;
    }
  }
}

constexpr Permission string_to_permission(const std::string_view s)
{
  if (s == "network"sv)
    return Permission::Network;
  if (s == "sprite_read"sv)
    return Permission::SpriteRead;
  if (s == "sprite_write"sv)
    return Permission::SpriteWrite;
  if (s == "io_read"sv)
    return Permission::IORead;
  if (s == "io_write"sv)
    return Permission::IOWrite;
  if (s == "clipboard_read"sv)
    return Permission::ClipboardRead;
  if (s == "clipboard_write"sv)
    return Permission::ClipboardWrite;
  if (s == "temporary_file"sv)
    return Permission::TemporaryFile;
  if (s == "preferences"sv)
    return Permission::Preferences;
  if (s == "debug"sv)
    return Permission::Debug;
  if (s == "bytecode"sv)
    return Permission::Bytecode;
  if (s == "execute"sv)
    return Permission::Execute;
  if (s == "loadlib"sv)
    return Permission::LoadLib;

  ASSERT(true);
  return Permission::Unknown;
}

constexpr bool permission_supports_matching(const Permission p)
{
  ASSERT(p != Permission::Unknown);
  switch (p) {
    case Permission::Network:
    case Permission::SpriteRead:
    case Permission::SpriteWrite:
    case Permission::IORead:
    case Permission::IOWrite:
    case Permission::Preferences:
    case Permission::Execute:
    case Permission::LoadLib:     return true;
    default:                      return false;
  }
}

constexpr bool permission_is_scary(const Permission p)
{
  ASSERT(p != Permission::Unknown);
  switch (p) {
    case Permission::IORead:
    case Permission::IOWrite:
    case Permission::Bytecode:
    case Permission::Execute:
    case Permission::LoadLib:  return true;
    default:                   return false;
  }
}

} // namespace app::script

#endif
