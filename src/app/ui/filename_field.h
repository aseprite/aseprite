// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FILENAME_FIELD_H_INCLUDED
#define APP_UI_FILENAME_FIELD_H_INCLUDED
#pragma once

#include "obs/connection.h"
#include "obs/signal.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/entry.h"

#include <string>

namespace app {

class FilenameField : public ui::HBox {
public:
  enum Type { EntryAndButton, ButtonOnly };

  FilenameField(const Type type, const std::string& pathAndFilename);

  std::string filepath() const { return m_path; }
  std::string filename() const { return m_file; }
  std::string fullFilename() const;
  std::string displayedFilename() const;
  bool askOverwrite() const { return m_askOverwrite; }
  void setFilename(const std::string& pathAndFilename);
  void setFilenameQuiet(const std::string& fn) { m_file = fn; }
  void setDocFilename(const std::string& fn) { m_docFilename = fn; }
  void setAskOverwrite(const bool on) { m_askOverwrite = on; }
  void onUpdateText();

  obs::signal<std::string()> SelectOutputFile;
  obs::signal<void()> Change;

protected:
  bool onProcessMessage(ui::Message* msg) override;
  void onInitTheme(ui::InitThemeEvent& ev) override;
  void onSetEditFullPath();

private:
  void setEditFullPath(const bool on);
  void updateWidgets();
  void onBrowse();
  std::string updatedFilename() const;

  std::string m_path;
  std::string m_pathBase;
  std::string m_file;
  std::string m_docFilename;
  ui::Entry* m_entry;
  ui::Button m_button;
  bool m_editFullPath;
  bool m_askOverwrite;

  obs::scoped_connection m_editFullPathChangeConn;
};

} // namespace app

#endif
