// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/app.h"
#include "app/extensions.h"
#include "app/i18n/strings.h"
#include "app/resource_finder.h"
#include "app/script/about_extension_window.h"
#include "app/script/engine.h"
#include "app/script/permissions.h"
#include "app/ui/skin/skin_theme.h"
#include "base/file_handle.h"
#include "base/fstream_path.h"
#include "base/log.h"
#include "ui/system.h"

#include "app/script/permission_storage.h"

#ifdef LAF_WINDOWS
  #include <shlwapi.h>
#else
  #include <fnmatch.h>
#endif

#include <fstream>
#include <sstream>

#include "blake3.h"
#include "nlohmann/json.hpp"

namespace app::script {
using json = nlohmann::json;

namespace {
PermissionStorage* g_instance_override = nullptr;

bool wildcard_match(const std::string& pattern, const std::string& str)
{
#ifdef LAF_WINDOWS
  return PathMatchSpec(base::from_utf8(str).c_str(), base::from_utf8(pattern).c_str());
#else
  return fnmatch(pattern.c_str(), str.c_str(), 0) != FNM_NOMATCH;
#endif
}

int wildcard_specificity_score(const std::string_view pattern)
{
  int score = 0;
  for (const char c : pattern) {
    if (c != '*' && c != '?') {
      score++;
    }
    else {
      score -= 2;
    }
  }
  return score;
}

constexpr bool is_extension(const std::string_view script)
{
  return script.rfind(Engine::kExtensionPrefix, 0) == 0;
}

std::string get_integrity_hash(const std::string& filename)
{
  ASSERT(!is_extension(filename));
  const auto file = base::open_file(filename, "rb");
  if (!file)
    return std::string();

  blake3_hasher hasher;
  blake3_hasher_init(&hasher);

  constexpr size_t size = 1024uL * 1024;
  std::vector<uint8_t> buffer(size);

  size_t read;
  while ((read = std::fread(buffer.data(), 1, size, file.get())) > 0)
    blake3_hasher_update(&hasher, buffer.data(), read);

  std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
  blake3_hasher_finalize(&hasher, hash.data(), BLAKE3_OUT_LEN);

  std::stringstream stream;
  for (size_t i = 0; i < BLAKE3_OUT_LEN; ++i)
    stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(hash[i]);

  return stream.str();
}
} // namespace

void set_permission_storage(PermissionStorage* storage)
{
  g_instance_override = storage;
}

PermissionStorage::PermissionStorage()
{
  ResourceFinder rf;
  rf.includeUserDir("permissions.json");
  m_path = rf.getFirstOrCreateDefault();
  load();
}

PermissionStorage::PermissionStorage(const std::string& path) : m_path(path)
{
  load();
}

std::optional<bool> PermissionStorage::read(const std::string& script,
                                            const Permission permission) const
{
  ASSERT(!permission_supports_matching(permission))

  if (!m_json.contains(script))
    return std::nullopt;

  if (readFullAccess(script))
    return true;

  const auto& permissionString = permission_to_string(permission);
  if (m_json[script].contains("permissions"sv) &&
      m_json[script]["permissions"sv].contains(permissionString)) {
    return m_json[script]["permissions"sv][permissionString].value("granted"sv, false);
  }

  return std::nullopt;
}

std::optional<bool> PermissionStorage::readMatch(const std::string& script,
                                                 const Permission permission,
                                                 const std::string& match) const
{
  ASSERT(permission_supports_matching(permission))
  ASSERT(permission != Permission::Unknown);

  if (!m_json.contains(script))
    return std::nullopt;

  if (readFullAccess(script))
    return true;

  const std::string permissionString(permission_to_string(permission));

  json::json_pointer ptr("/permissions");
  ptr /= permissionString;

  if (m_json[script].contains(ptr)) {
    std::vector<std::pair<std::string, bool>> matches;
    for (const auto& it : m_json[script][ptr].items()) {
      const auto& permMatch = it.value().value("match"sv, "");
      if (wildcard_match(permMatch, match))
        matches.emplace_back(permMatch, it.value().value("granted"sv, false));
    }

    std::sort(
      matches.begin(),
      matches.end(),
      [](const std::pair<std::string_view, bool>& a, const std::pair<std::string_view, bool>& b) {
        return wildcard_specificity_score(a.first) > wildcard_specificity_score(b.first);
      });

    if (!matches.empty())
      return matches.begin()->second;
  }

  return std::nullopt;
}

void PermissionStorage::write(const std::string& script, const Permission permission, bool value)
{
  ASSERT(!permission_supports_matching(permission));
  ASSERT(permission != Permission::Unknown);
  if (permission == Permission::Unknown)
    return;

  const auto& permissionString = permission_to_string(permission);

  if (!m_json.contains(script) && !is_extension(script))
    m_json[script]["integrity"sv] = get_integrity_hash(script);

  m_json[script]["permissions"sv][permissionString] = json::object({
    { "granted"sv, value }
  });

  flush();
}

void PermissionStorage::writeForMatch(const std::string& script,
                                      const Permission permission,
                                      const std::string& match,
                                      const bool value)
{
  ASSERT(permission_supports_matching(permission));
  ASSERT(permission != Permission::Unknown);
  if (permission == Permission::Unknown)
    return;

  const std::string permissionString(permission_to_string(permission));

  json::json_pointer ptr("/permissions");
  ptr /= permissionString;

  if (!m_json.contains(script) && !is_extension(script))
    m_json[script]["integrity"sv] = get_integrity_hash(script);

  auto obj = json::object({
    { "match",   match },
    { "granted", value }
  });

  if (m_json[script][ptr].is_array()) {
    // Modify existing match in-place
    for (const auto& it : m_json[script][ptr].items()) {
      if (it.value().value("match"sv, "") == match) {
        m_json[script][ptr / it.key() / "granted"] = value;
        return;
      }
    }

    m_json[script][ptr].push_back(obj);
  }
  else {
    m_json[script][ptr] = nlohmann::json::array({ obj });
  }

  flush();
}

bool PermissionStorage::readFullAccess(const std::string& script) const
{
  if (!m_json.contains(script))
    return false;

  return m_json[script].value("full_access"sv, false);
}

void PermissionStorage::writeFullAccess(const std::string& script, const bool access)
{
  // Granting "full access" is entirely destructive of old permissions
  if (access) {
    m_json[script]["full_access"sv] = true;

    if (!is_extension(script))
      m_json[script]["integrity"sv] = get_integrity_hash(script);
  }
  else {
    m_json[script]["full_access"sv] = false;
  }
  flush();
}

bool PermissionStorage::isValid() const
{
  try {
    if (!m_json.is_object())
      return false;

    for (const auto& item : m_json.items()) {
      if (item.key().empty())
        return false;

      if (item.value().empty())
        return false;

      if (!item.value().is_object())
        return false;

      if (item.value().contains("permissions")) {
        auto permissions = item.value()["permissions"];
        if (!permissions.is_object())
          return false;

        for (const auto& perm : permissions.items()) {
          const auto p = string_to_permission(perm.key());
          if (perm.key().empty() || p == Permission::Unknown)
            return false;
          if (perm.value().empty())
            return false;

          if (perm.value().is_object()) {
            if (permission_supports_matching(p))
              return false;

            if (!(perm.value().contains("granted")))
              return false;

            if (!perm.value()["granted"].is_boolean())
              return false;
          }
          else if (perm.value().is_array()) {
            if (!permission_supports_matching(p))
              return false;

            for (const auto& match : perm.value()) {
              if (!match.is_object())
                return false;

              if (!(match.contains("granted") && match.contains("match")))
                return false;

              if (!(match["granted"].is_boolean() && match["match"].is_string()))
                return false;
            }
          }
          else {
            return false;
          }
        }
      }
    }
  }
  catch (const std::exception& e) {
    return false;
  }

  return true;
}

bool PermissionStorage::passesIntegrityCheck(const std::string& script)
{
  if (!m_json.contains(script))
    return true;

  try {
    const std::string& hash = m_json[script].value("integrity"sv, "");
    if (hash.size() != BLAKE3_OUT_LEN * 2uL)
      return false;
    return hash == get_integrity_hash(script);
  }
  catch (const std::exception& e) {
    LOG(WARNING,
        "Failed to perform integrity check of script '%s' with error '%s'",
        script.c_str(),
        e.what());
    return false;
  }
}

void PermissionStorage::reset(const std::string& script)
{
  try {
    if (script.empty())
      m_json = json::object();
    else
      m_json.erase(script);
  }
  catch (const std::exception& e) {
    LOG(ERROR, "Parsing error while resetting permissions with error '%s'", e.what());
    m_json = json::object();
  }
  flush();
}

void PermissionStorage::reset(const std::string& script, Permission permission)
{
  try {
    if (!m_json.contains(script))
      return;

    m_json[script]["permissions"sv].erase(permission_to_string(permission));
    flush();
  }
  catch (const std::exception& e) {
  }
}

void PermissionStorage::reset(const std::string& script,
                              const Permission permission,
                              const std::string& match)
{
  try {
    const std::string permissionString(permission_to_string(permission));
    json::json_pointer ptr("/permissions");
    ptr /= permissionString;

    if (!m_json[script].contains(ptr))
      return;

    int i = 0;
    for (const auto& val : m_json[script][ptr]) {
      if (val["match"sv] == match) {
        m_json[script][ptr].erase(i);
        flush();
        return;
      }
      i++;
    }
  }
  catch (const std::exception& e) {
  }
}

std::vector<std::string> PermissionStorage::scripts() const
{
  std::vector<std::string> scripts;
  for (const auto& [key, _] : m_json.items())
    scripts.push_back(key);
  return scripts;
}

std::vector<Permission> PermissionStorage::stored(const std::string& script) const
{
  std::vector<Permission> stored;

  if (!m_json.contains(script) || !m_json[script].contains("permissions"sv))
    return stored;

  for (const auto& [key, _] : m_json[script]["permissions"sv].items()) {
    stored.push_back(string_to_permission(key));
  }

  return stored;
}

std::vector<std::pair<std::string, bool>> PermissionStorage::matches(
  const std::string& script,
  const Permission permission) const
{
  std::vector<std::pair<std::string, bool>> matches;

  if (!m_json.contains(script) || !m_json[script].contains("permissions"sv))
    return matches;

  const std::string permissionString(permission_to_string(permission));
  json::json_pointer ptr("/permissions");
  ptr /= permissionString;

  if (m_json[script][ptr].is_array()) {
    for (const auto& it : m_json[script][ptr].items()) {
      matches.emplace_back(it.value()["match"sv], it.value()["granted"sv]);
    }
  }

  return matches;
}

void PermissionStorage::load()
{
  try {
    const auto file = base::open_file(m_path, "rb");
    if (file) {
      m_json = json::parse(file.get());
      if (!isValid())
        throw std::runtime_error("invalid permissions schema");
    }
  }
  catch (const std::exception& e) {
    TRACEARGS(e.what());
    LOG(ERROR, "Failed to load permissions at '%s' with error '%s'\n", m_path.c_str(), e.what());
    m_json = json::object();
    flush();
  }
}

void PermissionStorage::flush()
{
  if (m_pendingFlush)
    return;

  m_pendingFlush = true;
  execute_now_or_enqueue([this] {
    m_pendingFlush = false;
    std::ofstream out(FSTREAM_PATH(m_path), std::ios::binary);
#if defined(_DEBUG) || defined(ENABLE_DEVMODE)
    const int indent = 2;
#else
    const int indent = -1;
#endif
    if (out) {
      out << m_json.dump(indent);
    }
    else {
      LOG(ERROR, "Failed to write permissions to '%s'", m_path.c_str());
    }
  });
}

PermissionStorage* PermissionStorage::instance()
{
  if (g_instance_override)
    return g_instance_override;
  static PermissionStorage instance;
  return &instance;
}
} // namespace app::script
