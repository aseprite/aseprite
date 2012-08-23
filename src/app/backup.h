/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#ifndef APP_BACKUP_H_INCLUDED
#define APP_BACKUP_H_INCLUDED

#include "base/disable_copying.h"
#include "base/string.h"

namespace app {

  // A class to record/restore backup information.
  class Backup {
  public:
    Backup(const base::string& path);
    ~Backup();

    // Returns true if there are items that can be restored.
    bool hasDataToRestore();

  private:
    DISABLE_COPYING(Backup);

    base::string m_path;
  };

} // namespace app

#endif
