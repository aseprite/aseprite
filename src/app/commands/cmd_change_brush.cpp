// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/tilemap_mode.h"
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/ui/context_bar.h"
#include "app/ui/main_window.h"
#include "base/convert_to.h"
#include "doc/algorithm/flip_image.h"
#include "doc/brush.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "doc/tile.h"
#include "fmt/format.h"

#include <algorithm>
#include <string>

namespace app {

using namespace doc;
using namespace doc::algorithm;

class ChangeBrushCommand : public Command {
  enum Change {
    None,
    IncrementSize,
    DecrementSize,
    IncrementAngle,
    DecrementAngle,
    FlipX,
    FlipY,
    FlipD,
    Rotate90CW,
    CustomBrush,
  };

public:
  ChangeBrushCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Change m_change;
  int m_slot;
};

ChangeBrushCommand::ChangeBrushCommand()
  : Command(CommandId::ChangeBrush(), CmdUIOnlyFlag)
{
  m_change = None;
  m_slot = 0;
}

void ChangeBrushCommand::onLoadParams(const Params& params)
{
  std::string change = params.get("change");
  if (change == "increment-size") m_change = IncrementSize;
  else if (change == "decrement-size") m_change = DecrementSize;
  else if (change == "increment-angle") m_change = IncrementAngle;
  else if (change == "decrement-angle") m_change = DecrementAngle;
  else if (change == "flip-x") m_change = FlipX;
  else if (change == "flip-y") m_change = FlipY;
  else if (change == "flip-d") m_change = FlipD;
  else if (change == "rotate-90cw") m_change = Rotate90CW;
  else if (change == "custom") m_change = CustomBrush;

  if (m_change == CustomBrush)
    m_slot = params.get_as<int>("slot");
  else
    m_slot = 0;
}

void ChangeBrushCommand::onExecute(Context* context)
{
  auto app = App::instance();
  auto atm = app->activeToolManager();
  auto& pref = Preferences::instance();
  auto colorBar = ColorBar::instance();
  auto contextBar = (app->mainWindow() ? app->mainWindow()->getContextBar():
                                         nullptr);
  const BrushRef brush = (contextBar ? contextBar->activeBrush():
                                       nullptr);
  const bool isImageBrush = (brush && brush->type() == kImageBrushType);

  // Change the brush of the active tool (i.e. quick tool) only if the
  // active tool is of paint/eraser (e.g. proximity tool for eraser),
  // in other case (e.g. if the active tool is the Hand tool because
  // the user pressed the Space bar modifier), we'll change the brush
  // of the selected tool in the toolbar (ignoring the quick tool).
  tools::Tool* tool = atm->activeTool();
  if (!tool->getInk(0)->isPaint() &&
      !tool->getInk(0)->isEffect() &&
      !tool->getInk(0)->isEraser() &&
      !tool->getInk(0)->isShading()) {
    tool = atm->selectedTool();
  }

  ToolPreferences::Brush& brushPref = pref.tool(tool).brush;

  switch (m_change) {

    case None:
      // Do nothing
      break;

    case IncrementSize:
    case DecrementSize:
      // Resize x2 or x0.5 when resizing image brushes
      if (isImageBrush) {
        double scale = 1.0;
        switch (m_change) {
          case IncrementSize: scale = 2.0; break;
          case DecrementSize: scale = 0.5; break;
        }

        gfx::Size size = brush->bounds().size();
        size = gfx::Size(std::max<int>(1, size.w * scale),
                         std::max<int>(1, size.h * scale));

        ImageRef newImg(Image::create(brush->image()->pixelFormat(), size.w, size.h));
        ImageRef newMsk(Image::create(IMAGE_BITMAP, size.w, size.h));
        const color_t bg = brush->image()->maskColor();
        newImg->setMaskColor(bg);

        newImg->clear(bg);
        newMsk->clear(0);

        resize_image(brush->image(), newImg.get(),
                     RESIZE_METHOD_NEAREST_NEIGHBOR,
                     nullptr, nullptr, bg);

        resize_image(brush->maskBitmap(), newMsk.get(),
                     RESIZE_METHOD_NEAREST_NEIGHBOR,
                     nullptr, nullptr, 0);

        // Create a copy of the brush (to avoid modifying the original
        // brush from the AppBrushes stock)
        BrushRef newBrush = std::make_shared<Brush>(*brush);
        newBrush->setImage(newImg.get(),
                           newMsk.get());
        contextBar->setActiveBrush(newBrush);
      }
      else {
        switch (m_change) {
          case IncrementSize:
            if (brushPref.size() < Brush::kMaxBrushSize)
              brushPref.size(brushPref.size()+1);
            break;
          case DecrementSize:
            if (brushPref.size() > Brush::kMinBrushSize)
              brushPref.size(brushPref.size()-1);
            break;
        }
      }
      break;

    case IncrementAngle:
      if (brushPref.angle() < 180)
        brushPref.angle(brushPref.angle()+1);
      break;

    case DecrementAngle:
      if (brushPref.angle() > 0)
        brushPref.angle(brushPref.angle()-1);
      break;

    case FlipX:
    case FlipY:
      if (colorBar && colorBar->tilemapMode() == TilemapMode::Tiles) {
        doc::tile_t t = pref.colorBar.fgTile();
        switch (m_change) {
          case FlipX: pref.colorBar.fgTile(t ^ tile_f_xflip); break;
          case FlipY: pref.colorBar.fgTile(t ^ tile_f_yflip); break;
        }
      }
      else if (isImageBrush) {
        ImageRef newImg(Image::createCopy(brush->image()));
        ImageRef newMsk(Image::createCopy(brush->maskBitmap()));
        const gfx::Rect bounds = newImg->bounds();

        switch (m_change) {
          case FlipX:
            flip_image(newImg.get(), bounds, FlipType::FlipHorizontal);
            flip_image(newMsk.get(), bounds, FlipType::FlipHorizontal);
            break;
          case FlipY:
            flip_image(newImg.get(), bounds, FlipType::FlipVertical);
            flip_image(newMsk.get(), bounds, FlipType::FlipVertical);
            break;
        }

        BrushRef newBrush = std::make_shared<Brush>(*brush);
        newBrush->setImage(newImg.get(),
                           newMsk.get());
        contextBar->setActiveBrush(newBrush);
      }
      else {
        switch (m_change) {
          case FlipX:
            brushPref.angle(brushPref.angle() < 0 ? 180 + brushPref.angle():
                                                    180 - brushPref.angle());
            break;
          case FlipY:
            brushPref.angle(-brushPref.angle());
            break;
        }
      }
      break;

    case FlipD:
    case Rotate90CW:
      if (colorBar && colorBar->tilemapMode() == TilemapMode::Tiles) {
        doc::tile_t t = pref.colorBar.fgTile();
        switch (m_change) {
          case FlipD:
            pref.colorBar.fgTile(t ^ tile_f_dflip);
            break;
          case Rotate90CW: {
            doc::tile_flags ti = doc::tile_geti(t);
            doc::tile_flags tf = doc::tile_getf(t);
            tf =
              (tf & doc::tile_f_xflip ? doc::tile_f_yflip: 0) |
              (tf & doc::tile_f_yflip ? 0: doc::tile_f_xflip) |
              (tf & doc::tile_f_dflip ? 0: doc::tile_f_dflip);
            pref.colorBar.fgTile(doc::tile(ti, tf));
            break;
          }
        }
      }
      else if (isImageBrush) {
        const gfx::Rect origBounds = brush->bounds();
        const int m = std::max(origBounds.w, origBounds.h);
        const gfx::Rect maxBounds(0, 0, m, m);
        ImageRef newImg(Image::create(brush->image()->pixelFormat(), m, m));
        ImageRef newMsk(Image::create(IMAGE_BITMAP, m, m));
        const color_t bg = brush->image()->maskColor();
        newImg->setMaskColor(bg);

        newImg->clear(bg);
        newMsk->clear(0);

        const gfx::Clip clip(0, 0, 0, 0, origBounds.w, origBounds.h);
        newImg->copy(brush->image(), clip);
        newMsk->copy(brush->maskBitmap(), clip);

        gfx::Rect cropBounds;

        switch (m_change) {

          case FlipD:
            flip_image(newImg.get(), maxBounds, FlipType::FlipDiagonal);
            flip_image(newMsk.get(), maxBounds, FlipType::FlipDiagonal);
            cropBounds = gfx::Rect(0, 0, origBounds.h, origBounds.w);
            break;

          case Rotate90CW:
            // To rotate 90cw:
            //
            //   A B -> C A
            //   C D    D B
            //
            // We can flip Y and then flip D:
            //
            //   A B -> C D -> C A
            //   C D    A B    D B
            //
            flip_image(newImg.get(), maxBounds, FlipType::FlipVertical);
            flip_image(newImg.get(), maxBounds, FlipType::FlipDiagonal);
            flip_image(newMsk.get(), maxBounds, FlipType::FlipVertical);
            flip_image(newMsk.get(), maxBounds, FlipType::FlipDiagonal);

            cropBounds = gfx::Rect(m - origBounds.h, 0,
                                   origBounds.h, origBounds.w);
            break;
        }

        ImageRef newImg2(crop_image(newImg.get(), cropBounds, bg));
        ImageRef newMsk2(crop_image(newMsk.get(), cropBounds, bg));

        BrushRef newBrush = std::make_shared<Brush>(*brush);
        newBrush->setImage(newImg.get(),
                           newMsk.get());
        contextBar->setActiveBrush(newBrush);
      }
      break;

    case CustomBrush:
      if (contextBar)
        contextBar->setActiveBrushBySlot(tool, m_slot);
      break;
  }

  // Notify the ActiveToolManager that the active brush changed, so we
  // can show the new brush in the real (non-quick) active tool.
  //
  // E.g. Space key activates the Hand quick tool, Space+H flips the
  // brush, but we are still in the Hand tool, in this case we'd like
  // to go back to the original tool (e.g. Pencil) and show the new
  // brush.
  atm->brushChanged();
}

std::string ChangeBrushCommand::onGetFriendlyName() const
{
  std::string change;
  switch (m_change) {
    case None: break;
    case IncrementSize: change = Strings::commands_ChangeBrush_IncrementSize(); break;
    case DecrementSize: change = Strings::commands_ChangeBrush_DecrementSize(); break;
    case IncrementAngle: change = Strings::commands_ChangeBrush_IncrementAngle(); break;
    case DecrementAngle: change = Strings::commands_ChangeBrush_DecrementAngle(); break;
    case FlipX: change = Strings::commands_ChangeBrush_FlipX(); break;
    case FlipY: change = Strings::commands_ChangeBrush_FlipY(); break;
    case FlipD: change = Strings::commands_ChangeBrush_FlipD(); break;
    case Rotate90CW: change = Strings::commands_ChangeBrush_Rotate90CW(); break;
    case CustomBrush:
      change = fmt::format(Strings::commands_ChangeBrush_CustomBrush(), m_slot);
      break;
  }
  return fmt::format(getBaseFriendlyName(), change);
}

Command* CommandFactory::createChangeBrushCommand()
{
  return new ChangeBrushCommand;
}

} // namespace app
