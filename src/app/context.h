// Aseprite
// Copyright (C) 2001-2016  David Capello
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
#include "base/signal.h"
#include "doc/context.h"

#include <vector>

namespace app {
  class Command;
  class Document;

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

    app::Document* activeDocument() const;
    bool hasModifiedDocuments() const;

    void executeCommand(const char* commandName);
    virtual void executeCommand(Command* command, const Params& params = Params());

    base::Signal1<void, CommandExecutionEvent&> BeforeCommandExecution;
    base::Signal1<void, CommandExecutionEvent&> AfterCommandExecution;

  protected:
    virtual void onCreateDocument(doc::CreateDocumentArgs* args) override;

  private:
    // Last updated flags.
    ContextFlags m_flags;

    DISABLE_COPYING(Context);
  };

} // namespace app

#endif
