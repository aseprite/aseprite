// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CLI_DEFAULT_CLI_DELEGATE_H_INCLUDED
#define APP_CLI_DEFAULT_CLI_DELEGATE_H_INCLUDED
#pragma once

#include "app/cli/cli_delegate.h"

namespace app {

  class DefaultCliDelegate : public CliDelegate {
  public:
    void showHelp(const AppOptions& programOptions) override;
    void showVersion() override;
    void afterOpenFile(const CliOpenFile& cof) override;
    void saveFile(const CliOpenFile& cof) override;
    void exportFiles(DocumentExporter& exporter) override;
    void execScript(const std::string& filename) override;
  };

} // namespace app

#endif
