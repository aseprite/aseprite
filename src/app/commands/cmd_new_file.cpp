// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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
#include "app/console.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/modules/editors.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/ui/button_set.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/pixel_ratio.h"
#include "base/bind.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "new_sprite.xml.h"

using namespace ui;

namespace app {

class NewFileCommand : public Command {
public:
  NewFileCommand();

protected:
  void onExecute(Context* context) override;
};

static int _sprite_counter = 0;

NewFileCommand::NewFileCommand()
  : Command(CommandId::NewFile(), CmdRecordableFlag)
{
}

/**
 * Shows the "New Sprite" dialog.
 */
void NewFileCommand::onExecute(Context* context)
{
  Preferences& pref = Preferences::instance();
  int ncolors = get_default_palette()->size();
  char buf[1024];
  app::Color bg_table[] = {
    app::Color::fromMask(),
    app::Color::fromRgb(255, 255, 255),
    app::Color::fromRgb(0, 0, 0),
  };

  // Load the window widget
  app::gen::NewSprite window;

  // Default values: Indexed, 320x240, Background color
  doc::ColorMode colorMode = pref.newFile.colorMode();
  // Invalid format in config file.
  if (colorMode != ColorMode::RGB &&
      colorMode != ColorMode::INDEXED &&
      colorMode != ColorMode::GRAYSCALE) {
    colorMode = ColorMode::INDEXED;
  }
  int w = pref.newFile.width();
  int h = pref.newFile.height();
  int bg = pref.newFile.backgroundColor();
  bg = MID(0, bg, 2);

  // If the clipboard contains an image, we can show the size of the
  // clipboard as default image size.
  gfx::Size clipboardSize;
  if (clipboard::get_image_size(clipboardSize)) {
    w = clipboardSize.w;
    h = clipboardSize.h;
  }

  window.width()->setTextf("%d", MAX(1, w));
  window.height()->setTextf("%d", MAX(1, h));

  // Select image-type
  window.colorMode()->setSelectedItem(int(colorMode));

  // Select background color
  window.bgColor()->setSelectedItem(bg);

  // Advance options
  bool advanced = pref.newFile.advanced();
  window.advancedCheck()->setSelected(advanced);
  window.advancedCheck()->Click.connect(
    base::Bind<void>(
      [&]{
        gfx::Rect bounds = window.bounds();
        window.advanced()->setVisible(window.advancedCheck()->isSelected());
        window.setBounds(gfx::Rect(window.bounds().origin(),
                                   window.sizeHint()));
        window.layout();

        window.manager()->invalidateRect(bounds);
      }));
  window.advanced()->setVisible(advanced);
  if (advanced)
    window.pixelRatio()->setValue(pref.newFile.pixelRatio());
  else
    window.pixelRatio()->setValue("1:1");

  // Open the window
  window.openWindowInForeground();

  if (window.closer() == window.okButton()) {
    bool ok = false;

    // Get the options
    colorMode = (doc::ColorMode)window.colorMode()->selectedItem();
    w = window.width()->textInt();
    h = window.height()->textInt();
    bg = window.bgColor()->selectedItem();

    static_assert(int(ColorMode::RGB) == 0, "RGB pixel format should be 0");
    static_assert(int(ColorMode::INDEXED) == 2, "Indexed pixel format should be 2");

    colorMode = MID(ColorMode::RGB, colorMode, ColorMode::INDEXED);
    w = MID(1, w, DOC_SPRITE_MAX_WIDTH);
    h = MID(1, h, DOC_SPRITE_MAX_HEIGHT);
    bg = MID(0, bg, 2);

    // Select the color
    app::Color color = app::Color::fromMask();

    if (bg >= 0 && bg <= 3) {
      color = bg_table[bg];
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

      // Create the new sprite
      ASSERT(colorMode == ColorMode::RGB ||
             colorMode == ColorMode::GRAYSCALE ||
             colorMode == ColorMode::INDEXED);
      ASSERT(w > 0 && h > 0);

      std::unique_ptr<Sprite> sprite(
        Sprite::createBasicSprite(
          ImageSpec(colorMode, w, h, 0,
                    get_working_rgb_space_from_preferences()), ncolors));

      if (window.advancedCheck()->isSelected()) {
        sprite->setPixelRatio(
          base::convert_to<PixelRatio>(window.pixelRatio()->getValue()));
      }

      if (sprite->colorMode() != ColorMode::GRAYSCALE)
        get_default_palette()->copyColorsTo(sprite->palette(frame_t(0)));

      // If the background color isn't transparent, we have to
      // convert the `Layer 1' in a `Background'
      if (color.getType() != app::Color::MaskType) {
        Layer* layer = sprite->root()->firstLayer();

        if (layer && layer->isImage()) {
          LayerImage* layerImage = static_cast<LayerImage*>(layer);
          layerImage->configureAsBackground();

          Image* image = layerImage->cel(frame_t(0))->image();

          // TODO Replace this adding a new parameter to color utils
          Palette oldPal = *get_current_palette();
          set_current_palette(get_default_palette(), false);

          doc::clear_image(image,
            color_utils::color_for_target(color,
              ColorTarget(
                ColorTarget::BackgroundLayer,
                sprite->pixelFormat(),
                sprite->transparentColor())));

          set_current_palette(&oldPal, false);
        }
      }

      // Show the sprite to the user
      std::unique_ptr<Doc> doc(new Doc(sprite.get()));
      sprite.release();
      sprintf(buf, "Sprite-%04d", ++_sprite_counter);
      doc->setFilename(buf);
      doc->setContext(context);
      doc.release();
    }
  }
}

Command* CommandFactory::createNewFileCommand()
{
  return new NewFileCommand;
}

} // namespace app
