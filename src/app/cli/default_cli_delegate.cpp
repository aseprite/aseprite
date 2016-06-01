// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cli/default_cli_delegate.h"

#include "app/cli/app_options.h"
#include "app/cli/cli_open_file.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/document_exporter.h"
#include "app/script/app_scripting.h"
#include "app/ui_context.h"
#include "base/convert_to.h"
#include "doc/frame_tag.h"
#include "doc/layer.h"
#include "doc/layers_range.h"
#include "doc/sprite.h"
#include "script/engine_delegate.h"

#include <iostream>

namespace app {

void DefaultCliDelegate::showHelp(const AppOptions& options)
{
  std::cout
    << PACKAGE << " v" << VERSION << " | A pixel art program\n" << COPYRIGHT
    << "\n\nUsage:\n"
    << "  " << options.exeName() << " [OPTIONS] [FILES]...\n\n"
    << "Options:\n"
    << options.programOptions()
    << "\nFind more information in " << PACKAGE
    << " web site: " << WEBSITE << "\n\n";
}

void DefaultCliDelegate::showVersion()
{
  std::cout << PACKAGE << ' ' << VERSION << '\n';
}

void DefaultCliDelegate::afterOpenFile(const CliOpenFile& cof)
{
  if (!cof.document)            // Do nothing
    return;

  if (cof.listLayers) {
    for (doc::Layer* layer : cof.document->sprite()->layers()) {
      if (layer->isVisible())
        std::cout << layer->name() << "\n";
    }
  }

  if (cof.listTags) {
    for (doc::FrameTag* tag : cof.document->sprite()->frameTags())
      std::cout << tag->name() << "\n";
  }
}

void DefaultCliDelegate::saveFile(const CliOpenFile& cof)
{
  Context* ctx = UIContext::instance();
  Command* saveAsCommand = CommandsModule::instance()->getCommandByName(CommandId::SaveFileCopyAs);
  Params params;
  params.set("filename", cof.filename.c_str());
  params.set("filename-format", cof.filenameFormat.c_str());

  if (cof.hasFrameTag()) {
    params.set("frame-tag", cof.frameTag.c_str());
  }
  else if (cof.hasFrameRange()) {
    params.set("from-frame", base::convert_to<std::string>(cof.fromFrame).c_str());
    params.set("to-frame", base::convert_to<std::string>(cof.toFrame).c_str());
  }

  ctx->executeCommand(saveAsCommand, params);
}

void DefaultCliDelegate::exportFiles(DocumentExporter& exporter)
{
  LOG("Exporting sheet...\n");

  base::UniquePtr<app::Document> spriteSheet(exporter.exportSheet());

  // Sprite sheet isn't used, we just delete it.

  LOG("Export sprite sheet: Done\n");
}

void DefaultCliDelegate::execScript(const std::string& filename)
{
  script::StdoutEngineDelegate delegate;
  AppScripting engine(&delegate);
  engine.evalFile(filename);
}

} // namespace app
