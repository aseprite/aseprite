// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
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
#include "app/crash/log.h"
#include "app/crash/read_document.h"
#include "app/crash/recovery_config.h"
#include "app/crash/write_document.h"
#include "app/doc.h"
#include "app/doc_access.h"
#include "app/file/file.h"
#include "app/ui_context.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/process.h"
#include "base/split_string.h"
#include "base/string.h"
#include "base/thread.h"
#include "base/time.h"
#include "doc/cancel_io.h"
#include "fmt/format.h"
#include "ver/info.h"

namespace app {
namespace crash {

static const char* kPidFilename = "pid";   // Process ID running the session (or non-existent if the PID was closed correctly)
static const char* kVerFilename = "ver";   // File that indicates the Aseprite version used in the session
static const char* kOpenFilename = "open"; // File that indicates if the document is/was open in the session (or non-existent if the document was closed correctly)

Session::Backup::Backup(const std::string& dir)
  : m_dir(dir)
{
  DocumentInfo info;
  read_document_info(dir, info);

  m_fn = info.filename;
  m_desc =
    fmt::format("{} Sprite {}x{}, {} {}",
                info.mode == ColorMode::RGB ? "RGB":
                info.mode == ColorMode::GRAYSCALE ? "Grayscale":
                info.mode == ColorMode::INDEXED ? "Indexed":
                info.mode == ColorMode::BITMAP ? "Bitmap": "Unknown",
                info.width, info.height, info.frames,
                info.frames == 1 ? "frame": "frames");
}

std::string Session::Backup::description(const bool withFullPath) const
{
  return fmt::format("{}: {}",
                     m_desc,
                     withFullPath ? m_fn:
                                    base::get_file_name(m_fn));
}

Session::Session(RecoveryConfig* config,
                 const std::string& path)
  : m_pid(0)
  , m_path(path)
  , m_config(config)
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
        m_backups.push_back(std::make_shared<Backup>(docDir));
      }
    }
  }
  return m_backups;
}

bool Session::isRunning()
{
  loadPid();
  if (m_pid)
    return base::is_process_running(m_pid);
  else
    return false;
}

bool Session::isCrashedSession()
{
  loadPid();
  return (m_pid != 0);
}

bool Session::isOldSession()
{
  if (m_config->keepEditedSpriteDataFor <= 0)
    return true;

  std::string verfile = verFilename();
  if (!base::is_file(verfile))
    return true;

  int lifespanDays = m_config->keepEditedSpriteDataFor;
  base::Time sessionTime = base::get_modification_time(verfile);

  return (sessionTime.addDays(lifespanDays) < base::current_time());
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
  verf << get_app_version();
}

void Session::close()
{
  try {
    // Just remove the PID file to indicate that this session was
    // correctly closed
    if (base::is_file(pidFilename()))
      base::delete_file(pidFilename());

    // If we don't have to keep the sprite data, just remove it from
    // the disk.
    if (m_config->keepEditedSpriteDataFor == 0)
      removeFromDisk();
  }
  catch (const std::exception&) {
    // TODO Log this error
  }
}

void Session::removeFromDisk()
{
  try {
    // Remove all backups from disk
    Backups baks = backups();
    for (const BackupPtr& bak : baks)
      deleteBackup(bak);

    if (base::is_file(pidFilename()))
      base::delete_file(pidFilename());

    if (base::is_file(verFilename()))
      base::delete_file(verFilename());

    base::remove_directory(m_path);
  }
  catch (const std::exception& ex) {
    (void)ex;
    LOG(ERROR, "RECO: Session directory cannot be removed, it's not empty.\n"
               "      Error: %s\n", ex.what());
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
  RECO_TRACE("RECO: Saving document '%s'...\n", dir.c_str());

  // Create directory for document
  if (!base::is_directory(dir))
    base::make_directory(dir);

  // Create "open" file to indicate that the document is open in this session
  {
    std::string openfile = base::join_path(dir, kOpenFilename);
    if (!base::is_file(openfile)) {
      std::ofstream of(FSTREAM_PATH(openfile));
      if (of)
        of << "open";
    }
  }

  // Save document information
  return write_document(dir, doc, &reader);
}

void Session::removeDocument(Doc* doc)
{
  try {
    delete_document_internals(doc);

    markDocumentAsCorrectlyClosed(doc);
  }
  catch (const std::exception& ex) {
    LOG(FATAL, "Exception deleting document %s\n", ex.what());
  }
}

Doc* Session::restoreBackupDoc(const std::string& backupDir,
                               base::task_token* t)
{
  Console console;
  try {
    Doc* doc = read_document(backupDir, t);
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

Doc* Session::restoreBackupDoc(const BackupPtr& backup,
                               base::task_token* t)
{
  return restoreBackupDoc(backup->dir(), t);
}

Doc* Session::restoreBackupById(const doc::ObjectId id,
                                base::task_token* t)
{
  std::string docDir = base::join_path(m_path, base::convert_to<std::string>(int(id)));
  if (base::is_directory(docDir))
    return restoreBackupDoc(docDir, t);
  else
    return nullptr;
}

Doc* Session::restoreBackupDocById(const doc::ObjectId id,
                                   base::task_token* t)
{
  std::string docDir = base::join_path(m_path, base::convert_to<std::string>(int(id)));
  if (!base::is_directory(docDir))
    return nullptr;

  return restoreBackupDoc(docDir, t);
}

Doc* Session::restoreBackupRawImages(const BackupPtr& backup,
                                     const RawImagesAs as,
                                     base::task_token* t)
{
  Console console;
  try {
    Doc* doc = read_document_with_raw_images(backup->dir(), as, t);
    if (doc) {
      if (isCrashedSession())
        fixFilename(doc);
    }
    return doc;
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
  }
  return nullptr;
}

void Session::deleteBackup(const BackupPtr& backup)
{
  auto it = std::find(m_backups.begin(), m_backups.end(), backup);
  ASSERT(it != m_backups.end());
  if (it != m_backups.end())
    m_backups.erase(it);

  if (base::is_directory(backup->dir()))
    deleteDirectory(backup->dir());
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
  return base::join_path(m_path, kPidFilename);
}

std::string Session::verFilename() const
{
  return base::join_path(m_path, kVerFilename);
}

void Session::markDocumentAsCorrectlyClosed(app::Doc* doc)
{
  std::string dir = base::join_path(
    m_path, base::convert_to<std::string>(doc->id()));

  ASSERT(!dir.empty());
  if (dir.empty() || !base::is_directory(dir))
    return;

  std::string openFn = base::join_path(dir, kOpenFilename);
  if (base::is_file(openFn)) {
    RECO_TRACE("RECO: Document was closed correctly, deleting file '%s'\n", openFn.c_str());
    base::delete_file(openFn);
  }
}

void Session::deleteDirectory(const std::string& dir)
{
  ASSERT(!dir.empty());
  if (dir.empty())
    return;

  for (auto& item : base::list_files(dir)) {
    std::string objfn = base::join_path(dir, item);
    if (base::is_file(objfn)) {
      RECO_TRACE("RECO: Deleting file '%s'\n", objfn.c_str());
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
