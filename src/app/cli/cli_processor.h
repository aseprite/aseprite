// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_APP_CLI_PROCESSOR_H_INCLUDED
#define APP_APP_CLI_PROCESSOR_H_INCLUDED
#pragma once

#include "app/cli/cli_delegate.h"
#include "app/cli/cli_open_file.h"
#include "app/doc_exporter.h"
#include "app/util/open_batch.h"
#include "doc/selected_layers.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace doc {
class Sprite;
}

namespace app {

class AppOptions;
class Context;
class DocExporter;

class CliProcessor {
public:
  CliProcessor(CliDelegate* delegate, const AppOptions& options);
  int process(Context* ctx);

  // Public so it can be tested
  static void FilterLayers(const doc::Sprite* sprite,
                           // By value because these vectors will be modified inside
                           std::vector<std::string> includes,
                           std::vector<std::string> excludes,
                           doc::SelectedLayers& filteredLayers);

private:
  bool openFile(Context* ctx, CliOpenFile& cof);
  void saveFile(Context* ctx, const CliOpenFile& cof);

  void filterLayers(const doc::Sprite* sprite,
                    const CliOpenFile& cof,
                    doc::SelectedLayers& filteredLayers)
  {
    CliProcessor::FilterLayers(sprite, cof.includeLayers, cof.excludeLayers, filteredLayers);
  }

  CliDelegate* m_delegate;
  const AppOptions& m_options;
  std::unique_ptr<DocExporter> m_exporter;

  // Files already used in the CLI processing (e.g. when used to
  // load a sequence of files) so we don't ask for them again.
  std::set<std::string> m_usedFiles;
  OpenBatchOfFiles m_batch;
};

} // namespace app

#endif
