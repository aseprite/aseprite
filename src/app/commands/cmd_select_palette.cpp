// Aseprite
// Copyright (C) 2021  Igara Studio SA
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "app/site.h"
#include "doc/cel.h"
#include "doc/frame_range.h"
#include "doc/image_bits.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/octree_map.h"
#include "doc/palette.h"
#include "doc/sprite.h"

namespace app {

using namespace ui;

class SelectPaletteColorsCommand : public Command {
public:
  enum Modifier {
    UsedColors,
    UnusedColors,
    UsedTiles,
    UnusedTiles
  };

  SelectPaletteColorsCommand();

protected:
  bool onEnabled(Context* context) override;
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  bool selectTiles(Sprite* sprite,
                   Site* site,
                   PalettePicks& usedTilesIndices,
                   SelectedFrames& selectedFrames);

  Modifier m_modifier;
};

SelectPaletteColorsCommand::SelectPaletteColorsCommand()
  : Command(CommandId::SelectPaletteColors(), CmdRecordableFlag)
  , m_modifier(Modifier::UsedColors)
{
}

bool SelectPaletteColorsCommand::onEnabled(Context* context)
{
  if (!context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::HasActiveSprite))
    return false;

  if (m_modifier == Modifier::UsedTiles ||
      m_modifier == Modifier::UnusedTiles) {
    Layer* layer = context->activeSite().layer();
    return (layer && layer->isTilemap());
  }

  return true;
}

void SelectPaletteColorsCommand::onLoadParams(const Params& params)
{
  std::string strParam = params.get("modifier");
  if (strParam == "unused_colors")
    m_modifier = Modifier::UnusedColors;
  else if (strParam == "used_tiles")
    m_modifier = Modifier::UsedTiles;
  else if (strParam == "unused_tiles")
    m_modifier = Modifier::UnusedTiles;
  else
    m_modifier = Modifier::UsedColors;
}

bool SelectPaletteColorsCommand::selectTiles(
  Sprite* sprite,
  Site* site,
  PalettePicks& usedTilesIndices,
  SelectedFrames& selectedFrames)
{
  Tileset* currentTileset = site->tileset();
  Layer* layer = site->layer();

  if (!currentTileset || !site->layer() || !layer->isTilemap())
    return false;

  CelList tilemapCels;
  for (frame_t frame : selectedFrames) {
    if (Cel* cel = layer->cel(frame))
      tilemapCels.push_back(cel);
  }

  int tilesetSize = currentTileset->size();
  if (usedTilesIndices.size() != tilesetSize)
    usedTilesIndices.resize(tilesetSize);
  usedTilesIndices.clear();

  if (tilemapCels.size() <=  0 || tilesetSize <= 0)
    return false;

  for (tile_index i=0; i<tilesetSize; ++i) {
    for (auto cel : tilemapCels) {
      bool skiptilesetIndex = false;
      for (const doc::tile_t t : LockImageBits<TilemapTraits>(cel->image())) {
        if (t == i) {
          usedTilesIndices[doc::tile_geti(t)] = true;
          skiptilesetIndex = true;
          break;
        }
      }
      if (skiptilesetIndex)
        break;
    }
  }
  return true;
}

void SelectPaletteColorsCommand::onExecute(Context* context)
{
  Site site = context->activeSite();
  Sprite* sprite = site.sprite();
  DocRange range = site.range();
  SelectedFrames selectedFrames;
  SelectedLayers selectedLayers;
  if (range.type() == DocRange::Type::kNone) {
    // If there isn't a cels range selected, it assumes the whole sprite:
    range.startRange(site.layer(), 0, DocRange::Type::kFrames);
    range.endRange(site.layer(), sprite->lastFrame());
    selectedFrames = range.selectedFrames();
    selectedLayers.selectAllLayers(sprite->root());
  }
  else {
    selectedFrames = range.selectedFrames();
    selectedLayers = range.selectedLayers();
  }

  if (m_modifier == Modifier::UsedColors ||
      m_modifier == Modifier::UnusedColors) {
    doc::OctreeMap octreemap;
    const doc::Palette* currentPalette = get_current_palette();
    PalettePicks usedEntries(currentPalette->size());

    auto countImage = [&octreemap, &usedEntries](const Image* image){
      switch (image->pixelFormat()) {

        case IMAGE_RGB:
        case IMAGE_GRAYSCALE:
          octreemap.feedWithImage(image, image->maskColor(), 8);
          break;

        case IMAGE_INDEXED:
          doc::for_each_pixel<IndexedTraits>(
            image,
            [&usedEntries](const color_t p) {
              usedEntries[p] = true;
            });
          break;
      }
    };

    // Loop throught selected layers and frames
    for (auto& layer : selectedLayers) {
      for (frame_t frame : selectedFrames) {
        const Cel* cel = layer->cel(frame);
        if (cel && cel->image()) {
          const Image* image = cel->image();

          // Tilemap layer case
          if (layer->isTilemap()) {
            Tileset* tileset = static_cast<LayerTilemap*>(layer)->tileset();
            tile_index ti;
            PalettePicks usedTiles(tileset->size());

            // Looking for tiles (available in tileset) used in the tilemap image:
            doc::for_each_pixel<TilemapTraits>(
              image,
              [&usedTiles, &tileset, &ti](const tile_t t) {
                if (tileset->findTileIndex(tileset->get(t), ti))
                  usedTiles[ti] = true;
              });

            // Looking for tile matches in usedTiles. If a tile matches, then
            // search into the tilemap (pixel by pixel) looking for color matches.
            for (int i=0; i<usedTiles.size(); ++i) {
              if (usedTiles[i])
                countImage(tileset->get(i).get());
            }
          }
          // Regular layers
          else {
            countImage(image);
          }
        }
      }
    }

    doc::Palette tempPalette;
    octreemap.makePalette(&tempPalette, std::numeric_limits<int>::max(), 8);

    for (int i=0; i < currentPalette->size(); ++i) {
      if (tempPalette.findExactMatch(currentPalette->getEntry(i))) {
        usedEntries[i] = true;
        continue;
      }
    }

    if (m_modifier == Modifier::UnusedColors) {
      for (int i=0; i<usedEntries.size(); ++i)
        usedEntries[i] = !usedEntries[i];
    }
    context->setSelectedColors(usedEntries);
  }
  else if (m_modifier == Modifier::UsedTiles ||
           m_modifier == Modifier::UnusedTiles) {
    if (!site.tileset())
      return;

    PalettePicks usedTileIndices(site.tileset()->size());
    selectTiles(sprite, &site, usedTileIndices, selectedFrames);

    if (m_modifier == Modifier::UnusedTiles) {
      for (int i=0; i<usedTileIndices.size(); ++i)
        usedTileIndices[i] = !usedTileIndices[i];
    }
    context->setSelectedTiles(usedTileIndices);
  }
}

std::string SelectPaletteColorsCommand::onGetFriendlyName() const
{
  switch (m_modifier) {
    case UsedColors:   return Strings::commands_SelectPaletteColors();
    case UnusedColors: return Strings::commands_SelectPaletteColors_UnusedColors();
    case UsedTiles:    return Strings::commands_SelectPaletteColors_UsedTiles();
    case UnusedTiles:  return Strings::commands_SelectPaletteColors_UnusedTiles();
  }
  return getBaseFriendlyName();
}

Command* CommandFactory::createSelectPaletteColorsCommand()
{
  return new SelectPaletteColorsCommand;
}

} // namespace app
