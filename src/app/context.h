// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CONTEXT_H_INCLUDED
#define APP_CONTEXT_H_INCLUDED
#pragma once

#include "app/commands/params.h"
#include "app/context_flags.h"
#include "base/disable_copying.h"
#include "base/exception.h"
#include "doc/context.h"
#include "obs/signal.h"

#include <vector>

namespace app {
  class Command;
  class Document;
  class DocumentView;

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

  class Context : public doc::Context {
  public:
    Context();

    virtual bool isUIAvailable() const     { return false; }
    virtual bool isRecordingMacro() const  { return false; }
    virtual bool isExecutingMacro() const  { return false; }
    virtual bool isExecutingScript() const { return false; }

    bool checkFlags(uint32_t flags) const { return m_flags.check(flags); }
    void updateFlags() { m_flags.update(this); }

    void sendDocumentToTop(doc::Document* document);

    void setActiveDocument(doc::Document* document);
    app::Document* activeDocument() const;
    bool hasModifiedDocuments() const;

    void executeCommand(const char* commandName);
    virtual void executeCommand(Command* command, const Params& params = Params());

    virtual DocumentView* getFirstDocumentView(doc::Document* document) const {
      return nullptr;
    }

    obs::signal<void (CommandExecutionEvent&)> BeforeCommandExecution;
    obs::signal<void (CommandExecutionEvent&)> AfterCommandExecution;

  protected:
    void onCreateDocument(doc::CreateDocumentArgs* args) override;
    void onAddDocument(doc::Document* doc) override;
    void onRemoveDocument(doc::Document* doc) override;
    void onGetActiveSite(doc::Site* site) const override;
    virtual void onSetActiveDocument(doc::Document* doc);

    Document* lastSelectedDoc() { return m_lastSelectedDoc; }

  private:
    // Last updated flags.
    ContextFlags m_flags;
    Document* m_lastSelectedDoc;

    DISABLE_COPYING(Context);
  };

} // namespace app

#endif
