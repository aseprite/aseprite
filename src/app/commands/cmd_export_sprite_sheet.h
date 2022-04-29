// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#define APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#pragma once

#include "app/commands/new_params.h"
#include "app/sprite_sheet_data_format.h"
#include "app/sprite_sheet_type.h"

#include <limits>
#include <sstream>

namespace app {

struct ExportSpriteSheetParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<bool> askOverwrite { this, true, { "askOverwrite", "ask-overwrite" } };
  Param<app::SpriteSheetType> type { this, app::SpriteSheetType::None, "type" };
  Param<int> columns { this, 0, "columns" };
  Param<int> rows { this, 0, "rows" };
  Param<int> width { this, 0, "width" };
  Param<int> height { this, 0, "height" };
  Param<std::string> textureFilename { this, std::string(), "textureFilename" };
  Param<std::string> dataFilename { this, std::string(), "dataFilename" };
  Param<SpriteSheetDataFormat> dataFormat { this, SpriteSheetDataFormat::Default, "dataFormat" };
  Param<std::string> filenameFormat { this, std::string(), "filenameFormat" };
  Param<std::string> tagnameFormat { this, std::string(), "tagnameFormat" };
  Param<int> borderPadding { this, 0, "borderPadding" };
  Param<int> shapePadding { this, 0, "shapePadding" };
  Param<int> innerPadding { this, 0, "innerPadding" };
  Param<bool> trimSprite { this, false, "trimSprite" };
  Param<bool> trim { this, false, "trim" };
  Param<bool> trimByGrid { this, false, "trimByGrid" };
  Param<bool> extrude { this, false, "extrude" };
  Param<bool> ignoreEmpty { this, false, "ignoreEmpty" };
  Param<bool> mergeDuplicates { this, false, "mergeDuplicates" };
  Param<bool> openGenerated { this, false, "openGenerated" };
  Param<std::string> layer { this, std::string(), "layer" };
  // TODO The layerIndex parameter is for internal use only, layers
  //      are counted in the same order as they are displayed in the
  //      Timeline or in the Export Sprite Sheet combobox. But this
  //      index is different to the one specified in the .aseprite
  //      file spec (where layers are counted from bottom to top).
  Param<int> layerIndex { this, -1, "_layerIndex" };
  Param<std::string> tag { this, std::string(), "tag" };
  Param<bool> splitLayers { this, false, "splitLayers" };
  Param<bool> splitTags { this, false, "splitTags" };
  Param<bool> splitGrid { this, false, "splitGrid" };
  Param<bool> listLayers { this, true, "listLayers" };
  Param<bool> listTags { this, true, "listTags" };
  Param<bool> listSlices { this, true, "listSlices" };
  Param<bool> fromTilesets { this, false, "fromTilesets" };
};

class ExportSpriteSheetCommand : public CommandWithNewParams<ExportSpriteSheetParams> {
public:
  ExportSpriteSheetCommand(const char* id = CommandId::ExportSpriteSheet());
protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

} // namespace app

#endif // APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
