// Aseprite
// Copyright (C) 2001-2018  David Capello
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
#include "app/site.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "ui/system.h"

namespace app {

using namespace ui;

EyedropperCommand::EyedropperCommand()
  : Command(CommandId::Eyedropper(), CmdUIOnlyFlag)
{
  m_background = false;
}

void EyedropperCommand::pickSample(const Site& site,
                                   const gfx::PointF& pixelPos,
                                   const render::Projection& proj,
                                   app::Color& color)
{
  // Check if we've to grab alpha channel or the merged color.
  Preferences& pref = Preferences::instance();
  ColorPicker::Mode mode = ColorPicker::FromComposition;
  switch (pref.eyedropper.sample()) {
    case app::gen::EyedropperSample::ALL_LAYERS:
      mode = ColorPicker::FromComposition;
      break;
    case app::gen::EyedropperSample::CURRENT_LAYER:
      mode = ColorPicker::FromActiveLayer;
      break;
    case app::gen::EyedropperSample::FIRST_REFERENCE_LAYER:
      mode = ColorPicker::FromFirstReferenceLayer;
      break;
  }

  ColorPicker picker;
  picker.pickColor(site, pixelPos, proj, mode);

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
          color = app::Color::fromHsv(color.getHsvHue(),
                                      color.getHsvSaturation(),
                                      color.getHsvValue(),
                                      picked.getAlpha());
          break;

        case app::Color::HslType:
          color = app::Color::fromHsl(color.getHslHue(),
                                      color.getHslSaturation(),
                                      color.getHslLightness(),
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
        color = app::Color::fromHsv(picked.getHsvHue(),
                                    picked.getHsvSaturation(),
                                    picked.getHsvValue(),
                                    picked.getAlpha());
      break;
    case app::gen::EyedropperChannel::HSV:
      if (picked.getAlpha() > 0)
        color = app::Color::fromHsv(picked.getHsvHue(),
                                    picked.getHsvSaturation(),
                                    picked.getHsvValue(),
                                    color.getAlpha());
      break;
    case app::gen::EyedropperChannel::HSLA:
      if (picked.getType() == app::Color::HslType)
        color = picked;
      else
        color = app::Color::fromHsl(picked.getHslHue(),
                                    picked.getHslSaturation(),
                                    picked.getHslLightness(),
                                    picked.getAlpha());
      break;
    case app::gen::EyedropperChannel::HSL:
      if (picked.getAlpha() > 0)
        color = app::Color::fromHsl(picked.getHslHue(),
                                    picked.getHslSaturation(),
                                    picked.getHslLightness(),
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
  gfx::Point mousePos = ui::get_mouse_position();
  Widget* widget = ui::Manager::getDefault()->pick(mousePos);
  if (!widget || widget->type() != Editor::Type())
    return;

  Editor* editor = static_cast<Editor*>(widget);
  executeOnMousePos(context, editor, mousePos, !m_background);
}

void EyedropperCommand::executeOnMousePos(Context* context,
                                          Editor* editor,
                                          const gfx::Point& mousePos,
                                          const bool foreground)
{
  ASSERT(editor);

  Sprite* sprite = editor->sprite();
  if (!sprite)
    return;

  // Discard current image brush
  if (Preferences::instance().eyedropper.discardBrush()) {
    Command* discardBrush = Commands::instance()->byId(CommandId::DiscardBrush());
    context->executeCommand(discardBrush);
  }

  // Pixel position to get
  gfx::PointF pixelPos = editor->screenToEditorF(mousePos);

  // Start with fg/bg color
  DisableColorBarEditMode disable;
  Preferences& pref = Preferences::instance();
  app::Color color =
    foreground ? pref.colorBar.fgColor():
                 pref.colorBar.bgColor();

  pickSample(editor->getSite(),
             pixelPos,
             editor->projection(),
             color);

  if (foreground)
    pref.colorBar.fgColor(color);
  else
    pref.colorBar.bgColor(color);
}

Command* CommandFactory::createEyedropperCommand()
{
  return new EyedropperCommand;
}

} // namespace app
