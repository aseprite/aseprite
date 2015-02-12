// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_DATA_RECOVERY_H_INCLUDED
#define APP_DATA_RECOVERY_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/slot.h"
#include "doc/context_observer.h"
#include "doc/document_observer.h"
#include "doc/documents_observer.h"

namespace base {
  class TempDir;
}

namespace doc {
  class Context;
}

namespace app {
  class Backup;

  class DataRecovery : public doc::ContextObserver
                     , public doc::DocumentsObserver
                     , public doc::DocumentObserver {
  public:
    DataRecovery(doc::Context* context);
    ~DataRecovery();

    // Returns a backup if there are data to be restored from a
    // crash. Or null if the program didn't crash in its previous
    // execution.
    Backup* getBackup() { return m_backup; }

  private:
    virtual void onAddDocument(doc::Document* document) override;
    virtual void onRemoveDocument(doc::Document* document) override;

    base::TempDir* m_tempDir;
    Backup* m_backup;
    doc::Context* m_context;

    DISABLE_COPYING(DataRecovery);
  };

} // namespace app

#endif
