// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_spaces.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/console.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/pixel_ratio.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "render/quantization.h"
#include "ui/ui.h"

#include "new_sprite.xml.h"

using namespace ui;

namespace app {

struct NewFileParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<int> width{ this, 0, "width" };
  Param<int> height{ this, 0, "height" };
  Param<ColorMode> colorMode{ this, ColorMode::RGB, "colorMode" };
  Param<bool> fromClipboard{ this, false, "fromClipboard" };
};

class NewFileCommand : public CommandWithNewParams<NewFileParams> {
public:
  NewFileCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;

  static int g_spriteCounter;
};

// static
int NewFileCommand::g_spriteCounter = 0;

NewFileCommand::NewFileCommand() : CommandWithNewParams(CommandId::NewFile(), CmdRecordableFlag)
{
}

bool NewFileCommand::onEnabled(Context* ctx)
{
  return (!params().fromClipboard() || (ctx->clipboard()->format() == ClipboardFormat::Image));
}

void NewFileCommand::onExecute(Context* ctx)
{
  int width = params().width();
  int height = params().height();
  doc::ColorMode colorMode = params().colorMode();
  app::Color bgColor = app::Color::fromMask();
  doc::PixelRatio pixelRatio(1, 1);
  doc::ImageRef clipboardImage;
  doc::Palette clipboardPalette(0, 256);
  const int ncolors = get_default_palette()->size();

  if (params().fromClipboard()) {
    clipboardImage = ctx->clipboard()->getImage(&clipboardPalette);
    if (!clipboardImage)
      return;

    width = clipboardImage->width();
    height = clipboardImage->height();
    colorMode = clipboardImage->colorMode();
  }
  else if (ctx->isUIAvailable() && params().ui()) {
    Preferences& pref = Preferences::instance();
    app::Color bg_table[] = { app::Color::fromMask(),
                              app::Color::fromRgb(255, 255, 255),
                              app::Color::fromRgb(0, 0, 0) };

    // Load the window widget
    app::gen::NewSprite window;

    // Default values: Indexed, 320x240, Background color
    if (!params().colorMode.isSet()) {
      colorMode = pref.newFile.colorMode();
      // Invalid format in config file.
      if (colorMode != ColorMode::RGB && colorMode != ColorMode::INDEXED &&
          colorMode != ColorMode::GRAYSCALE) {
        colorMode = ColorMode::INDEXED;
      }
    }

    int w = pref.newFile.width();
    int h = pref.newFile.height();
    int bg = pref.newFile.backgroundColor();
    bg = std::clamp(bg, 0, 2);

    // If the clipboard contains an image, we can show the size of the
    // clipboard as default image size.
    gfx::Size clipboardSize;
    if (ctx->clipboard()->getImageSize(clipboardSize)) {
      w = clipboardSize.w;
      h = clipboardSize.h;
    }

    if (params().width.isSet())
      w = width;
    if (params().height.isSet())
      h = height;

    window.width()->setTextf("%d", std::max(1, w));
    window.height()->setTextf("%d", std::max(1, h));

    // Select image-type
    window.colorMode()->setSelectedItem(int(colorMode));

    // Select background color
    window.bgColor()->setSelectedItem(bg);

    // Advance options
    bool advanced = pref.newFile.advanced();
    window.advancedCheck()->setSelected(advanced);
    window.advancedCheck()->Click.connect([&] {
      window.advanced()->setVisible(window.advancedCheck()->isSelected());
      window.expandWindow(window.sizeHint());
    });
    window.advanced()->setVisible(advanced);
    if (advanced)
      window.pixelRatio()->setValue(pref.newFile.pixelRatio());
    else
      window.pixelRatio()->setValue("1:1");

    // Open the window
    window.openWindowInForeground();

    if (window.closer() != window.okButton())
      return;

    bool ok = false;

    // Get the options
    colorMode = (doc::ColorMode)window.colorMode()->selectedItem();
    w = window.width()->textInt();
    h = window.height()->textInt();
    bg = window.bgColor()->selectedItem();
    if (window.advancedCheck()->isSelected()) {
      pixelRatio = base::convert_to<PixelRatio>(window.pixelRatio()->getValue());
    }

    static_assert(int(ColorMode::RGB) == 0, "RGB pixel format should be 0");
    static_assert(int(ColorMode::INDEXED) == 2, "Indexed pixel format should be 2");

    colorMode = std::clamp(colorMode, ColorMode::RGB, ColorMode::INDEXED);
    w = std::clamp(w, 1, DOC_SPRITE_MAX_WIDTH);
    h = std::clamp(h, 1, DOC_SPRITE_MAX_HEIGHT);
    bg = std::clamp(bg, 0, 2);

    // Select the background color
    if (bg >= 0 && bg <= 3) {
      bgColor = bg_table[bg];
      ok = true;
    }

    if (ok) {
      // Save the configuration
      pref.newFile.width(w);
      pref.newFile.height(h);
      pref.newFile.colorMode(colorMode);
      pref.newFile.backgroundColor(bg);
      pref.newFile.advanced(window.advancedCheck()->isSelected());
      pref.newFile.pixelRatio(window.pixelRatio()->getValue());
    }

    width = w;
    height = h;
  }

  ASSERT(colorMode == ColorMode::RGB || colorMode == ColorMode::GRAYSCALE ||
         colorMode == ColorMode::INDEXED);
  if (width < 1 || height < 1)
    return;

  // Create the new sprite
  std::unique_ptr<Sprite> sprite(Sprite::MakeStdSprite(
    ImageSpec(colorMode, width, height, 0, get_working_rgb_space_from_preferences()),
    ncolors));

  sprite->setPixelRatio(pixelRatio);

  if (sprite->colorMode() != ColorMode::GRAYSCALE)
    get_default_palette()->copyColorsTo(sprite->palette(frame_t(0)));

  Layer* layer = sprite->root()->firstLayer();
  if (layer && layer->isImage()) {
    // If the background color isn't transparent, we have to
    // convert the `Layer 1' in a `Background'
    if (bgColor.getType() != app::Color::MaskType) {
      LayerImage* layerImage = static_cast<LayerImage*>(layer);
      layerImage->configureAsBackground();

      Image* image = layerImage->cel(frame_t(0))->image();

      // TODO Replace this adding a new parameter to color utils
      Palette oldPal = *get_current_palette();
      set_current_palette(get_default_palette(), false);

      doc::clear_image(image,
                       color_utils::color_for_target(bgColor,
                                                     ColorTarget(ColorTarget::BackgroundLayer,
                                                                 sprite->pixelFormat(),
                                                                 sprite->transparentColor())));

      set_current_palette(&oldPal, false);
    }
    else if (clipboardImage) {
      LayerImage* layerImage = static_cast<LayerImage*>(layer);
      // layerImage->configureAsBackground();

      Image* image = layerImage->cel(frame_t(0))->image();
      image->copy(clipboardImage.get(), gfx::Clip(clipboardImage->bounds()));

      if (clipboardPalette.isBlack()) {
        render::create_palette_from_sprite(sprite.get(),
                                           0,
                                           sprite->lastFrame(),
                                           true,
                                           &clipboardPalette,
                                           nullptr,
                                           true,
                                           Preferences::instance().quantization.rgbmapAlgorithm());
      }
      sprite->setPalette(&clipboardPalette, false);
    }

    if (layer->isBackground())
      layer->setName(Strings::commands_NewFile_BackgroundLayer());
    else
      layer->setName(fmt::format("{} {}", Strings::commands_NewLayer_Layer(), 1));
  }
  if (sprite->pixelFormat() == IMAGE_INDEXED) {
    sprite->rgbMap(0,
                   Sprite::RgbMapFor(!layer->isBackground()),
                   Preferences::instance().quantization.rgbmapAlgorithm(),
                   Preferences::instance().quantization.fitCriteria());
  }

  // Show the sprite to the user
  std::unique_ptr<Doc> doc(new Doc(sprite.get()));
  sprite.release();
  doc->setFilename(fmt::format("{}-{:04d}", Strings::commands_NewFile_Sprite(), ++g_spriteCounter));
  doc->setContext(ctx);
  doc.release();
}

std::string NewFileCommand::onGetFriendlyName() const
{
  if (params().fromClipboard())
    return Strings::commands_NewFile_FromClipboard();
  else
    return Strings::commands_NewFile();
}

Command* CommandFactory::createNewFileCommand()
{
  return new NewFileCommand;
}

} // namespace app
