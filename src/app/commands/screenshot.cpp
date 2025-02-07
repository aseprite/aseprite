// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/convert_color_profile.h"
#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/i18n/strings.h"
#include "app/resource_finder.h"
#include "base/buffer.h"
#include "base/fs.h"
#include "doc/cel.h"
#include "doc/color.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "os/surface.h"
#include "os/window.h"
#include "ui/alert.h"
#include "ui/manager.h"
#include "ui/scale.h"

#ifdef ENABLE_STEAM
  #include "steam/steam.h"
#endif

namespace app {

using namespace ui;

struct ScreenshotParams : public NewParams {
  Param<bool> save{ this, false, "save" };
  Param<bool> srgb{ this, true, "srgb" };
#ifdef ENABLE_STEAM
  Param<bool> steam{ this, false, "steam" };
#endif
};

class ScreenshotCommand : public CommandWithNewParams<ScreenshotParams> {
public:
  ScreenshotCommand();

protected:
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
  bool isListed(const Params& params) const override { return !params.empty(); }
};

ScreenshotCommand::ScreenshotCommand()
  : CommandWithNewParams<ScreenshotParams>(CommandId::Screenshot(), CmdUIOnlyFlag)
{
}

void ScreenshotCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  app::ResourceFinder rf(false);
  rf.includeDesktopDir("");

  os::Window* window = ui::Manager::getDefault()->display()->nativeWindow();
  os::Surface* surface = window->surface();
  std::string fn;

  if (params().save()) {
    for (int i = 0; i < 10000; ++i) {
      fn = base::join_path(rf.defaultFilename(), fmt::format("screenshot-{}.png", i));
      if (!base::is_file(fn))
        break;
    }
  }
  else {
    fn = "screenshot.png";
  }

  doc::ImageSpec spec(doc::ColorMode::RGB,
                      surface->width(),
                      surface->height(),
                      0, // Mask color
                      window->colorSpace()->gfxColorSpace());

  doc::Sprite* spr = doc::Sprite::MakeStdSprite(spec);
  static_cast<doc::LayerImage*>(spr->firstLayer())->configureAsBackground();

  doc::Cel* cel = spr->firstLayer()->cel(0);
  doc::Image* img = cel->image();
  const int w = img->width();
  const int h = img->height();

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      gfx::Color c = surface->getPixel(x, y);

      img->putPixel(x, y, doc::rgba(gfx::getr(c), gfx::getg(c), gfx::getb(c), 255));
    }
  }

  std::unique_ptr<Doc> doc(new Doc(spr));
  doc->setFilename(fn);

  // Convert color profile to sRGB
  if (params().srgb())
    cmd::convert_color_profile(spr, gfx::ColorSpace::MakeSRGB());

#ifdef ENABLE_STEAM
  if (params().steam()) {
    if (auto steamAPI = steam::SteamAPI::instance()) {
      // Get image again (cmd::convert_color_profile() might have changed it)
      img = cel->image();

      const int scale = window->scale();
      base::buffer rgbBuffer(3 * w * h * scale * scale);
      int c = 0;
      doc::LockImageBits<RgbTraits> bits(img);
      for (int y = 0; y < h; ++y) {
        for (int i = 0; i < scale; ++i) {
          for (int x = 0; x < w; ++x) {
            color_t color = get_pixel_fast<RgbTraits>(img, x, y);
            for (int j = 0; j < scale; ++j) {
              rgbBuffer[c++] = doc::rgba_getr(color);
              rgbBuffer[c++] = doc::rgba_getg(color);
              rgbBuffer[c++] = doc::rgba_getb(color);
            }
          }
        }
      }
      if (steamAPI->writeScreenshot(&rgbBuffer[0], rgbBuffer.size(), w * scale, h * scale))
        return;
    }
  }
#endif

  if (params().save()) {
    save_document(ctx, doc.get());
  }
  else {
    doc->setContext(ctx);
    doc.release();
  }
}

std::string ScreenshotCommand::onGetFriendlyName() const
{
  std::string name;
#ifdef ENABLE_STEAM
  if (params().steam())
    name = Strings::commands_Screenshot_Steam();
  else
#endif
    if (params().save())
    name = Strings::commands_Screenshot_Save();
  else
    name = Strings::commands_Screenshot_Open();
  if (params().srgb()) {
    name.push_back(' ');
    name += Strings::commands_Screenshot_sRGB();
  }
  else {
    name.push_back(' ');
    name += Strings::commands_Screenshot_DisplayCS();
  }
  return name;
}

Command* CommandFactory::createScreenshotCommand()
{
  return new ScreenshotCommand;
}

} // namespace app
