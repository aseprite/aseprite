// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/move_layer.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/find_widget.h"
#include "app/i18n/strings.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/ui.h"

#include "new_layer.xml.h"

#include <cstdio>
#include <cstring>

namespace app {

using namespace ui;

class NewLayerCommand : public Command {
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
  std::string getUniqueLayerName(const Sprite* sprite) const;
  int getMaxLayerNum(const Layer* layer) const;
  const char* layerPrefix() const;

  std::string m_name;
  Type m_type;
  Place m_place;
  bool m_ask;
  bool m_fromFile;
};

NewLayerCommand::NewLayerCommand()
  : Command(CommandId::NewLayer(), CmdRecordableFlag)
{
  m_name = "";
  m_type = Type::Layer;
  m_place = Place::AfterActiveLayer;
  m_ask = false;
}

void NewLayerCommand::onLoadParams(const Params& params)
{
  m_name = params.get("name");
  m_type = Type::Layer;
  if (params.get("group") == "true")
    m_type = Type::Group;
  else if (params.get("reference") == "true")
    m_type = Type::ReferenceLayer;

  m_ask = (params.get("ask") == "true");
  m_fromFile = (params.get("from-file") == "true");
  m_place = Place::AfterActiveLayer;
  if (params.get("top") == "true")
    m_place = Place::Top;
  else if (params.get("before") == "true")
    m_place = Place::BeforeActiveLayer;
}

bool NewLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
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
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  std::string name;

  Doc* pasteDoc = nullptr;
  Scoped destroyPasteDoc(
    [&pasteDoc, context]{
      if (pasteDoc) {
        context->documents().remove(pasteDoc);
        delete pasteDoc;
      }
    });

  // Default name (m_name is a name specified in params)
  if (!m_name.empty())
    name = m_name;
  else
    name = getUniqueLayerName(sprite);

  // Select a file to copy its content
  if (m_fromFile) {
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
  if (m_ask) {
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
  SelectedLayers selLayers = writer.site()->selectedLayers();
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
      std::string("New ") + layerPrefix());
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
    if (m_type == Type::Group && writer.site()->inTimeline()) {
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
              render::DitheringAlgorithm::None,
              render::DitheringMatrix(),
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
            gfx::RectF bounds(0, 0, pasteSpr->width(), pasteSpr->height());
            double scale = MIN(double(sprite->width()) / bounds.w,
                               double(sprite->height()) / bounds.h);
            bounds.w *= scale;
            bounds.h *= scale;
            bounds.x = sprite->width()/2 - bounds.w/2;
            bounds.y = sprite->height()/2 - bounds.h/2;
            cel->setBoundsF(bounds);
          }
          else {
            cel->setPosition(sprite->width()/2 - pasteSpr->width()/2,
                             sprite->height()/2 - pasteSpr->height()/2);
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
      1000, "%s '%s' created",
      layerPrefix(),
      name.c_str());

    App::instance()->mainWindow()->popTimeline();
  }
#endif
}

std::string NewLayerCommand::onGetFriendlyName() const
{
  std::string text;

  switch (m_type) {
    case Type::Layer:
      if (m_place == Place::BeforeActiveLayer)
        text = Strings::commands_NewLayer_BeforeActiveLayer();
      else
        text = Strings::commands_NewLayer();
      break;
    case Type::Group:
      text = Strings::commands_NewLayer_Group();
      break;
    case Type::ReferenceLayer:
      text = Strings::commands_NewLayer_ReferenceLayer();
      break;
  }

  return text;
}

std::string NewLayerCommand::getUniqueLayerName(const Sprite* sprite) const
{
  char buf[1024];
  std::sprintf(buf, "%s %d",
               layerPrefix(),
               getMaxLayerNum(sprite->root())+1);
  return buf;
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
      max = MAX(tmp, max);
    }
  }

  return max;
}

const char* NewLayerCommand::layerPrefix() const
{
  switch (m_type) {
    case Type::Layer: return "Layer";
    case Type::Group: return "Group";
    case Type::ReferenceLayer: return "Reference Layer";
  }
  return "Unknown";
}

Command* CommandFactory::createNewLayerCommand()
{
  return new NewLayerCommand;
}

} // namespace app
