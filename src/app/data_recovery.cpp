/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "app/data_recovery.h"

#include "app/backup.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/temp_dir.h"
#include "document.h"
#include "ui_context.h"

#include <allegro.h>

namespace app {

DataRecovery::DataRecovery(Context* context)
  : m_tempDir(NULL)
  , m_backup(NULL)
  , m_context(context)
{
  // Check if there is already data to recover
  const base::string existent_data_path = get_config_string("DataRecovery", "Path", "");
  if (!existent_data_path.empty() &&
      base::directory_exists(existent_data_path)) {
    // Load the backup data.
    m_tempDir = new base::TempDir();
    m_tempDir->attach(existent_data_path);
    m_backup = new Backup(existent_data_path);
  }
  else {
    // Create a new directory to save the backup information.
    m_tempDir = new base::TempDir(PACKAGE);
    m_backup = new Backup(m_tempDir->path());

    set_config_string("DataRecovery", "Path", m_tempDir->path().c_str());
    flush_config_file();
  }

  m_context->addObserver(this);
}

DataRecovery::~DataRecovery()
{
  m_context->removeObserver(this);

  delete m_backup;

  if (m_tempDir) {
    delete m_tempDir;
    set_config_string("DataRecovery", "Path", "");
  }
}

void DataRecovery::onAddDocument(Context* context, Document* document)
{
  document->addObserver(this);
}

void DataRecovery::onRemoveDocument(Context* context, Document* document)
{
  document->removeObserver(this);
}

} // namespace app
