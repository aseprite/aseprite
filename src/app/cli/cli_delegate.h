// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CLI_CLI_DELEGATE_H_INCLUDED
#define APP_CLI_CLI_DELEGATE_H_INCLUDED
#pragma once

#include <string>

namespace app {

  class AppOptions;
  class DocumentExporter;
  struct CliOpenFile;

  class CliDelegate {
  public:
    virtual ~CliDelegate() { }
    virtual void showHelp(const AppOptions& options) { }
    virtual void showVersion() { }
    virtual void uiMode() { }
    virtual void shellMode() { }
    virtual void batchMode() { }
    virtual void beforeOpenFile(const CliOpenFile& cof) { }
    virtual void afterOpenFile(const CliOpenFile& cof) { }
    virtual void saveFile(const CliOpenFile& cof) { }
    virtual void exportFiles(DocumentExporter& exporter) { }
    virtual void execScript(const std::string& filename) { }
  };

} // namespace app

#endif
