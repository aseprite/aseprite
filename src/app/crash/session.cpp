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

#include "base/fs.h"
#include "base/path.h"
#include "base/process.h"

#include <windows.h>

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
  return !base::is_file(dataFilename());
}

void Session::create(base::pid pid)
{
  m_pid = pid;

  std::ofstream of(pidFilename());
  of << m_pid;
}

void Session::removeFromDisk()
{
  base::delete_file(pidFilename());
  base::remove_directory(m_path);
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
