// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/send_crash.h"

#include "app/app.h"
#include "app/console.h"
#include "app/i18n/strings.h"
#include "app/resource_finder.h"
#include "app/task.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "fmt/format.h"
#include "ui/alert.h"
#include "ui/system.h"
#include "ver/info.h"

#include "send_crash.xml.h"

namespace app {

// static
std::string SendCrash::DefaultMemoryDumpFilename()
{
#ifdef _WIN32
  std::string kDefaultCrashName = fmt::format("{}-crash-{}.dmp",
                                              get_app_name(),
                                              get_app_version());
  ResourceFinder rf;
  rf.includeUserDir(kDefaultCrashName.c_str());
  return rf.getFirstOrCreateDefault();
#else
  return std::string();
#endif
}

SendCrash::~SendCrash()
{
  if (m_task.running()) {
    m_task.cancel();
    m_task.wait();
  }
}

void SendCrash::search()
{
#ifdef _WIN32
  // On Windows we use one mini-dump to report bugs, then we can open
  // this .dmp file locally along with the .exe + .pdb to check the
  // stack trace and detect the cause of the bug.

  m_dumpFilename = SendCrash::DefaultMemoryDumpFilename();
  if (!m_dumpFilename.empty() &&
      base::is_file(m_dumpFilename)) {
    auto app = App::instance();
    app->memoryDumpFilename(m_dumpFilename);
#ifdef ENABLE_UI
    app->showNotification(this);
#endif
  }

#elif defined(__APPLE__)
  // On macOS we can show the possibility to send the latest crash
  // report from ~/Library/Logs/DiagnosticReports which is the
  // location where crash reports (.crash files) are located.

  m_task.run(
    [this](base::task_token&){
      ResourceFinder rf;
      rf.includeHomeDir("Library/Logs/DiagnosticReports");
      std::string dir = rf.defaultFilename();
      if (base::is_directory(dir)) {
        std::vector<std::string> candidates;
        int n = std::strlen(get_app_name());
        for (const auto& fn : base::list_files(dir)) {
          // Cancel everything
          if (m_task.canceled())
            return;

          if (base::utf8_icmp(get_app_name(), fn, n) == 0) {
            candidates.push_back(fn);
          }
        }
        std::sort(candidates.begin(), candidates.end());
        if (!candidates.empty()) {
          std::string fn = base::join_path(dir, candidates.back());
          if (base::is_file(fn)) {
            ui::execute_from_ui_thread(
              [this, fn]{
                m_dumpFilename = fn;
                if (auto app = App::instance()) {
                  app->memoryDumpFilename(fn);
#ifdef ENABLE_UI
                  app->showNotification(this);
#endif
                }
              });
          }
        }
      }
    });

#endif
}

#ifdef ENABLE_UI

std::string SendCrash::notificationText()
{
  return "Report last crash";
}

void SendCrash::notificationClick()
{
  if (m_dumpFilename.empty()) {
    ui::Alert::show(Strings::alerts_nothing_to_report());
    return;
  }

  app::gen::SendCrash dlg;

#if _WIN32
  // Only on Windows, if the current version is a development version
  // (i.e. the get_app_version() contains "-dev"), the .dmp
  // file is useless for us. This is because we need the .exe + .pdb +
  // source code used in the compilation process to make some sense of
  // the .dmp file.

  bool isDev = (std::string(get_app_version()).find("-dev") != std::string::npos);
  if (isDev) {
    dlg.official()->setVisible(false);
    dlg.devFilename()->setText(m_dumpFilename);
    dlg.devFilename()->Click.connect(base::Bind(&SendCrash::onClickDevFilename, this));
  }
  else
#endif  // On other platforms the crash file might be useful even in
        // the "-dev" version (e.g. on macOS it's a text file with
        // stack traces).
  {
    dlg.dev()->setVisible(false);
    dlg.filename()->setText(m_dumpFilename);
    dlg.filename()->Click.connect(base::Bind(&SendCrash::onClickFilename, this));
  }

  dlg.openWindowInForeground();
  if (dlg.closer() == dlg.deleteFile()) {
    try {
      base::delete_file(m_dumpFilename);
      m_dumpFilename = "";
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
  }
}

void SendCrash::onClickFilename()
{
  base::launcher::open_folder(m_dumpFilename);
}

void SendCrash::onClickDevFilename()
{
  base::launcher::open_file(m_dumpFilename);
}

#endif // ENABLE_UI

} // namespace app
