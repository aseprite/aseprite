// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_picker.h"
#include "app/commands/cmd_eyedropper.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/image.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "ui/system.h"

namespace app {

using namespace ui;

EyedropperCommand::EyedropperCommand()
  : Command("Eyedropper",
            "Eyedropper",
            CmdUIOnlyFlag)
{
  m_background = false;
}

void EyedropperCommand::pickSample(const doc::Site& site,
                                   const gfx::Point& pixelPos,
                                   app::Color& color)
{
  // Check if we've to grab alpha channel or the merged color.
  Preferences& pref = Preferences::instance();
  bool allLayers =
    (pref.eyedropper.sample() == app::gen::EyedropperSample::ALL_LAYERS);

  ColorPicker picker;
  picker.pickColor(site,
                   pixelPos,
                   (allLayers ?
                    ColorPicker::FromComposition:
                    ColorPicker::FromActiveLayer));

  app::gen::EyedropperChannel channel =
    pref.eyedropper.channel();

  app::Color picked = picker.color();

  switch (channel) {
    case app::gen::EyedropperChannel::COLOR_ALPHA:
      color = picked;
      break;
    case app::gen::EyedropperChannel::COLOR:
      if (picked.getAlpha() > 0)
        color = app::Color::fromRgb(picked.getRed(),
                                    picked.getGreen(),
                                    picked.getBlue(),
                                    color.getAlpha());
      break;
    case app::gen::EyedropperChannel::ALPHA:
      switch (color.getType()) {

        case app::Color::RgbType:
        case app::Color::IndexType:
          color = app::Color::fromRgb(color.getRed(),
                                      color.getGreen(),
                                      color.getBlue(),
                                      picked.getAlpha());
          break;

        case app::Color::HsvType:
          color = app::Color::fromHsv(color.getHue(),
                                      color.getSaturation(),
                                      color.getValue(),
                                      picked.getAlpha());
          break;

        case app::Color::GrayType:
          color = app::Color::fromGray(color.getGray(),
                                       picked.getAlpha());
          break;

      }
      break;
    case app::gen::EyedropperChannel::RGBA:
      if (picked.getType() == app::Color::RgbType)
        color = picked;
      else
        color = app::Color::fromRgb(picked.getRed(),
                                    picked.getGreen(),
                                    picked.getBlue(),
                                    picked.getAlpha());
      break;
    case app::gen::EyedropperChannel::RGB:
      if (picked.getAlpha() > 0)
        color = app::Color::fromRgb(picked.getRed(),
                                    picked.getGreen(),
                                    picked.getBlue(),
                                    color.getAlpha());
      break;
    case app::gen::EyedropperChannel::HSVA:
      if (picked.getType() == app::Color::HsvType)
        color = picked;
      else
        color = app::Color::fromHsv(picked.getHue(),
                                    picked.getSaturation(),
                                    picked.getValue(),
                                    picked.getAlpha());
      break;
    case app::gen::EyedropperChannel::HSV:
      if (picked.getAlpha() > 0)
        color = app::Color::fromHsv(picked.getHue(),
                                    picked.getSaturation(),
                                    picked.getValue(),
                                    color.getAlpha());
      break;
    case app::gen::EyedropperChannel::GRAYA:
      if (picked.getType() == app::Color::GrayType)
        color = picked;
      else
        color = app::Color::fromGray(picked.getGray(),
                                     picked.getAlpha());
      break;
    case app::gen::EyedropperChannel::GRAY:
      if (picked.getAlpha() > 0)
        color = app::Color::fromGray(picked.getGray(),
                                     color.getAlpha());
      break;
    case app::gen::EyedropperChannel::INDEX:
      color = app::Color::fromIndex(picked.getIndex());
      break;
  }
}

void EyedropperCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;
}

void EyedropperCommand::onExecute(Context* context)
{
  Widget* widget = ui::Manager::getDefault()->getMouse();
  if (!widget || widget->type() != editor_type())
    return;

  Editor* editor = static_cast<Editor*>(widget);
  Sprite* sprite = editor->sprite();
  if (!sprite)
    return;

  // Discard current image brush
  {
    Command* discardBrush = CommandsModule::instance()->getCommandByName(CommandId::DiscardBrush);
    context->executeCommand(discardBrush);
  }

  // Pixel position to get
  gfx::Point pixelPos = editor->screenToEditor(ui::get_mouse_position());

  // Start with fg/bg color
  Preferences& pref = Preferences::instance();
  app::Color color =
    m_background ? pref.colorBar.bgColor():
                   pref.colorBar.fgColor();

  pickSample(editor->getSite(), pixelPos, color);

  if (m_background)
    pref.colorBar.bgColor(color);
  else
    pref.colorBar.fgColor(color);
}

Command* CommandFactory::createEyedropperCommand()
{
  return new EyedropperCommand;
}

} // namespace app
