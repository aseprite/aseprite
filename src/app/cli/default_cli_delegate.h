// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
    void saveFile(Context* ctx, const CliOpenFile& cof) override;
    void loadPalette(Context* ctx, const CliOpenFile& cof, const std::string& filename) override;
    void exportFiles(Context* ctx, DocExporter& exporter) override;
#ifdef ENABLE_SCRIPTING
    void execScript(const std::string& filename,
                    const Params& params) override;
#endif
  };

} // namespace app

#endif
