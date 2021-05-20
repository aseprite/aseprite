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

  typedef enum {
    UsedColors,
    UnusedColors,
    UsedTiles,
    UnusedTiles
  } Modifier;

  SelectPaletteColorsCommand();

protected:
  bool onEnabled(Context* context) override;
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

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
  TilemapMode tilemapMode = context->activeSite().tilemapMode();
  if (m_modifier == Modifier::UsedColors || m_modifier == Modifier::UnusedColors)
    return (tilemapMode == TilemapMode::Pixels) ? true : false;
  else
    return (tilemapMode == TilemapMode::Tiles) ? true : false;
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
  int tilesetSize = 0;
  SelectedFrames::const_iterator selected_frames_it = selectedFrames.begin();
  SelectedFrames::const_iterator selected_frames_end = selectedFrames.end();
  CelList tilemapCels;
  if (!currentTileset || !site->layer() || !layer->isTilemap())
    return false;

  for (;selected_frames_it != selected_frames_end; ++selected_frames_it) {
    int frame = *selected_frames_it;
    if (layer->cel(frame))
      tilemapCels.push_back(layer->cel(frame));
  }
  tilesetSize = currentTileset->size();
  if (usedTilesIndices.size() != tilesetSize)
    usedTilesIndices.resize(tilesetSize);
  usedTilesIndices.clear();

  if (tilemapCels.size() <=  0 || tilesetSize <= 0)
    return false;

  tile_index i = 0;
  for (; i<tilesetSize; ++i) {
    for (auto cel : tilemapCels) {
      bool skiptilesetIndex = false;
      for (const doc::tile_t t : LockImageBits<TilemapTraits>(cel->imageRef().get())) {
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
  TilemapMode tilemapMode = context->activeSite().tilemapMode();
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

  if (tilemapMode == TilemapMode::Pixels) {
    doc::OctreeMap octreemap;
    SelectedFrames::const_iterator selected_frames_it = selectedFrames.begin();
    SelectedFrames::const_iterator selected_frames_end = selectedFrames.end();
    const doc::Palette* currentPalette = get_current_palette();
    PalettePicks usedEntries(currentPalette->size());

    // Loop throught selected layers and frames:
    for (auto& layer : selectedLayers) {
      selected_frames_it = selectedFrames.begin();
      for (;selected_frames_it != selected_frames_end; ++selected_frames_it) {
        int frame = *selected_frames_it;
        if (layer->cel(frame) && layer->cel(frame)->image()) {
          Image* image = layer->cel(frame)->image();
          // Ordinary layer case:
          if (!layer->isTilemap()) {
            // INDEXED case:
            if (image->pixelFormat() == IMAGE_INDEXED) {
              doc::for_each_pixel<IndexedTraits>(
                image,
                [&usedEntries](const color_t p) {
                  usedEntries[p] = true;
                });
            }
            // RGB / GRAYSCALE case:
            else if (image->pixelFormat() == IMAGE_RGB ||
                     image->pixelFormat() == IMAGE_GRAYSCALE)
              octreemap.feedWithImage(image, image->maskColor(), 8);
            else
              ASSERT(false);
          }
          // Tilemap layer case:
          else if (layer->isTilemap()) {
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
              if (usedTiles[i]) {
                // The tileset format is INDEXED:
                if (tileset->get(i).get()->pixelFormat() == IMAGE_INDEXED) {
                  // Looking pixel by pixel in each usedTiles
                  doc::for_each_pixel<IndexedTraits>(
                    image,
                    [&usedEntries](const color_t p) {
                      usedEntries[p] = true;
                    });
                }
                // The tileset format is RGB / GRAYSCALE:
                else if (tileset->get(i).get()->pixelFormat() == IMAGE_RGB ||
                         tileset->get(i).get()->pixelFormat() == IMAGE_GRAYSCALE)
                  octreemap.feedWithImage(tileset->get(i).get(), image->maskColor(), 8);
                else
                  ASSERT(false);
              }
            }
          }
          else
            ASSERT(false);
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

    if (m_modifier == Modifier::UsedColors) {
      context->setSelectedColors(usedEntries);
    }
    else if (m_modifier == Modifier::UnusedColors) {
      for (int i=0; i<usedEntries.size(); ++i)
        usedEntries[i] = !usedEntries[i];
      context->setSelectedColors(usedEntries);
    }
    else
      ASSERT(false);
  }
  else { // tilemapMode == TilemapMode::Tiles
    if (!site.tileset())
      return;
    PalettePicks usedTileIndices(site.tileset()->size());
    selectTiles(sprite, &site, usedTileIndices, selectedFrames);
    if (m_modifier == Modifier::UsedTiles)
      context->setSelectedTiles(usedTileIndices);
    else if (m_modifier == Modifier::UnusedTiles) {
      for (int i=0; i<usedTileIndices.size(); ++i)
        usedTileIndices[i] = !usedTileIndices[i];
      context->setSelectedTiles(usedTileIndices);
    }
    else
      ASSERT(false);
  }
}

Command* CommandFactory::createSelectPaletteColorsCommand()
{
  return new SelectPaletteColorsCommand;
}

} // namespace app
