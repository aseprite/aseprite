// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/backup.h"

namespace app {

Backup::Backup(const std::string& path)
  : m_path(path)
{
}

Backup::~Backup()
{
}

bool Backup::hasDataToRestore()
{
  return false;
}

} // namespace app
