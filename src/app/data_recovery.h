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

#ifndef DATA_RECOVERY_H_INCLUDED
#define DATA_RECOVERY_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/disable_copying.h"
#include "base/slot.h"
#include "context_observer.h"
#include "document_observer.h"
#include "documents.h"

namespace base { class TempDir; }

namespace app {

  class Backup;

  class DataRecovery : public ContextObserver
                     , public DocumentObserver {
  public:
    DataRecovery(Context* context);
    ~DataRecovery();

    // Returns a backup if there are data to be restored from a
    // crash. Or null if the program didn't crash in its previous
    // execution.
    Backup* getBackup() { return m_backup; }

  private:
    void onAddDocument(Context* context, Document* document) OVERRIDE;
    void onRemoveDocument(Context* context, Document* document) OVERRIDE;

    base::TempDir* m_tempDir;
    Backup* m_backup;
    Context* m_context;

    DISABLE_COPYING(DataRecovery);
  };

} // namespace app

#endif
