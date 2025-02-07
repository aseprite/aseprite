// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/cmd_open_file.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/fs.h"
#include "base/thread.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <cstdio>

namespace app {

class OpenFileJob : public Job,
                    public IFileOpProgress {
public:
  OpenFileJob(FileOp* fop, const bool showProgress)
    : Job(Strings::open_file_loading(), showProgress)
    , m_fop(fop)
  {
  }

  void showProgressWindow()
  {
    startJob();

    if (isCanceled())
      m_fop->stop();

    waitJob();
  }

private:
  // Thread to do the hard work: load the file from the disk.
  virtual void onJob() override
  {
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

  virtual void ackFileOpProgress(double progress) override { jobProgress(progress); }

  FileOp* m_fop;
};

OpenFileCommand::OpenFileCommand()
  : Command(CommandId::OpenFile(), CmdRecordableFlag)
  , m_ui(true)
  , m_repeatCheckbox(false)
  , m_oneFrame(false)
  , m_seqDecision(gen::SequenceDecision::ASK)
{
}

void OpenFileCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
  m_folder = params.get("folder"); // Initial folder

  if (params.has_param("ui"))
    m_ui = params.get_as<bool>("ui");
  else
    m_ui = true;

  m_repeatCheckbox = params.get_as<bool>("repeat_checkbox");
  m_oneFrame = params.get_as<bool>("oneframe");

  std::string sequence = params.get("sequence");
  if (m_oneFrame || sequence == "skip" || sequence == "no") {
    m_seqDecision = gen::SequenceDecision::NO;
  }
  else if (sequence == "agree" || sequence == "yes") {
    m_seqDecision = gen::SequenceDecision::YES;
  }
  else {
    m_seqDecision = gen::SequenceDecision::ASK;
  }
}

void OpenFileCommand::onExecute(Context* context)
{
  Console console;

  m_usedFiles.clear();

  base::paths filenames;

  // interactive
  if (context->isUIAvailable() && m_filename.empty()) {
    base::paths exts = get_readable_extensions();

    // Add backslash as show_file_selector() expected a filename as
    // initial path (and the file part is removed from the path).
    if (!m_folder.empty() && !base::is_path_separator(m_folder[m_folder.size() - 1]))
      m_folder.push_back(base::path_separator);

    if (!app::show_file_selector(Strings::open_file_title(),
                                 m_folder,
                                 exts,
                                 FileSelectorType::OpenMultiple,
                                 filenames)) {
      // The user cancelled the operation through UI
      return;
    }

    // If the user selected several files, show the checkbox to repeat
    // the action for future filenames in the batch of selected files
    // to open.
    if (filenames.size() > 1)
      m_repeatCheckbox = true;
  }
  else if (!m_filename.empty()) {
    filenames.push_back(m_filename);
  }

  if (filenames.empty())
    return;

  int flags = FILE_LOAD_DATA_FILE | FILE_LOAD_CREATE_PALETTE |
              (m_repeatCheckbox ? FILE_LOAD_SEQUENCE_ASK_CHECKBOX : 0);

  if (context->isUIAvailable() && m_seqDecision == gen::SequenceDecision::ASK) {
    if (Preferences::instance().openFile.openSequence() == gen::SequenceDecision::ASK) {
      // Do nothing (ask by default, or whatever the command params
      // specified)
    }
    else {
      m_seqDecision = Preferences::instance().openFile.openSequence();
    }
  }

  switch (m_seqDecision) {
    case gen::SequenceDecision::ASK: flags |= FILE_LOAD_SEQUENCE_ASK; break;
    case gen::SequenceDecision::YES: flags |= FILE_LOAD_SEQUENCE_YES; break;
    case gen::SequenceDecision::NO:  flags |= FILE_LOAD_SEQUENCE_NONE; break;
  }

  if (m_oneFrame)
    flags |= FILE_LOAD_ONE_FRAME;

  std::string filename;
  while (!filenames.empty()) {
    filename = filenames[0];
    filenames.erase(filenames.begin());

    std::unique_ptr<FileOp> fop(FileOp::createLoadDocumentOperation(context, filename, flags));
    bool unrecent = false;

    // Do nothing (the user cancelled or something like that)
    if (!fop)
      return;

    if (fop->hasError()) {
      console.printf(fop->error().c_str());
      unrecent = true;
    }
    else {
      if (fop->isSequence()) {
        if (fop->sequenceFlags() & FILE_LOAD_SEQUENCE_YES) {
          m_seqDecision = gen::SequenceDecision::YES;
          flags &= ~FILE_LOAD_SEQUENCE_ASK;
          flags |= FILE_LOAD_SEQUENCE_YES;
        }
        else if (fop->sequenceFlags() & FILE_LOAD_SEQUENCE_NONE) {
          m_seqDecision = gen::SequenceDecision::NO;
          flags &= ~FILE_LOAD_SEQUENCE_ASK;
          flags |= FILE_LOAD_SEQUENCE_NONE;
        }

        for (std::string fn : fop->filenames()) {
          fn = base::normalize_path(fn);
          m_usedFiles.push_back(fn);

          auto it = std::find(filenames.begin(), filenames.end(), fn);
          if (it != filenames.end())
            filenames.erase(it);
        }
      }
      else {
        auto fn = base::normalize_path(fop->filename());
        m_usedFiles.push_back(fn);
      }

      OpenFileJob task(fop.get(), m_ui);
      task.showProgressWindow();

      // Post-load processing, it is called from the GUI because may require user intervention.
      fop->postLoad();

      // Show any error
      if (fop->hasError() && !fop->isStop())
        console.printf(fop->error().c_str());

      Doc* doc = fop->document();
      if (doc) {
        if (context->isUIAvailable()) {
          App::instance()->recentFiles()->addRecentFile(fop->filename().c_str());
          auto& docPref = Preferences::instance().document(doc);

          if (fop->hasEmbeddedGridBounds() && !doc->sprite()->gridBounds().isEmpty()) {
            // If the sprite contains the grid bounds inside, we put
            // those grid bounds into the settings (e.g. useful to
            // interact with old versions of Aseprite saving the grid
            // bounds in the aseprite.ini file)
            docPref.grid.bounds(doc->sprite()->gridBounds());
          }
          else {
            // Get grid bounds from preferences
            doc->sprite()->setGridBounds(docPref.grid.bounds());
          }
        }

        doc->setContext(context);
      }
      else if (!fop->isStop())
        unrecent = true;
    }

    // The file was not found or was loaded loaded with errors,
    // so we can remove it from the recent-file list
    if (unrecent) {
      if (context->isUIAvailable())
        App::instance()->recentFiles()->removeRecentFile(m_filename);
    }
  }
}

std::string OpenFileCommand::onGetFriendlyName() const
{
  // TO DO: would be better to show the last part of the path
  // via text size hint instead of a fixed number of chars.
  auto uiScale = Preferences::instance().general.uiScale();
  auto scScale = Preferences::instance().general.screenScale();
  int pos(68.0 / double(uiScale) / double(scScale));
  return Command::onGetFriendlyName().append(
    (m_filename.empty() ?
       "" :
       (": " + (m_filename.size() >= pos ? m_filename.substr(m_filename.size() - pos, pos) :
                                           m_filename))));
}

Command* CommandFactory::createOpenFileCommand()
{
  return new OpenFileCommand;
}

} // namespace app
