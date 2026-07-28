// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_PERMISSION_STORAGE_H_INCLUDED
#define APP_SCRIPT_PERMISSION_STORAGE_H_INCLUDED

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/script/permissions.h"

#include "nlohmann/json.hpp"

namespace app::script {

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

  bool isValid() const;

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

void set_permission_storage(PermissionStorage* storage);

} // namespace app::script

#endif // APP_SCRIPT_PERMISSION_STORAGE_H_INCLUDED
