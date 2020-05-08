// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/move_layer.h"
#include "app/cmd/trim_cel.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/new_params.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/find_widget.h"
#include "app/i18n/strings.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/restore_visible_layers.h"
#include "app/tx.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/new_image_from_mask.h"
#include "app/util/range_utils.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/ui.h"

#include "new_layer.xml.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <cstdlib>

namespace app {

using namespace ui;

struct NewLayerParams : public NewParams {
  Param<std::string> name { this, std::string(), "name" };
  Param<bool> group { this, false, "group" };
  Param<bool> reference { this, false, "reference" };
  Param<bool> ask { this, false, "ask" };
  Param<bool> fromFile { this, false, { "fromFile", "from-file" } };
  Param<bool> fromClipboard { this, false, "fromClipboard" };
  Param<bool> viaCut { this, false, "viaCut" };
  Param<bool> viaCopy { this, false, "viaCopy" };
  Param<bool> top { this, false, "top" };
  Param<bool> before { this, false, "before" };
};

class NewLayerCommand : public CommandWithNewParams<NewLayerParams> {
public:
  enum class Type { Layer, Group, ReferenceLayer };
  enum class Place { AfterActiveLayer, BeforeActiveLayer, Top };

  NewLayerCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  void adjustRefCelBounds(Cel* cel, gfx::RectF bounds);
  std::string getUniqueLayerName(const Sprite* sprite) const;
  int getMaxLayerNum(const Layer* layer) const;
  std::string layerPrefix() const;

  Type m_type;
  Place m_place;
};

NewLayerCommand::NewLayerCommand()
  : CommandWithNewParams(CommandId::NewLayer(), CmdRecordableFlag)
{
}

void NewLayerCommand::onLoadParams(const Params& commandParams)
{
  CommandWithNewParams<NewLayerParams>::onLoadParams(commandParams);

  m_type = Type::Layer;
  if (params().group())
    m_type = Type::Group;
  else if (params().reference())
    m_type = Type::ReferenceLayer;

  m_place = Place::AfterActiveLayer;
  if (params().top())
    m_place = Place::Top;
  else if (params().before())
    m_place = Place::BeforeActiveLayer;
}

bool NewLayerCommand::onEnabled(Context* context)
{
  if (!context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::HasActiveSprite))
    return false;

#ifdef ENABLE_UI
  if (params().fromClipboard() &&
      clipboard::get_current_format() != clipboard::ClipboardImage)
    return false;
#endif

  if ((params().viaCut() ||
       params().viaCopy()) &&
      !context->checkFlags(ContextFlags::HasVisibleMask))
    return false;

  return true;
}

namespace {
class Scoped {                  // TODO move this to base library
public:
  Scoped(const std::function<void()>& func) : m_func(func) { }
  ~Scoped() { m_func(); }
private:
  std::function<void()> m_func;
};
}

void NewLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Site site = context->activeSite();
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  std::string name;

  Doc* pasteDoc = nullptr;
  Scoped destroyPasteDoc(
    [&pasteDoc, context]{
      if (pasteDoc) {
        DocDestroyer destroyer(context, pasteDoc, 100);
        destroyer.destroyDocument();
      }
    });

  // Default name
  if (params().name.isSet())
    name = params().name();
  else
    name = getUniqueLayerName(sprite);

  // Select a file to copy its content
  if (params().fromFile()) {
    Doc* oldActiveDocument = context->activeDocument();
    Command* openFile = Commands::instance()->byId(CommandId::OpenFile());
    Params params;
    params.set("filename", "");
    context->executeCommand(openFile, params);

    // The user have selected another document.
    if (oldActiveDocument != context->activeDocument()) {
      pasteDoc = context->activeDocument();
      static_cast<UIContext*>(context)
        ->setActiveDocument(oldActiveDocument);
    }
    // If the user didn't selected a new document, it means that the
    // file selector dialog was canceled.
    else
      return;
  }

#ifdef ENABLE_UI
  // If params specify to ask the user about the name...
  if (params().ask() && context->isUIAvailable()) {
    // We open the window to ask the name
    app::gen::NewLayer window;
    window.name()->setText(name.c_str());
    window.name()->setMinSize(gfx::Size(128, 0));
    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    name = window.name()->text();
  }
#endif

  LayerGroup* parent = sprite->root();
  Layer* activeLayer = writer.layer();
  SelectedLayers selLayers = site.selectedLayers();
  if (activeLayer) {
    if (activeLayer->isGroup() &&
        activeLayer->isExpanded() &&
        m_type != Type::Group) {
      parent = static_cast<LayerGroup*>(activeLayer);
      activeLayer = nullptr;
    }
    else {
      parent = activeLayer->parent();
    }
  }

  Layer* layer = nullptr;
  {
    Tx tx(
      writer.context(),
      fmt::format(Strings::commands_NewLayer(), layerPrefix()));
    DocApi api = document->getApi(tx);
    bool afterBackground = false;

    switch (m_type) {
      case Type::Layer:
        layer = api.newLayer(parent, name);
        if (m_place == Place::BeforeActiveLayer)
          api.restackLayerBefore(layer, parent, activeLayer);
        break;
      case Type::Group:
        layer = api.newGroup(parent, name);
        break;
      case Type::ReferenceLayer:
        layer = api.newLayer(parent, name);
        if (layer)
          layer->setReference(true);
        afterBackground = true;
        break;
    }

    ASSERT(layer);
    if (!layer)
      return;

    ASSERT(layer->parent());

    // Put new layer as an overlay of the background or in the first
    // layer in case the sprite is transparent.
    if (afterBackground) {
      Layer* first = sprite->root()->firstLayer();
      if (first) {
        if (first->isBackground())
          api.restackLayerAfter(layer, sprite->root(), first);
        else
          api.restackLayerBefore(layer, sprite->root(), first);
      }
    }
    // Move the layer above the active one.
    else if (activeLayer && m_place == Place::AfterActiveLayer) {
      api.restackLayerAfter(layer,
                            activeLayer->parent(),
                            activeLayer);
    }

    // Put all selected layers inside the group
    if (m_type == Type::Group && site.inTimeline()) {
      LayerGroup* commonParent = nullptr;
      layer_t sameParents = 0;
      for (Layer* l : selLayers) {
        if (!commonParent ||
            commonParent == l->parent()) {
          commonParent = l->parent();
          ++sameParents;
        }
      }

      if (sameParents == selLayers.size()) {
        for (Layer* newChild : selLayers.toLayerList()) {
          tx(
            new cmd::MoveLayer(newChild, layer,
                               static_cast<LayerGroup*>(layer)->lastLayer()));
        }
      }
    }

    // Paste sprite content
    if (pasteDoc && layer->isImage()) {
      Sprite* pasteSpr = pasteDoc->sprite();
      render::Render render;
      render.setNewBlend(true);
      render.setBgType(render::BgType::NONE);

      // Add more frames at the end
      if (writer.frame()+pasteSpr->lastFrame() > sprite->lastFrame())
        api.addEmptyFramesTo(sprite, writer.frame()+pasteSpr->lastFrame());

      // Paste the given sprite as flatten
      for (frame_t fr=0; fr<=pasteSpr->lastFrame(); ++fr) {
        ImageRef pasteImage(
          Image::create(
            pasteSpr->pixelFormat(),
            pasteSpr->width(),
            pasteSpr->height()));
        clear_image(pasteImage.get(),
                    pasteSpr->transparentColor());
        render.renderSprite(pasteImage.get(), pasteSpr, fr);

        frame_t dstFrame = writer.frame()+fr;

        if (sprite->pixelFormat() != pasteSpr->pixelFormat() ||
            sprite->pixelFormat() == IMAGE_INDEXED) {
          ImageRef pasteImageConv(
            render::convert_pixel_format(
              pasteImage.get(),
              nullptr,
              sprite->pixelFormat(),
              render::Dithering(),
              sprite->rgbMap(dstFrame),
              pasteSpr->palette(fr),
              (pasteSpr->backgroundLayer() ? true: false),
              sprite->transparentColor()));
          if (pasteImageConv)
            pasteImage = pasteImageConv;
        }

        Cel* cel = layer->cel(dstFrame);
        if (cel) {
          api.replaceImage(sprite, cel->imageRef(), pasteImage);
        }
        else {
          cel = api.addCel(static_cast<LayerImage*>(layer),
                           dstFrame, pasteImage);
        }

        if (cel) {
          if (layer->isReference()) {
            adjustRefCelBounds(
              cel, gfx::RectF(0, 0, pasteSpr->width(), pasteSpr->height()));
          }
          else {
            cel->setPosition(sprite->width()/2 - pasteSpr->width()/2,
                             sprite->height()/2 - pasteSpr->height()/2);
          }
        }
      }
    }
#ifdef ENABLE_UI
    // Paste new layer from clipboard
    else if (params().fromClipboard() && layer->isImage()) {
      clipboard::paste(context, false);

      if (layer->isReference()) {
        if (Cel* cel = layer->cel(site.frame())) {
          adjustRefCelBounds(
            cel, cel->boundsF());
        }
      }
    }
#endif // ENABLE_UI
    // Paste new layer from selection
    else if ((params().viaCut() || params().viaCopy())
             && document->isMaskVisible()) {
      const doc::Mask* mask = document->mask();
      ASSERT(mask);

      RestoreVisibleLayers restore;
      SelectedLayers layers;
      SelectedFrames frames;
      bool merged;
      if (site.range().enabled()) {
        merged = true;
        layers = site.range().selectedLayers();
        frames = site.range().selectedFrames();
        restore.showSelectedLayers(site.sprite(), layers);
      }
      else {
        merged = false;
        layers.insert(site.layer());
        frames.insert(site.frame());
      }

      for (frame_t frame : frames) {
        ImageRef newImage(new_image_from_mask(site, mask, true, merged));
        if (!newImage)
          continue;

        Cel* newCel = api.addCel(static_cast<LayerImage*>(layer),
                                 frame, newImage);
        if (newCel) {
          gfx::Point pos = mask->bounds().origin();
          newCel->setPosition(pos.x, pos.y);
        }

        for (Layer* layer : layers) {
          if (!layer->isImage() ||
              !layer->isEditable()) // Locked layers will not be modified
            continue;

          Cel* origCel = layer->cel(site.frame());
          if (origCel &&
              params().viaCut()) {
            tx(new cmd::ClearMask(origCel));

            if (layer->isTransparent()) {
              // If the cel wasn't deleted by cmd::ClearMask, we trim it.
              origCel = layer->cel(frame);
              if (origCel)
                tx(new cmd::TrimCel(origCel));
            }
          }
        }
      }
    }

    tx.commit();
  }

#ifdef ENABLE_UI
  if (context->isUIAvailable()) {
    update_screen_for_document(document);

    StatusBar::instance()->invalidate();
    StatusBar::instance()->showTip(
      1000, fmt::format("{} '{}' created",
                        layerPrefix(),
                        name));

    App::instance()->mainWindow()->popTimeline();
  }
#endif
}

std::string NewLayerCommand::onGetFriendlyName() const
{
  std::string text;
  if (m_place == Place::BeforeActiveLayer)
    text = fmt::format(Strings::commands_NewLayer_BeforeActiveLayer(), layerPrefix());
  else
    text = fmt::format(Strings::commands_NewLayer(), layerPrefix());
  if (params().fromClipboard())
    text = fmt::format(Strings::commands_NewLayer_FromClipboard(), text);
  if (params().viaCopy())
    text = fmt::format(Strings::commands_NewLayer_ViaCopy(), text);
  if (params().viaCut())
    text = fmt::format(Strings::commands_NewLayer_ViaCut(), text);
  return text;
}

void NewLayerCommand::adjustRefCelBounds(Cel* cel, gfx::RectF bounds)
{
  Sprite* sprite = cel->sprite();
  double scale = std::min(double(sprite->width()) / bounds.w,
                          double(sprite->height()) / bounds.h);
  bounds.w *= scale;
  bounds.h *= scale;
  bounds.x = sprite->width()/2 - bounds.w/2;
  bounds.y = sprite->height()/2 - bounds.h/2;
  cel->setBoundsF(bounds);
}

std::string NewLayerCommand::getUniqueLayerName(const Sprite* sprite) const
{
  return fmt::format("{} {}",
                     layerPrefix(),
                     getMaxLayerNum(sprite->root())+1);
}

int NewLayerCommand::getMaxLayerNum(const Layer* layer) const
{
  std::string prefix = layerPrefix();
  prefix += " ";

  int max = 0;
  if (std::strncmp(layer->name().c_str(), prefix.c_str(), prefix.size()) == 0)
    max = std::strtol(layer->name().c_str()+prefix.size(), NULL, 10);

  if (layer->isGroup()) {
    for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers()) {
      int tmp = getMaxLayerNum(child);
      max = std::max(tmp, max);
    }
  }

  return max;
}

std::string NewLayerCommand::layerPrefix() const
{
  switch (m_type) {
    case Type::Layer: return Strings::commands_NewLayer_Layer();
    case Type::Group: return Strings::commands_NewLayer_Group();
    case Type::ReferenceLayer: return Strings::commands_NewLayer_ReferenceLayer();
  }
  return "Unknown";
}

Command* CommandFactory::createNewLayerCommand()
{
  return new NewLayerCommand;
}

} // namespace app
