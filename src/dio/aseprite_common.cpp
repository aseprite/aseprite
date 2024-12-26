// Aseprite Document IO Library
// Copyright (c) 2018-2023 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dio/aseprite_common.h"

#include "base/debug.h"

namespace dio {

uint32_t AsepriteExternalFiles::insert(const uint8_t type, const std::string& filename)
{
  auto it = m_toID[type].find(filename);
  if (it != m_toID[type].end())
    return it->second;
  else {
    insert(++m_lastid, type, filename);
    return m_lastid;
  }
}

void AsepriteExternalFiles::insert(uint32_t id, const uint8_t type, const std::string& filename)
{
  ASSERT(type >= 0 && type < ASE_EXTERNAL_FILE_TYPES);

  m_items[id] = Item{ filename, type };
  m_toID[type][filename] = id;
}

bool AsepriteExternalFiles::getIDByFilename(const uint8_t type,
                                            const std::string& fn,
                                            uint32_t& id) const
{
  ASSERT(type >= 0 && type < ASE_EXTERNAL_FILE_TYPES);

  auto it = m_toID[type].find(fn);
  if (it == m_toID[type].end())
    return false;
  id = it->second;
  return true;
}

bool AsepriteExternalFiles::getFilenameByID(uint32_t id, std::string& fn) const
{
  auto it = m_items.find(id);
  if (it == m_items.end())
    return false;
  fn = it->second.fn;
  return true;
}

std::string AsepriteExternalFiles::tileManagementPlugin() const
{
  constexpr uint8_t type = ASE_EXTERNAL_FILE_TILE_MANAGEMENT;
  auto it = m_toID[type].begin();
  if (it != m_toID[type].end())
    return it->first;
  else
    return std::string();
}

} // namespace dio
