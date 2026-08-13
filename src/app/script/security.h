// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
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

#include <string>

#include "script_access.xml.h"

#include "nlohmann/json.hpp"

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

class PermissionStorage {
public:
  PermissionStorage();
  explicit PermissionStorage(const std::string& path);

  static PermissionStorage* instance();

  std::optional<bool> read(const std::string& script, Permission permission) const;
  std::optional<bool> readMatch(const std::string& script,
                                Permission permission,
                                const std::string& match) const;
  bool readFullAccess(const std::string& script) const;

  void write(const std::string& script, Permission permission, bool value);
  void writeForMatch(const std::string& script,
                     Permission permission,
                     const std::string& match,
                     bool value);
  void writeFullAccess(const std::string& script, bool access);

  bool passesIntegrityCheck(const std::string& script);
  void reset(const std::string& script = std::string());
  void reset(const std::string& script, Permission permission);
  void reset(const std::string& script, Permission permission, const std::string& match);

  std::vector<std::string> scripts() const;
  std::vector<Permission> stored(const std::string& script) const;
  std::vector<std::pair<std::string, bool>> matches(const std::string& script,
                                                    Permission permission) const;

private:
  void load();
  void flush();

  nlohmann::json m_json;
  std::string m_path;
  bool m_pendingFlush = false;
};

class PermissionDialog : public app::gen::ScriptAccess {
public:
  enum class Remember : uint8_t { Nothing, Permission, Match, Directory, FullAccess, AllOfType };
  PermissionDialog(const std::string& origin,
                   const std::string& extensionName,
                   Permission permission,
                   const std::string& match);

  std::pair<bool, Remember> ask();

private:
  void showPopup();
  bool m_isExtension;
  ui::Timer m_timer;
  base::tick_t m_scaryAllowCooldown;
};

void set_permission_storage(PermissionStorage* storage);

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
    default:                         return "unknown"sv;
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

  return Permission::Unknown;
}

constexpr bool permission_supports_matching(const Permission p)
{
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
