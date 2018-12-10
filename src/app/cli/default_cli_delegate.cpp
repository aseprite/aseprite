// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cli/default_cli_delegate.h"

#include "app/cli/app_options.h"
#include "app/cli/cli_open_file.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/doc.h"
#include "app/doc_exporter.h"
#include "app/file/palette_file.h"
#include "app/ui_context.h"
#include "base/convert_to.h"
#include "doc/frame_tag.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/slice.h"
#include "doc/sprite.h"

#ifdef ENABLE_SCRIPTING
  #include "app/app.h"
  #include "app/script/engine.h"
#endif

#include <iostream>
#include <memory>

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
    for (doc::Layer* layer : cof.document->sprite()->allVisibleLayers())
      std::cout << layer->name() << "\n";
  }

  if (cof.listTags) {
    for (doc::FrameTag* tag : cof.document->sprite()->frameTags())
      std::cout << tag->name() << "\n";
  }

  if (cof.listSlices) {
    for (doc::Slice* slice : cof.document->sprite()->slices())
      std::cout << slice->name() << "\n";
  }
}

void DefaultCliDelegate::saveFile(Context* ctx, const CliOpenFile& cof)
{
  Command* saveAsCommand = Commands::instance()->byId(CommandId::SaveFileCopyAs());
  Params params;
  params.set("filename", cof.filename.c_str());
  params.set("filename-format", cof.filenameFormat.c_str());

  if (cof.hasFrameTag()) {
    params.set("frame-tag", cof.frameTag.c_str());
  }
  if (cof.hasFrameRange()) {
    params.set("from-frame", base::convert_to<std::string>(cof.fromFrame).c_str());
    params.set("to-frame", base::convert_to<std::string>(cof.toFrame).c_str());
  }
  if (cof.hasSlice()) {
    params.set("slice", cof.slice.c_str());
  }

  if (cof.ignoreEmpty)
    params.set("ignoreEmpty", "true");

  ctx->executeCommand(saveAsCommand, params);
}

void DefaultCliDelegate::loadPalette(Context* ctx,
                                     const CliOpenFile& cof,
                                     const std::string& filename)
{
  std::unique_ptr<doc::Palette> palette(load_palette(filename.c_str()));
  if (palette) {
    Command* loadPalCommand = Commands::instance()->byId(CommandId::LoadPalette());
    Params params;
    params.set("filename", filename.c_str());

    ctx->executeCommand(loadPalCommand, params);
  }
  else {
    Console().printf("Error loading palette in --palette '%s'\n",
                     filename.c_str());
  }
}

void DefaultCliDelegate::exportFiles(Context* ctx, DocExporter& exporter)
{
  LOG("APP: Exporting sheet...\n");

  std::unique_ptr<Doc> spriteSheet(exporter.exportSheet(ctx));

  // Sprite sheet isn't used, we just delete it.

  LOG("APP: Export sprite sheet: Done\n");
}

#ifdef ENABLE_SCRIPTING
void DefaultCliDelegate::execScript(const std::string& filename,
                                    const Params& params)
{
  if (!App::instance()->scriptEngine()->evalFile(filename, params))
    throw std::runtime_error("Error executing script");
}
#endif // ENABLE_SCRIPTING

} // namespace app
