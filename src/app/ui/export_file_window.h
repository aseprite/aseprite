// Aseprite
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
  class Document;

  class ExportFileWindow : protected app::gen::ExportFile {
  public:
    ExportFileWindow(const Document* doc);

    bool show();
    void savePref();

    std::string outputFilenameValue() const;
    double resizeValue() const;
    std::string layersValue() const;
    std::string framesValue() const;
    bool applyPixelRatio() const;

    obs::signal<std::string()> SelectOutputFile;

  private:
    const Document* m_doc;
    DocumentPreferences& m_docPref;
  };

}

#endif
