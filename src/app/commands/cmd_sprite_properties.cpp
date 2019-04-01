// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/assign_color_profile.h"
#include "app/cmd/convert_color_profile.h"
#include "app/cmd/set_pixel_ratio.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/color_button.h"
#include "app/util/pixel_ratio.h"
#include "base/bind.h"
#include "base/mem_utils.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "os/color_space.h"
#include "os/system.h"
#include "ui/ui.h"

#include "sprite_properties.xml.h"

namespace app {

using namespace ui;

class SpritePropertiesCommand : public Command {
public:
  SpritePropertiesCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SpritePropertiesCommand::SpritePropertiesCommand()
  : Command(CommandId::SpriteProperties(), CmdUIOnlyFlag)
{
}

bool SpritePropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void SpritePropertiesCommand::onExecute(Context* context)
{
  std::string imgtype_text;
  ColorButton* color_button = nullptr;

  // List of available color profiles
  std::vector<os::ColorSpacePtr> colorSpaces;
  os::instance()->listColorSpaces(colorSpaces);

  // Load the window widget
  app::gen::SpriteProperties window;
  int selectedColorProfile = -1;

  auto updateButtons =
    [&] {
      bool enabled = (selectedColorProfile != window.colorProfile()->getSelectedItemIndex());
      window.assignColorProfile()->setEnabled(enabled);
      window.convertColorProfile()->setEnabled(enabled);
      window.ok()->setEnabled(!enabled);
    };

  // Get sprite properties and fill frame fields
  {
    const ContextReader reader(context);
    const Doc* document(reader.document());
    const Sprite* sprite(reader.sprite());

    // Update widgets values
    switch (sprite->pixelFormat()) {
      case IMAGE_RGB:
        imgtype_text = "RGB";
        break;
      case IMAGE_GRAYSCALE:
        imgtype_text = "Grayscale";
        break;
      case IMAGE_INDEXED:
        imgtype_text = fmt::format("Indexed ({0} colors)",
                                   sprite->palette(0)->size());
        break;
      default:
        ASSERT(false);
        imgtype_text = "Unknown";
        break;
    }

    // Filename
    window.name()->setText(document->filename());

    // Color mode
    window.type()->setText(imgtype_text.c_str());

    // Sprite size (width and height)
    window.size()->setTextf(
      "%dx%d (%s)",
      sprite->width(),
      sprite->height(),
      base::get_pretty_memory_size(sprite->getMemSize()).c_str());

    // How many frames
    window.frames()->setTextf("%d", (int)sprite->totalFrames());

    if (sprite->pixelFormat() == IMAGE_INDEXED) {
      color_button = new ColorButton(app::Color::fromIndex(sprite->transparentColor()),
                                     IMAGE_INDEXED,
                                     ColorButtonOptions());

      window.transparentColorPlaceholder()->addChild(color_button);

      // TODO add a way to get or create an existent TooltipManager
      TooltipManager* tooltipManager = new TooltipManager;
      window.addChild(tooltipManager);
      tooltipManager->addTooltipFor(
        color_button,
        Strings::sprite_properties_transparent_color_tooltip(),
        LEFT);
    }
    else {
      window.transparentColorPlaceholder()->addChild(new Label("(only for indexed images)"));
    }

    // Pixel ratio
    window.pixelRatio()->setValue(
      base::convert_to<std::string>(sprite->pixelRatio()));

    // Color profile
    selectedColorProfile = -1;
    int i = 0;
    for (auto& cs : colorSpaces) {
      if (cs->gfxColorSpace()->nearlyEqual(*sprite->colorSpace())) {
        selectedColorProfile = i;
        break;
      }
      ++i;
    }
    if (selectedColorProfile < 0) {
      colorSpaces.push_back(os::instance()->createColorSpace(sprite->colorSpace()));
      selectedColorProfile = colorSpaces.size()-1;
    }

    for (auto& cs : colorSpaces)
      window.colorProfile()->addItem(cs->gfxColorSpace()->name());
    window.colorProfile()->setSelectedItemIndex(selectedColorProfile);

    window.assignColorProfile()->setEnabled(false);
    window.convertColorProfile()->setEnabled(false);
    window.colorProfile()->Change.connect(updateButtons);

    window.assignColorProfile()->Click.connect(
      [&](Event&){
        selectedColorProfile = window.colorProfile()->getSelectedItemIndex();

        ContextWriter writer(context);
        Sprite* sprite(writer.sprite());
        Tx tx(writer.context(), "Assign Color Profile");
        tx(new cmd::AssignColorProfile(
             sprite, colorSpaces[selectedColorProfile]->gfxColorSpace()));
        tx.commit();

        updateButtons();
      });
    window.convertColorProfile()->Click.connect(
      [&](Event&){
        selectedColorProfile = window.colorProfile()->getSelectedItemIndex();

        ContextWriter writer(context);
        Sprite* sprite(writer.sprite());
        Tx tx(writer.context(), "Convert Color Profile");
        tx(new cmd::ConvertColorProfile(
             sprite, colorSpaces[selectedColorProfile]->gfxColorSpace()));
        tx.commit();

        updateButtons();
      });
  }

  window.remapWindow();
  window.centerWindow();

  load_window_pos(&window, "SpriteProperties");
  window.setVisible(true);
  window.openWindowInForeground();

  if (window.closer() == window.ok()) {
    ContextWriter writer(context);
    Sprite* sprite(writer.sprite());

    color_t index = (color_button ? color_button->getColor().getIndex():
                                    sprite->transparentColor());
    PixelRatio pixelRatio =
      base::convert_to<PixelRatio>(window.pixelRatio()->getValue());

    if (index != sprite->transparentColor() ||
        pixelRatio != sprite->pixelRatio()) {
      Tx tx(writer.context(), "Change Sprite Properties");
      DocApi api = writer.document()->getApi(tx);

      if (index != sprite->transparentColor())
        api.setSpriteTransparentColor(sprite, index);

      if (pixelRatio != sprite->pixelRatio())
        tx(new cmd::SetPixelRatio(sprite, pixelRatio));

      tx.commit();

      update_screen_for_document(writer.document());
    }
  }

  save_window_pos(&window, "SpriteProperties");
}

Command* CommandFactory::createSpritePropertiesCommand()
{
  return new SpritePropertiesCommand;
}

} // namespace app
