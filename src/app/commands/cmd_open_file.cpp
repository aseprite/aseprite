// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/job.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/recent_files.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/path.h"
#include "base/thread.h"
#include "base/unique_ptr.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <cstdio>

namespace app {

class OpenFileCommand : public Command {
public:
  OpenFileCommand();
  Command* clone() const override { return new OpenFileCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  std::string m_filename;
  std::string m_folder;
};

class OpenFileJob : public Job, public IFileOpProgress
{
public:
  OpenFileJob(FileOp* fop)
    : Job("Loading file")
    , m_fop(fop)
  {
  }

  void showProgressWindow() {
    startJob();

    if (isCanceled())
      m_fop->stop();

    waitJob();
  }

private:
  // Thread to do the hard work: load the file from the disk.
  virtual void onJob() override {
    try {
      m_fop->operate(this);
    }
    catch (const std::exception& e) {
      m_fop->setError("Error loading file:\n%s", e.what());
    }

    if (m_fop->isStop() && m_fop->document())
      delete m_fop->releaseDocument();

    m_fop->done();
  }

  virtual void ackFileOpProgress(double progress) override {
    jobProgress(progress);
  }

  FileOp* m_fop;
};

OpenFileCommand::OpenFileCommand()
  : Command("OpenFile",
            "Open Sprite",
            CmdRecordableFlag)
{
}

void OpenFileCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
  m_folder = params.get("folder"); // Initial folder
}

void OpenFileCommand::onExecute(Context* context)
{
  Console console;

  // interactive
  if (context->isUIAvailable() && m_filename.empty()) {
    std::string exts = get_readable_extensions();

    // Add backslash as show_file_selector() expected a filename as
    // initial path (and the file part is removed from the path).
    if (!m_folder.empty() && !base::is_path_separator(m_folder[m_folder.size()-1]))
      m_folder.push_back(base::path_separator);

    m_filename = app::show_file_selector("Open", m_folder, exts,
      FileSelectorType::Open);
  }

  if (!m_filename.empty()) {
    base::UniquePtr<FileOp> fop(
      FileOp::createLoadDocumentOperation(
        context, m_filename.c_str(), FILE_LOAD_SEQUENCE_ASK));
    bool unrecent = false;

    if (fop) {
      if (fop->hasError()) {
        console.printf(fop->error().c_str());
        unrecent = true;
      }
      else {
        OpenFileJob task(fop);
        task.showProgressWindow();

        // Post-load processing, it is called from the GUI because may require user intervention.
        fop->postLoad();

        // Show any error
        if (fop->hasError())
          console.printf(fop->error().c_str());

        Document* document = fop->document();
        if (document) {
          if (context->isUIAvailable())
            App::instance()->getRecentFiles()->addRecentFile(fop->filename().c_str());

          document->setContext(context);
        }
        else if (!fop->isStop())
          unrecent = true;
      }

      // The file was not found or was loaded loaded with errors,
      // so we can remove it from the recent-file list
      if (unrecent) {
        if (context->isUIAvailable())
          App::instance()->getRecentFiles()->removeRecentFile(m_filename.c_str());
      }
    }
    else {
      // Do nothing (the user cancelled or something like that)
    }
  }
}

Command* CommandFactory::createOpenFileCommand()
{
  return new OpenFileCommand;
}

} // namespace app
