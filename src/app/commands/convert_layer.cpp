// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_cel.h"
#include "app/cmd/add_layer.h"
#include "app/cmd/add_tileset.h"
#include "app/cmd/background_from_layer.h"
#include "app/cmd/copy_cel.h"
#include "app/cmd/layer_from_background.h"
#include "app/cmd/remove_layer.h"
#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/tileset_selector.h"
#include "app/util/cel_ops.h"
#include "doc/grid.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/tileset.h"

#ifdef ENABLE_SCRIPTING
  #include "app/script/luacpp.h"
#endif

#include "tileset_selector_window.xml.h"

#include <map>

namespace app {

enum class ConvertLayerParam { None, Background, Layer, Tilemap };

template<>
void Param<ConvertLayerParam>::fromString(const std::string& value)
{
  if (value == "background")
    setValue(ConvertLayerParam::Background);
  else if (value == "layer")
    setValue(ConvertLayerParam::Layer);
  else if (value == "tilemap")
    setValue(ConvertLayerParam::Tilemap);
  else
    setValue(ConvertLayerParam::None);
}

#ifdef ENABLE_SCRIPTING
template<>
void Param<ConvertLayerParam>::fromLua(lua_State* L, int index)
{
  fromString(lua_tostring(L, index));
}
#endif // ENABLE_SCRIPTING

struct ConvertLayerParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<ConvertLayerParam> to{ this, ConvertLayerParam::None, "to" };
};

class ConvertLayerCommand : public CommandWithNewParams<ConvertLayerParams> {
public:
  ConvertLayerCommand();

private:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

  void copyCels(Tx& tx, Layer* srcLayer, Layer* newLayer);
};

ConvertLayerCommand::ConvertLayerCommand()
  : CommandWithNewParams<ConvertLayerParams>(CommandId::ConvertLayer(), CmdRecordableFlag)
{
}

bool ConvertLayerCommand::onEnabled(Context* ctx)
{
  if (!ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveSprite |
                       ContextFlags::HasActiveLayer | ContextFlags::ActiveLayerIsVisible |
                       ContextFlags::ActiveLayerIsEditable))
    return false;

  // TODO add support to convert reference layers into regular layers or tilemaps
  if (ctx->checkFlags(ContextFlags::ActiveLayerIsReference))
    return false;

  switch (params().to()) {
    case ConvertLayerParam::Background:
      return
        // Doesn't have a background layer
        !ctx->checkFlags(ContextFlags::HasBackgroundLayer) &&
        // Convert a regular layer or tilemap into background
        ctx->checkFlags(ContextFlags::ActiveLayerIsImage) &&
        // TODO add support for background tliemaps
        !ctx->checkFlags(ContextFlags::ActiveLayerIsTilemap);

    case ConvertLayerParam::Layer:
      return
        // Convert a background layer into a transparent layer
        ctx->checkFlags(ContextFlags::ActiveLayerIsImage | ContextFlags::ActiveLayerIsBackground) ||
        // or a tilemap into a regular layer
        ctx->checkFlags(ContextFlags::ActiveLayerIsTilemap);

    case ConvertLayerParam::Tilemap:
      return ctx->checkFlags(ContextFlags::ActiveLayerIsImage) &&
             !ctx->checkFlags(ContextFlags::ActiveLayerIsTilemap) &&
             // TODO add support for background tliemaps
             !ctx->checkFlags(ContextFlags::ActiveLayerIsBackground);

    default: return false;
  }
}

void ConvertLayerCommand::onExecute(Context* ctx)
{
  Site site = ctx->activeSite();
  Sprite* sprite = site.sprite();
  Layer* srcLayer = site.layer();

  // Unexpected (onEnabled filters this case)
  if (!sprite || !srcLayer)
    return;

  // Default options to convert a layer to a tilemap
  std::string tilesetName;
  int baseIndex = 1;
  tile_flags matchFlags = 0;
  Grid grid0 = site.grid();
  grid0.origin(gfx::Point(0, 0));

  if (params().to() == ConvertLayerParam::Tilemap && ctx->isUIAvailable() && params().ui() &&
      // Background or Transparent Layer -> Tilemap
      (srcLayer->isImage() && (srcLayer->isBackground() || srcLayer->isTransparent()))) {
    TilesetSelector::Info tilesetInfo;
    tilesetInfo.allowNewTileset = true;
    tilesetInfo.allowExistentTileset = false;
    tilesetInfo.grid = grid0;

    gen::TilesetSelectorWindow window;
    TilesetSelector tilesetSel(sprite, tilesetInfo);
    window.tilesetOptions()->addChild(&tilesetSel);
    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    // Save "advanced" options
    tilesetSel.saveAdvancedPreferences();

    tilesetInfo = tilesetSel.getInfo();
    tilesetName = tilesetInfo.name;
    grid0 = tilesetInfo.grid;
    baseIndex = tilesetInfo.baseIndex;
    matchFlags = tilesetInfo.matchFlags;
  }

  ContextWriter writer(ctx);
  Doc* document(writer.document());
  {
    Tx tx(writer, friendlyName());

    switch (params().to()) {
      case ConvertLayerParam::Background:
        // Layer -> Background
        if (srcLayer->isTransparent()) {
          ASSERT(srcLayer->isImage());
          tx(new cmd::BackgroundFromLayer(static_cast<LayerImage*>(srcLayer)));
        }
        // Tilemap -> Background
        else if (srcLayer->isTilemap()) {
          auto newLayer = new LayerImage(sprite);
          newLayer->configureAsBackground();
          newLayer->setName(Strings::commands_NewFile_BackgroundLayer());
          newLayer->setContinuous(srcLayer->isContinuous());
          newLayer->setUserData(srcLayer->userData());
          tx(new cmd::AddLayer(srcLayer->parent(), newLayer, srcLayer));

          CelList srcCels;
          srcLayer->getCels(srcCels);
          for (Cel* srcCel : srcCels)
            create_cel_copy(tx, srcCel, sprite, newLayer, srcCel->frame());

          tx(new cmd::RemoveLayer(srcLayer));
        }
        break;

      case ConvertLayerParam::Layer:
        // Background -> Layer
        if (srcLayer->isBackground()) {
          tx(new cmd::LayerFromBackground(srcLayer));
        }
        // Tilemap -> Layer
        else if (srcLayer->isTilemap()) {
          auto newLayer = new LayerImage(sprite);
          newLayer->setName(srcLayer->name());
          newLayer->setContinuous(srcLayer->isContinuous());
          newLayer->setBlendMode(static_cast<LayerImage*>(srcLayer)->blendMode());
          newLayer->setOpacity(static_cast<LayerImage*>(srcLayer)->opacity());
          newLayer->setUserData(srcLayer->userData());
          tx(new cmd::AddLayer(srcLayer->parent(), newLayer, srcLayer));

          copyCels(tx, srcLayer, newLayer);

          tx(new cmd::RemoveLayer(srcLayer));
        }
        break;

      case ConvertLayerParam::Tilemap:
        // Background or Transparent Layer -> Tilemap
        if (srcLayer->isImage() && (srcLayer->isBackground() || srcLayer->isTransparent())) {
          auto tileset = new Tileset(sprite, grid0, 1);
          tileset->setName(tilesetName);
          tileset->setBaseIndex(baseIndex);
          tileset->setMatchFlags(matchFlags);

          auto addTileset = new cmd::AddTileset(sprite, tileset);
          tx(addTileset);
          tileset_index tsi = addTileset->tilesetIndex();

          auto newLayer = new LayerTilemap(sprite, tsi);
          newLayer->setName(srcLayer->name());
          newLayer->setContinuous(srcLayer->isContinuous());
          newLayer->setBlendMode(static_cast<LayerImage*>(srcLayer)->blendMode());
          newLayer->setOpacity(static_cast<LayerImage*>(srcLayer)->opacity());
          newLayer->setUserData(srcLayer->userData());
          tx(new cmd::AddLayer(srcLayer->parent(), newLayer, srcLayer));

          copyCels(tx, srcLayer, newLayer);

          tx(new cmd::RemoveLayer(srcLayer));
        }
        break;
    }

    tx.commit();
  }

  update_screen_for_document(document);
}

void ConvertLayerCommand::copyCels(Tx& tx, Layer* srcLayer, Layer* newLayer)
{
  std::map<doc::ObjectId, doc::Cel*> linkedCels;

  CelList srcCels;
  srcLayer->getCels(srcCels);
  for (Cel* srcCel : srcCels) {
    frame_t frame = srcCel->frame();

    // Keep linked cels in the new layer
    Cel* linkedSrcCel = srcCel->link();
    if (linkedSrcCel) {
      auto it = linkedCels.find(linkedSrcCel->id());
      if (it != linkedCels.end()) {
        tx(new cmd::CopyCel(newLayer, linkedSrcCel->frame(), newLayer, frame, true));
        continue;
      }
    }

    Cel* newCel = create_cel_copy(tx, srcCel, srcLayer->sprite(), newLayer, frame);
    tx(new cmd::AddCel(newLayer, newCel));

    linkedCels[srcCel->id()] = newCel;
  }
}

std::string ConvertLayerCommand::onGetFriendlyName() const
{
  switch (params().to()) {
    case ConvertLayerParam::Background: return Strings::commands_ConvertLayer_Background(); break;
    case ConvertLayerParam::Layer:      return Strings::commands_ConvertLayer_Layer(); break;
    case ConvertLayerParam::Tilemap:    return Strings::commands_ConvertLayer_Tilemap(); break;
    default:                            return Strings::commands_ConvertLayer();
  }
}

Command* CommandFactory::createConvertLayerCommand()
{
  return new ConvertLayerCommand;
}

} // namespace app
