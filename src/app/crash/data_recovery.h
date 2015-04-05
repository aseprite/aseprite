// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CRASH_DATA_RECOVERY_H_INCLUDED
#define APP_CRASH_DATA_RECOVERY_H_INCLUDED
#pragma once

#include "app/crash/session.h"
#include "base/disable_copying.h"
#include "doc/context_observer.h"
#include "doc/document_observer.h"
#include "doc/documents_observer.h"

#include <vector>

namespace doc {
  class Context;
}

namespace app {
namespace crash {

  class DataRecovery : public doc::ContextObserver
                     , public doc::DocumentsObserver
                     , public doc::DocumentObserver {
  public:
    typedef std::vector<SessionPtr> Sessions;

    DataRecovery(doc::Context* context);
    ~DataRecovery();

    // Returns the list of sessions that can be recovered.
    const Sessions& sessions() { return m_sessions; }

  private:
    virtual void onAddDocument(doc::Document* document) override;
    virtual void onRemoveDocument(doc::Document* document) override;

    Sessions m_sessions;
    SessionPtr m_inProgress;
    doc::Context* m_context;

    DISABLE_COPYING(DataRecovery);
  };

} // namespace crash
} // namespace app

#endif
