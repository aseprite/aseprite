// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CONTEXT_H_INCLUDED
#define APP_CONTEXT_H_INCLUDED
#pragma once

#include "app/commands/params.h"
#include "app/context_flags.h"
#include "app/context_observer.h"
#include "app/docs.h"
#include "app/docs_observer.h"
#include "base/disable_copying.h"
#include "base/exception.h"
#include "doc/frame.h"
#include "obs/observable.h"
#include "obs/signal.h"

#include <memory>
#include <vector>

namespace doc {
  class Layer;
  class PalettePicks;
}

namespace app {
  class ActiveSiteHandler;
  class Command;
  class Doc;
  class DocRange;
  class DocView;
  class Preferences;

  class CommandPreconditionException : public base::Exception {
  public:
    CommandPreconditionException() throw()
    : base::Exception("Cannot execute the command because its pre-conditions are false.") { }
  };

  class CommandExecutionEvent {
  public:
    CommandExecutionEvent(Command* command)
      : m_command(command), m_canceled(false) {
    }

    Command* command() const { return m_command; }

    // True if the command was canceled or simulated by an
    // observer/signal slot.
    bool isCanceled() const { return m_canceled; }
    void cancel() {
      m_canceled = true;
    }

  private:
    Command* m_command;
    bool m_canceled;
  };

  class Context : public obs::observable<ContextObserver>,
                  public DocsObserver {
  public:
    Context();
    virtual ~Context();

    const Docs& documents() const { return m_docs; }
    Docs& documents() { return m_docs; }

    Preferences& preferences() const;

    virtual bool isUIAvailable() const     { return false; }
    virtual bool isRecordingMacro() const  { return false; }
    virtual bool isExecutingMacro() const  { return false; }
    virtual bool isExecutingScript() const { return false; }

    bool checkFlags(uint32_t flags) const { return m_flags.check(flags); }
    void updateFlags() { m_flags.update(this); }

    void sendDocumentToTop(Doc* doc);
    void closeDocument(Doc* doc);

    Site activeSite() const;
    Doc* activeDocument() const;
    void setActiveDocument(Doc* document);
    void setActiveLayer(doc::Layer* layer);
    void setActiveFrame(doc::frame_t frame);
    void setRange(const DocRange& range);
    void setSelectedColors(const doc::PalettePicks& picks);
    bool hasModifiedDocuments() const;
    void notifyActiveSiteChanged();

    void executeCommandFromMenuOrShortcut(Command* command, const Params& params = Params());
    virtual void executeCommand(Command* command, const Params& params = Params());

    virtual DocView* getFirstDocView(Doc* document) const {
      return nullptr;
    }

    obs::signal<void (CommandExecutionEvent&)> BeforeCommandExecution;
    obs::signal<void (CommandExecutionEvent&)> AfterCommandExecution;

  protected:
    // DocsObserver impl
    void onAddDocument(Doc* doc) override;
    void onRemoveDocument(Doc* doc) override;

    virtual void onGetActiveSite(Site* site) const;
    virtual void onSetActiveDocument(Doc* doc);
    virtual void onSetActiveLayer(doc::Layer* layer);
    virtual void onSetActiveFrame(const doc::frame_t frame);
    virtual void onSetRange(const DocRange& range);
    virtual void onSetSelectedColors(const doc::PalettePicks& picks);
    virtual void onCloseDocument(Doc* doc);

    Doc* lastSelectedDoc() { return m_lastSelectedDoc; }

  private:
    ActiveSiteHandler* activeSiteHandler() const;

    mutable Docs m_docs;
    ContextFlags m_flags;       // Last updated flags.
    Doc* m_lastSelectedDoc;
    mutable std::unique_ptr<ActiveSiteHandler> m_activeSiteHandler;
    mutable std::unique_ptr<Preferences> m_preferences;

    DISABLE_COPYING(Context);
  };

} // namespace app

#endif
