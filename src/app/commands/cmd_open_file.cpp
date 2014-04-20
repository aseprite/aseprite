/* Aseprite
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
#include "base/thread.h"
#include "base/unique_ptr.h"
#include "raster/sprite.h"
#include "ui/ui.h"

#include <cstdio>

static const int kMonitoringPeriod = 100;

namespace app {

class OpenFileCommand : public Command {
public:
  OpenFileCommand();
  Command* clone() const OVERRIDE { return new OpenFileCommand(*this); }

protected:
  void onLoadParams(Params* params) OVERRIDE;
  void onExecute(Context* context) OVERRIDE;

private:
  std::string m_filename;
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
    fop_stop(m_fop);
  }

private:
  // Thread to do the hard work: load the file from the disk.
  virtual void onJob() OVERRIDE {
    try {
      fop_operate(m_fop, this);
    }
    catch (const std::exception& e) {
      fop_error(m_fop, "Error loading file:\n%s", e.what());
    }

    if (fop_is_stop(m_fop) && m_fop->document) {
      delete m_fop->document;
      m_fop->document = NULL;
    }

    fop_done(m_fop);
  }

  virtual void ackFileOpProgress(double progress) OVERRIDE {
    jobProgress(progress);
  }

  FileOp* m_fop;
};

OpenFileCommand::OpenFileCommand()
  : Command("OpenFile",
            "Open Sprite",
            CmdRecordableFlag)
{
  m_filename = "";
}

void OpenFileCommand::onLoadParams(Params* params)
{
  m_filename = params->get("filename");
}

void OpenFileCommand::onExecute(Context* context)
{
  Console console;

  // interactive
  if (context->isUiAvailable() && m_filename.empty()) {
    char exts[4096];
    get_readable_extensions(exts, sizeof(exts));
    m_filename = app::show_file_selector("Open", "", exts);
  }

  if (!m_filename.empty()) {
    base::UniquePtr<FileOp> fop(fop_to_load_document(m_filename.c_str(), FILE_LOAD_SEQUENCE_ASK));
    bool unrecent = false;

    if (fop) {
      if (fop->has_error()) {
        console.printf(fop->error.c_str());
        unrecent = true;
      }
      else {
        OpenFileJob task(fop);
        task.showProgressWindow();

        // Post-load processing, it is called from the GUI because may require user intervention.
        fop_post_load(fop);

        // Show any error
        if (fop->has_error())
          console.printf(fop->error.c_str());

        Document* document = fop->document;
        if (document) {
          UIContext* context = UIContext::instance();

          App::instance()->getRecentFiles()->addRecentFile(fop->filename.c_str());
          context->addDocument(document);
        }
        else if (!fop_is_stop(fop))
          unrecent = true;
      }

      // The file was not found or was loaded loaded with errors,
      // so we can remove it from the recent-file list
      if (unrecent) {
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
