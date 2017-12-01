// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "ui/ui.h"

#include <stdexcept>

namespace app {

class NewFrameCommand : public Command {
public:
  enum class Content {
    DUPLICATE_FRAME,
    NEW_EMPTY_FRAME,
    DUPLICATE_CELS,
    DUPLICATE_CELS_BLOCK,
   };

  NewFrameCommand();
  Command* clone() const override { return new NewFrameCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Content m_content;
};

NewFrameCommand::NewFrameCommand()
  : Command(CommandId::NewFrame(), CmdRecordableFlag)
{
}

void NewFrameCommand::onLoadParams(const Params& params)
{
  m_content = Content::DUPLICATE_FRAME;

  std::string content = params.get("content");
  if (content == "current" ||
      content == "frame")
    m_content = Content::DUPLICATE_FRAME;
  else if (content == "empty")
    m_content = Content::NEW_EMPTY_FRAME;
  else if (content == "cel")
    m_content = Content::DUPLICATE_CELS;
  else if (content == "celblock")
    m_content = Content::DUPLICATE_CELS_BLOCK;
}

bool NewFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewFrameCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Transaction transaction(writer.context(), friendlyName());
    DocumentApi api = document->getApi(transaction);

    switch (m_content) {

      case Content::DUPLICATE_FRAME:
        api.addFrame(sprite, writer.frame()+1);
        break;

      case Content::NEW_EMPTY_FRAME:
        api.addEmptyFrame(sprite, writer.frame()+1);
        break;

      case Content::DUPLICATE_CELS:
      case Content::DUPLICATE_CELS_BLOCK: {
        const Site* site = writer.site();
        if (site->inTimeline() &&
            !site->selectedLayers().empty() &&
            !site->selectedFrames().empty()) {
          std::map<CelData*, Cel*> relatedCels;

          auto timeline = App::instance()->timeline();
          timeline->prepareToMoveRange();
          DocumentRange range = timeline->range();

          SelectedLayers selLayers;
          if (site->inFrames())
            selLayers.selectAllLayers(writer.sprite()->root());
          else
            selLayers = site->selectedLayers();

          frame_t frameRange =
            (site->selectedFrames().lastFrame() -
             site->selectedFrames().firstFrame() + 1);

          for (Layer* layer : selLayers) {
            if (layer->isImage()) {
              for (frame_t srcFrame : site->selectedFrames().reversed()) {
                frame_t dstFrame = srcFrame+frameRange;
                bool continuous;
                CelData* srcCelData = nullptr;

                if (m_content == Content::DUPLICATE_CELS_BLOCK) {
                  continuous = false;

                  Cel* srcCel = static_cast<LayerImage*>(layer)->cel(srcFrame);
                  if (srcCel) {
                    srcCelData = srcCel->data();

                    auto it = relatedCels.find(srcCelData);
                    if (it != relatedCels.end()) {
                      srcFrame = it->second->frame();
                      continuous = true;
                    }
                  }
                }
                else
                  continuous = layer->isContinuous();

                api.copyCel(
                  static_cast<LayerImage*>(layer), srcFrame,
                  static_cast<LayerImage*>(layer), dstFrame, continuous);

                if (srcCelData && !relatedCels[srcCelData])
                  relatedCels[srcCelData] = layer->cel(dstFrame);
              }
            }
          }

          range.displace(0, frameRange);
          timeline->moveRange(range);
        }
        else {
          api.copyCel(
            static_cast<LayerImage*>(writer.layer()), writer.frame(),
            static_cast<LayerImage*>(writer.layer()), writer.frame()+1);

          // TODO should we use DocumentObserver?
          if (UIContext::instance() == context) {
            if (DocumentView* view = UIContext::instance()->activeView())
              view->editor()->setFrame(writer.frame()+1);
          }
        }
        break;
      }
    }

    transaction.commit();
  }
  update_screen_for_document(document);

  StatusBar::instance()
    ->showTip(1000, "New frame %d/%d",
              (int)context->activeSite().frame()+1,
              (int)sprite->totalFrames());

  App::instance()->mainWindow()->popTimeline();
}

std::string NewFrameCommand::onGetFriendlyName() const
{
  std::string text;

  switch (m_content) {
    case Content::DUPLICATE_FRAME:
      text = Strings::commands_NewFrame();
      break;
    case Content::NEW_EMPTY_FRAME:
      text = Strings::commands_NewFrame_NewEmptyFrame();
      break;
    case Content::DUPLICATE_CELS:
      text = Strings::commands_NewFrame_DuplicateCels();
      break;
    case Content::DUPLICATE_CELS_BLOCK:
      text = Strings::commands_NewFrame_DuplicateCelsBlock();
      break;
  }

  return text;
}

Command* CommandFactory::createNewFrameCommand()
{
  return new NewFrameCommand;
}

} // namespace app
