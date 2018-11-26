// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/session.h"

#include "app/console.h"
#include "app/context.h"
#include "app/crash/read_document.h"
#include "app/crash/write_document.h"
#include "app/doc.h"
#include "app/doc_access.h"
#include "app/file/file.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/process.h"
#include "base/split_string.h"
#include "base/string.h"
#include "doc/cancel_io.h"

namespace app {
namespace crash {

Session::Backup::Backup(const std::string& dir)
  : m_dir(dir)
{
  DocumentInfo info;
  read_document_info(dir, info);

  std::vector<char> buf(1024);
  sprintf(&buf[0], "%s Sprite %dx%d, %d %s: %s",
    info.mode == ColorMode::RGB ? "RGB":
    info.mode == ColorMode::GRAYSCALE ? "Grayscale":
    info.mode == ColorMode::INDEXED ? "Indexed":
    info.mode == ColorMode::BITMAP ? "Bitmap": "Unknown",
    info.width, info.height, info.frames,
    info.frames == 1 ? "frame": "frames",
    info.filename.c_str());
  m_desc = &buf[0];
}

Session::Session(const std::string& path)
  : m_pid(0)
  , m_path(path)
{
}

Session::~Session()
{
}

std::string Session::name() const
{
  std::string name = base::get_file_title(m_path);
  std::vector<std::string> parts;
  base::split_string(name, parts, "-");

  if (parts.size() == 3) {
    if (parts[0].size() == 4+2+2) { // YYYYMMDD -> YYYY-MM-DD
      parts[0].insert(6, 1, '-');
      parts[0].insert(4, 1, '-');
    }
    if (parts[1].size() == 2+2+2) { // HHMMSS -> HH:MM.SS
      parts[1].insert(4, 1, '.');
      parts[1].insert(2, 1, ':');
    }
    return "Session date: " + parts[0] + " time: " + parts[1] + " (PID " + parts[2] + ")";
  }
  else
    return name;
}

std::string Session::version()
{
  if (m_version.empty()) {
    std::string verfile = verFilename();
    if (base::is_file(verfile)) {
      std::ifstream pf(FSTREAM_PATH(verfile));
      if (pf)
        pf >> m_version;
    }
  }
  return m_version;
}

const Session::Backups& Session::backups()
{
  if (m_backups.empty()) {
    for (auto& item : base::list_files(m_path)) {
      std::string docDir = base::join_path(m_path, item);
      if (base::is_directory(docDir)) {
        m_backups.push_back(new Backup(docDir));
      }
    }
  }
  return m_backups;
}

bool Session::isRunning()
{
  loadPid();
  return base::is_process_running(m_pid);
}

bool Session::isEmpty()
{
  for (auto& item : base::list_files(m_path)) {
    if (base::is_directory(base::join_path(m_path, item)))
      return false;
  }
  return true;
}

void Session::create(base::pid pid)
{
  m_pid = pid;

  std::ofstream pidf(FSTREAM_PATH(pidFilename()));
  std::ofstream verf(FSTREAM_PATH(verFilename()));

  pidf << m_pid;
  verf << VERSION;
}

void Session::removeFromDisk()
{
  try {
    if (base::is_file(pidFilename()))
      base::delete_file(pidFilename());

    if (base::is_file(verFilename()))
      base::delete_file(verFilename());

    base::remove_directory(m_path);
  }
  catch (const std::exception& ex) {
    (void)ex;
    LOG(ERROR) << "RECO: Session directory cannot be removed, it's not empty.\n"
               << "      Error: " << ex.what() << "\n";
  }
}

class CustomWeakDocReader : public WeakDocReader
                          , public doc::CancelIO {
public:
  explicit CustomWeakDocReader(Doc* doc)
    : WeakDocReader(doc) {
  }

  // CancelIO impl
  bool isCanceled() override {
    return !isLocked();
  }
};

bool Session::saveDocumentChanges(Doc* doc)
{
  CustomWeakDocReader reader(doc);
  if (!reader.isLocked())
    return false;

  app::Context ctx;
  std::string dir = base::join_path(m_path,
    base::convert_to<std::string>(doc->id()));
  TRACE("RECO: Saving document '%s'...\n", dir.c_str());

  if (!base::is_directory(dir))
    base::make_directory(dir);

  // Save document information
  return write_document(dir, doc, &reader);
}

void Session::removeDocument(Doc* doc)
{
  try {
    delete_document_internals(doc);

    // Delete document backup directory
    std::string dir = base::join_path(m_path,
      base::convert_to<std::string>(doc->id()));
    if (base::is_directory(dir))
      deleteDirectory(dir);
  }
  catch (const std::exception&) {
    // TODO Log this error
  }
}

Doc* Session::restoreBackupDoc(const std::string& backupDir)
{
  Console console;
  try {
    Doc* doc = read_document(backupDir);
    if (doc) {
      fixFilename(doc);
      return doc;
    }
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
  }
  return nullptr;
}

void Session::restoreBackup(Backup* backup)
{
  Doc* doc = restoreBackupDoc(backup->dir());
  if (doc)
    UIContext::instance()->documents().add(doc);
}

void Session::restoreBackupById(const ObjectId id)
{
  std::string docDir = base::join_path(m_path, base::convert_to<std::string>(int(id)));
  if (!base::is_directory(docDir))
    return;

  Doc* doc = restoreBackupDoc(docDir);
  if (doc)
    UIContext::instance()->documents().add(doc);
}

Doc* Session::restoreBackupDocById(const doc::ObjectId id)
{
  std::string docDir = base::join_path(m_path, base::convert_to<std::string>(int(id)));
  if (!base::is_directory(docDir))
    return nullptr;

  return restoreBackupDoc(docDir);
}

void Session::restoreRawImages(Backup* backup, RawImagesAs as)
{
  Console console;
  try {
    Doc* doc = read_document_with_raw_images(backup->dir(), as);
    if (doc) {
      fixFilename(doc);
      UIContext::instance()->documents().add(doc);
    }
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
  }
}

void Session::deleteBackup(Backup* backup)
{
  try {
    auto it = std::find(m_backups.begin(), m_backups.end(), backup);
    ASSERT(it != m_backups.end());
    if (it != m_backups.end())
      m_backups.erase(it);

    if (base::is_directory(backup->dir()))
      deleteDirectory(backup->dir());
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
  }
}

void Session::loadPid()
{
  if (m_pid)
    return;

  std::string pidfile = pidFilename();
  if (base::is_file(pidfile)) {
    std::ifstream pf(FSTREAM_PATH(pidfile));
    if (pf)
      pf >> m_pid;
  }
}

std::string Session::pidFilename() const
{
  return base::join_path(m_path, "pid");
}

std::string Session::verFilename() const
{
  return base::join_path(m_path, "ver");
}

void Session::deleteDirectory(const std::string& dir)
{
  ASSERT(!dir.empty());
  if (dir.empty())
    return;

  for (auto& item : base::list_files(dir)) {
    std::string objfn = base::join_path(dir, item);
    if (base::is_file(objfn)) {
      TRACE("RECO: Deleting file '%s'\n", objfn.c_str());
      base::delete_file(objfn);
    }
  }
  base::remove_directory(dir);
}

void Session::fixFilename(Doc* doc)
{
  std::string fn = doc->filename();
  if (fn.empty())
    return;

  std::string ext = base::get_file_extension(fn);
  if (!ext.empty())
    ext = "." + ext;

  doc->setFilename(
    base::join_path(
      base::get_file_path(fn),
      base::get_file_title(fn) + "-Recovered" + ext));
}

} // namespace crash
} // namespace app
