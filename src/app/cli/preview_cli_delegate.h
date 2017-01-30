// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CLI_PREVIEW_CLI_DELEGATE_H_INCLUDED
#define APP_CLI_PREVIEW_CLI_DELEGATE_H_INCLUDED
#pragma once

#include "app/cli/cli_delegate.h"

namespace app {

  class PreviewCliDelegate : public CliDelegate {
  public:
    void showHelp(const AppOptions& programOptions) override;
    void showVersion() override;
    void uiMode() override;
    void shellMode() override;
    void batchMode() override;
    void beforeOpenFile(const CliOpenFile& cof) override;
    void afterOpenFile(const CliOpenFile& cof) override;
    void saveFile(const CliOpenFile& cof) override;
    void loadPalette(const CliOpenFile& cof,
                     const std::string& filename) override;
    void exportFiles(DocumentExporter& exporter) override;
    void execScript(const std::string& filename) override;

  private:
    void showLayersFilter(const CliOpenFile& cof);
  };

} // namespace app

#endif
