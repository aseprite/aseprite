// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include "base/fs.h"
#include "doc/cel.h"
#include "doc/color.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "os/display.h"
#include "os/surface.h"
#include "ui/manager.h"

namespace app {

using namespace ui;

struct ScreenshotParams : public NewParams {
  Param<bool> save { this, false, "save" };
  Param<bool> srgb { this, true, "srgb" };
};

class ScreenshotCommand : public CommandWithNewParams<ScreenshotParams> {
public:
  ScreenshotCommand();

protected:
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
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

  os::Display* display = ui::Manager::getDefault()->getDisplay();
  os::Surface* surface = display->getSurface();
  std::string fn;

  if (params().save()) {
    for (int i=0; i<10000; ++i) {
      fn = base::join_path(rf.defaultFilename(),
                           fmt::format("screenshot-{}.png", i));
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
                      0,    // Mask color
                      display->colorSpace()->gfxColorSpace());

  doc::Sprite* spr = doc::Sprite::MakeStdSprite(spec);
  static_cast<doc::LayerImage*>(spr->firstLayer())->configureAsBackground();

  doc::Cel* cel = spr->firstLayer()->cel(0);
  doc::Image* img = cel->image();

  for (int y=0; y<img->height(); ++y) {
    for (int x=0; x<img->width(); ++x) {
      gfx::Color c = surface->getPixel(x, y);

      img->putPixel(x, y, doc::rgba(gfx::getr(c),
                                    gfx::getg(c),
                                    gfx::getb(c), 255));
    }
  }

  std::unique_ptr<Doc> doc(new Doc(spr));
  doc->setFilename(fn);

  // Convert color profile to sRGB
  if (params().srgb())
    cmd::convert_color_profile(spr, gfx::ColorSpace::MakeSRGB());

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
