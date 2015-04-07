// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/session.h"

#include "app/context.h"
#include "app/document.h"
#include "app/document_access.h"
#include "app/file/file.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/process.h"

namespace app {
namespace crash {

Session::Session(const std::string& path)
  : m_path(path)
  , m_pid(0)
{
}

Session::~Session()
{
}

bool Session::isRunning()
{
  loadPid();
  return base::is_process_running(m_pid);
}

bool Session::isEmpty()
{
  for (auto& item : base::list_files(m_path)) {
    if (base::get_file_extension(item) == "ase")
      return false;
  }
  return true;
}

void Session::create(base::pid pid)
{
  m_pid = pid;

#ifdef _WIN32
  std::ofstream of(base::from_utf8(pidFilename()));
#else
  std::ofstream of(pidFilename());
#endif
  of << m_pid;
}

void Session::removeFromDisk()
{
  base::delete_file(pidFilename());
  base::remove_directory(m_path);
}

void Session::saveDocumentChanges(app::Document* doc)
{
  DocumentReader reader(doc);
  DocumentWriter writer(reader);
  app::Context ctx;
  std::string fn = base::join_path(m_path,
    base::convert_to<std::string>(doc->id()) + ".ase");
  TRACE("DataRecovery: Saving document '%s'...\n", fn.c_str());

  FileOp* fop = fop_to_save_document(&ctx,
    static_cast<app::Document*>(doc), fn.c_str(), "");
  if (!fop) {
    TRACE("DataRecovery: Cannot create save file operation\n");
    return;
  }

  fop_operate(fop, NULL);
  fop_done(fop);
  if (fop->has_error())
    TRACE("DataRecovery: Error saving changes '%s'\n", fop->error.c_str());
  fop_free(fop);
}

void Session::loadPid()
{
  if (m_pid)
    return;

  std::string pidfile = pidFilename();
  if (base::is_file(pidfile)) {
    std::ifstream pf(pidfile);
    if (pf)
      pf >> m_pid;
  }
}

std::string Session::pidFilename() const
{
  return base::join_path(m_path, "pid");
}

std::string Session::dataFilename() const
{
  return base::join_path(m_path, "data");
}

} // namespace crash
} // namespace app
