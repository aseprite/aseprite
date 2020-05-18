// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/doc_view.h"
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
    DUPLICATE_CELS_COPIES,
    DUPLICATE_CELS_LINKED,
   };

  NewFrameCommand();

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
  else if (content == "celblock" ||
           content == "celcopies")
    m_content = Content::DUPLICATE_CELS_COPIES;
  else if (content == "cellinked")
    m_content = Content::DUPLICATE_CELS_LINKED;
}

bool NewFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewFrameCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Tx tx(writer.context(), friendlyName());
    DocApi api = document->getApi(tx);

    switch (m_content) {

      case Content::DUPLICATE_FRAME:
        api.addFrame(sprite, writer.frame()+1);
        break;

      case Content::NEW_EMPTY_FRAME:
        api.addEmptyFrame(sprite, writer.frame()+1);
        break;

      case Content::DUPLICATE_CELS:
      case Content::DUPLICATE_CELS_LINKED:
      case Content::DUPLICATE_CELS_COPIES: {
        std::unique_ptr<bool> continuous = nullptr;
        switch (m_content) {
          case Content::DUPLICATE_CELS_COPIES: continuous.reset(new bool(false)); break;
          case Content::DUPLICATE_CELS_LINKED: continuous.reset(new bool(true)); break;
        }

        const Site* site = writer.site();
        if (site->inTimeline() &&
            !site->selectedLayers().empty() &&
            !site->selectedFrames().empty()) {
#if ENABLE_UI
          auto timeline = App::instance()->timeline();
          timeline->prepareToMoveRange();
          DocRange range = timeline->range();
#endif

          SelectedLayers selLayers;
          if (site->inFrames())
            selLayers.selectAllLayers(writer.sprite()->root());
          else {
            selLayers = site->selectedLayers();
            selLayers.expandCollapsedGroups();
          }

          frame_t frameRange =
            (site->selectedFrames().lastFrame() -
             site->selectedFrames().firstFrame() + 1);

          for (Layer* layer : selLayers) {
            if (layer->isImage()) {
              for (frame_t srcFrame : site->selectedFrames().reversed()) {
                frame_t dstFrame = srcFrame+frameRange;
                api.copyCel(
                  static_cast<LayerImage*>(layer), srcFrame,
                  static_cast<LayerImage*>(layer), dstFrame, continuous.get());
              }
            }
          }

#ifdef ENABLE_UI                // TODO the range should be part of the Site
          range.displace(0, frameRange);
          timeline->moveRange(range);
#endif
        }
        else if (auto layer = static_cast<LayerImage*>(writer.layer())) {
          api.copyCel(
            layer, writer.frame(),
            layer, writer.frame()+1, continuous.get());

          context->setActiveFrame(writer.frame()+1);
        }
        break;
      }
    }

    tx.commit();
  }

#ifdef ENABLE_UI
  if (context->isUIAvailable()) {
    update_screen_for_document(document);

    StatusBar::instance()->showTip(
      1000, fmt::format("New frame {}/{}",
                        (int)context->activeSite().frame()+1,
                        (int)sprite->totalFrames()));

    App::instance()->mainWindow()->popTimeline();
  }
#endif
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
    case Content::DUPLICATE_CELS_COPIES:
      text = Strings::commands_NewFrame_DuplicateCelsCopies();
      break;
    case Content::DUPLICATE_CELS_LINKED:
      text = Strings::commands_NewFrame_DuplicateCelsLinked();
      break;
  }

  return text;
}

Command* CommandFactory::createNewFrameCommand()
{
  return new NewFrameCommand;
}

} // namespace app
