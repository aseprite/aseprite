// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cli/preview_cli_delegate.h"

#include "app/cli/cli_open_file.h"
#include "app/document.h"
#include "app/document_exporter.h"
#include "base/fs.h"
#include "doc/sprite.h"

#include <iostream>

namespace app {

void PreviewCliDelegate::showHelp(const AppOptions& options)
{
  std::cout << "- Show " PACKAGE " CLI usage\n";
}

void PreviewCliDelegate::showVersion()
{
  std::cout << "- Show " PACKAGE " version\n";
}

void PreviewCliDelegate::uiMode()
{
  std::cout << "- Run UI mode\n";
}

void PreviewCliDelegate::shellMode()
{
  std::cout << "- Run shell mode\n";
}

void PreviewCliDelegate::batchMode()
{
  std::cout << "- Exit\n";
}

void PreviewCliDelegate::beforeOpenFile(const CliOpenFile& cof)
{
  std::cout << "- Open file '" << cof.filename << "'\n";
}

void PreviewCliDelegate::afterOpenFile(const CliOpenFile& cof)
{
  if (!cof.document) {
    std::cout << "  - WARNING: File not found or error loading file\n";
    return;
  }

  if (cof.listLayers)
    std::cout << "  - Print/list layers\n";

  if (cof.listTags)
    std::cout << "  - Print/list tags\n";

  if (cof.allLayers)
    std::cout << "  - Make all layers visible\n";

  if (cof.splitLayers)
    std::cout << "  - Split layers for future save/export operation\n";

  if (!cof.importLayer.empty())
    std::cout << "  - Make layer '" << cof.importLayer << "' visible only (hide all other layers)\n";
}

void PreviewCliDelegate::saveFile(const CliOpenFile& cof)
{
  ASSERT(cof.document);
  ASSERT(cof.document->sprite());

  std::cout << "- Save file: '" << cof.filename << "'\n";
  std::cout << "  - Size: "
            << cof.document->sprite()->width() << "x"
            << cof.document->sprite()->height() << "\n";

  if (!cof.crop.isEmpty()) {
    std::cout << "  - Crop: '"
              << cof.crop.x << ","
              << cof.crop.y << " "
              << cof.crop.w << "x"
              << cof.crop.h << "\n";
  }

  if (cof.trim) {
    std::cout << "  - Trim\n";
  }

  if (!cof.filenameFormat.empty()) {
    std::cout << "  - Filename format: '"
              << cof.filenameFormat << "'\n";
  }
}

void PreviewCliDelegate::exportFiles(DocumentExporter& exporter)
{
  std::string type = "None";
  switch (exporter.spriteSheetType()) {
    case SpriteSheetType::Horizontal: type = "Horizontal"; break;
    case SpriteSheetType::Vertical:   type = "Vertical";   break;
    case SpriteSheetType::Rows:       type = "Rows";       break;
    case SpriteSheetType::Columns:    type = "Columns";    break;
    case SpriteSheetType::Packed:     type = "Packed";     break;
  }

  gfx::Size size = exporter.calculateSheetSize();
  std::cout << "- Export sprite sheet:\n"
            << "  - Type: " << type << "\n"
            << "  - Size: " << size.w << "x" << size.h << "\n";

  if (!exporter.textureFilename().empty()) {
    std::cout << "  - Save texture file: '"
              << exporter.textureFilename() << "'\n";
  }

  if (!exporter.dataFilename().empty()) {
    std::string format = "Unknown";
    switch (exporter.dataFormat()) {
      case DocumentExporter::JsonHashDataFormat: format = "JSON Hash"; break;
      case DocumentExporter::JsonArrayDataFormat: format = "JSON Array"; break;
    }
    std::cout << "  - Save data file: '" << exporter.dataFilename() << "'\n"
              << "  - Data format: " << format << "\n";

    if (!exporter.filenameFormat().empty()) {
      std::cout << "  - Filename format for JSON items: '"
                << exporter.filenameFormat() << "'\n";
    }
  }
}

void PreviewCliDelegate::execScript(const std::string& filename)
{
  std::cout << "- Run script: '" << filename << "'\n";
}

} // namespace app
