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

#include "config.h"

#include "app.h"
#include "app/file_selector.h"
#include "base/bind.h"
#include "base/thread.h"
#include "base/unique_ptr.h"
#include "commands/command.h"
#include "commands/params.h"
#include "console.h"
#include "document.h"
#include "file/file.h"
#include "job.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "recent_files.h"
#include "ui/gui.h"
#include "ui_context.h"
#include "widgets/status_bar.h"

#include <allegro.h>
#include <stdio.h>

static const int kMonitoringPeriod = 100;

//////////////////////////////////////////////////////////////////////
// open_file

class OpenFileCommand : public Command
{
public:
  OpenFileCommand();
  Command* clone() { return new OpenFileCommand(*this); }

protected:
  void onLoadParams(Params* params);
  void onExecute(Context* context);

private:
  std::string m_filename;
};

class OpenFileJob : public Job, public IFileOpProgress
{
public:
  OpenFileJob(FileOp* fop, const char* filename)
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
    UniquePtr<FileOp> fop(fop_to_load_document(m_filename.c_str(), FILE_LOAD_SEQUENCE_ASK));
    bool unrecent = false;

    if (fop) {
      if (fop->has_error()) {
        console.printf(fop->error.c_str());
        unrecent = true;
      }
      else {
        OpenFileJob task(fop, get_filename(m_filename.c_str()));
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

          set_document_in_more_reliable_editor(document);
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

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createOpenFileCommand()
{
  return new OpenFileCommand;
}
