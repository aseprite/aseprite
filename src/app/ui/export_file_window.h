// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EXPORT_FILE_WINDOW_H_INCLUDED
#define APP_UI_EXPORT_FILE_WINDOW_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "obs/signal.h"

#include "export_file.xml.h"

#include <string>

namespace app {
class Doc;

class ExportFileWindow : public app::gen::ExportFile {
public:
  ExportFileWindow(const Doc* doc);

  bool show();
  void savePref();

  std::string outputFilenameValue() const;
  double resizeValue() const;
  std::string areaValue() const;
  std::string layersValue() const;
  int layersIndex() const;
  std::string framesValue() const;
  doc::AniDir aniDirValue() const;
  bool isPlaySubtags() const;
  bool applyPixelRatio() const;
  bool isForTwitter() const;

  void setOutputFilename(const std::string& pathAndFilename);
  void setResizeScale(const double scale);
  void setArea(const std::string& area);
  void setAniDir(const doc::AniDir aniDir);

  obs::signal<std::string()> SelectOutputFile;

private:
  void updateOutputFilenameEntry();
  void onOutputFilenameEntryChange();
  void updateAniDir();
  void updatePlaySubtags();
  void updateAdjustResizeButton();
  void onAdjustResize();
  void onOK();
  std::string defaultExtension() const;

  const Doc* m_doc;
  DocumentPreferences& m_docPref;
  std::string m_outputPath;
  std::string m_outputFilename;
  int m_preferredResize;
};

} // namespace app

#endif
