/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "app.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "base/bind.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "console.h"
#include "document_wrappers.h"
#include "gfx/size.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "settings/document_settings.h"
#include "settings/settings.h"
#include "skin/skin_parts.h"
#include "tools/ink.h"
#include "tools/point_shape.h"
#include "tools/tool.h"
#include "ui/gui.h"
#include "ui_context.h"
#include "widgets/button_set.h"
#include "widgets/color_button.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"

#include <allegro.h>

using namespace gfx;
using namespace ui;
using namespace tools;

static Window* window = NULL;

static bool window_close_hook(Widget* widget, void *data);

class BrushPreview : public Widget
{
public:
  BrushPreview() : Widget(JI_WIDGET) {
  }

protected:
  bool onProcessMessage(Message* msg)
  {
    switch (msg->type) {

      case JM_DRAW: {
        BITMAP *bmp = create_bitmap(jrect_w(this->rc),
                                    jrect_h(this->rc));

        Tool* current_tool = UIContext::instance()
          ->getSettings()
          ->getCurrentTool();

        IPenSettings* pen_settings = UIContext::instance()
          ->getSettings()
          ->getToolSettings(current_tool)
          ->getPen();

        ASSERT(pen_settings != NULL);

        UniquePtr<Pen> pen(new Pen(pen_settings->getType(),
                                   pen_settings->getSize(),
                                   pen_settings->getAngle()));

        clear_to_color(bmp, makecol(0, 0, 0));
        image_to_allegro(pen->get_image(), bmp,
                         bmp->w/2 - pen->get_size()/2,
                         bmp->h/2 - pen->get_size()/2, NULL);
        blit(bmp, ji_screen, 0, 0, this->rc->x1, this->rc->y1,
             bmp->w, bmp->h);
        destroy_bitmap(bmp);
        return true;
      }
    }

    return Widget::onProcessMessage(msg);
  }
};

// Slot for App::Exit signal
static void on_exit_delete_this_widget()
{
  ASSERT(window != NULL);
  delete window;
}

static void on_pen_size_after_change()
{
  Slider* brush_size = window->findChildT<Slider>("brush_size");
  BrushPreview* brushPreview = window->findChildT<BrushPreview>("brush_preview");

  ASSERT(brush_size != NULL);

  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IToolSettings* tool_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool);

  brush_size->setValue(tool_settings->getPen()->getSize());

  // Regenerate the preview
  brushPreview->invalidate();
}

static void on_current_tool_change()
{
  Slider* brush_size = window->findChildT<Slider>("brush_size");
  Slider* brush_angle = window->findChildT<Slider>("brush_angle");
  ButtonSet* brush_type = window->findChildT<ButtonSet>("brush_type");
  BrushPreview* brushPreview = window->findChildT<BrushPreview>("brush_preview");
  Widget* opacity_label = window->findChild("opacity_label");
  Slider* opacity = window->findChildT<Slider>("opacity");
  Widget* tolerance_label = window->findChild("tolerance_label");
  Slider* tolerance = window->findChildT<Slider>("tolerance");
  Widget* spray_box = window->findChild("spray_box");
  Slider* spray_width = window->findChildT<Slider>("spray_width");
  Slider* air_speed = window->findChildT<Slider>("air_speed");

  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IToolSettings* tool_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool);

  opacity->setValue(tool_settings->getOpacity());
  tolerance->setValue(tool_settings->getTolerance());
  brush_size->setValue(tool_settings->getPen()->getSize());
  brush_angle->setValue(tool_settings->getPen()->getAngle());
  spray_width->setValue(tool_settings->getSprayWidth());
  air_speed->setValue(tool_settings->getSpraySpeed());

  // Select the brush type
  brush_type->setSelectedItem(tool_settings->getPen()->getType());

  // Regenerate the preview
  brushPreview->invalidate();

  // True if the current tool needs opacity options
  bool hasOpacity = (current_tool->getInk(0)->isPaint() ||
                     current_tool->getInk(0)->isEffect() ||
                     current_tool->getInk(1)->isPaint() ||
                     current_tool->getInk(1)->isEffect());

  // True if the current tool needs tolerance options
  bool hasTolerance = (current_tool->getPointShape(0)->isFloodFill() ||
                       current_tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs spray options
  bool hasSprayOptions = (current_tool->getPointShape(0)->isSpray() ||
                          current_tool->getPointShape(1)->isSpray());

  // Show/Hide parameters
  opacity_label->setVisible(hasOpacity);
  opacity->setVisible(hasOpacity);
  tolerance_label->setVisible(hasTolerance);
  tolerance->setVisible(hasTolerance);
  spray_box->setVisible(hasSprayOptions);

  // Get the required size of the whole window
  Size reqSize = window->getPreferredSize();
  if (jrect_w(window->rc) < reqSize.w ||
      jrect_h(window->rc) != reqSize.h) {
    JRect rect = jrect_new(window->rc->x1, window->rc->y1,
                           (jrect_w(window->rc) < reqSize.w) ? window->rc->x1 + reqSize.w:
                                                               window->rc->x2,
                           window->rc->y1 + reqSize.h);

    // Show the expanded area inside the screen
    if (rect->y2 > JI_SCREEN_H)
      jrect_displace(rect, 0, JI_SCREEN_H - rect->y2);

    window->moveWindow(rect);
    jrect_free(rect);
  }
  else
    window->setBounds(window->getBounds()); // TODO layout() method is missing

  // Redraw the window
  window->invalidate();
}

//////////////////////////////////////////////////////////////////////

class ConfigureTools : public Command
{
public:
  ConfigureTools();
  Command* clone() const { return new ConfigureTools(*this); }

protected:
  void onExecute(Context* context);

private:
  CheckBox* m_tiled;
  CheckBox* m_tiledX;
  CheckBox* m_tiledY;
  CheckBox* m_pixelGrid;
  CheckBox* m_snapToGrid;
  CheckBox* m_onionSkin;
  CheckBox* m_viewGrid;
  Widget* m_brushPreview;
  ButtonSet* m_brushType;
  Slider* m_brushSize;
  Slider* m_brushAngle;
  Slider* m_opacity;
  Slider* m_tolerance;
  Slider* m_sprayWidth;
  Slider* m_airSpeed;
  ISettings* m_settings;
  IDocumentSettings* m_docSettings;

  void onWindowClose();
  void onTiledClick();
  void onTiledXYClick(int tiled_axis, CheckBox* checkbox);
  void onViewGridClick();
  void onPixelGridClick();
  void onSetGridClick();
  void onSnapToGridClick();
  void onOnionSkinClick();
  void onBrushSizeSliderChange();
  void onBrushAngleSliderChange();
  void onOpacitySliderChange();
  void onToleranceSliderChange();
  void onSprayWidthSliderChange();
  void onAirSpeedSliderChange();
  void onBrushTypeChange();

};

ConfigureTools::ConfigureTools()
  : Command("ConfigureTools",
            "Configure Tools",
            CmdUIOnlyFlag)
{
  m_settings = NULL;
  m_docSettings = NULL;
}

void ConfigureTools::onExecute(Context* context)
{
  m_settings = UIContext::instance()->getSettings();
  m_docSettings = m_settings->getDocumentSettings(NULL);

  Button* set_grid;
  Widget* brush_preview_box;
  Widget* brush_type_box;
  bool first_time = false;

  if (!window) {
    window = app::load_widget<Window>("tools_configuration.xml", "configure_tool");
    first_time = true;
  }
  // If the window is opened, close it
  else if (window->isVisible()) {
    window->closeWindow(NULL);
    return;
  }

  try {
    m_tiled = app::find_widget<CheckBox>(window, "tiled");
    m_tiledX = app::find_widget<CheckBox>(window, "tiled_x");
    m_tiledY = app::find_widget<CheckBox>(window, "tiled_y");
    m_snapToGrid = app::find_widget<CheckBox>(window, "snap_to_grid");
    m_viewGrid = app::find_widget<CheckBox>(window, "view_grid");
    m_pixelGrid = app::find_widget<CheckBox>(window, "pixel_grid");
    set_grid = app::find_widget<Button>(window, "set_grid");
    m_brushSize = app::find_widget<Slider>(window, "brush_size");
    m_brushAngle = app::find_widget<Slider>(window, "brush_angle");
    m_opacity = app::find_widget<Slider>(window, "opacity");
    m_tolerance = app::find_widget<Slider>(window, "tolerance");
    m_sprayWidth = app::find_widget<Slider>(window, "spray_width");
    m_airSpeed = app::find_widget<Slider>(window, "air_speed");
    brush_preview_box = app::find_widget<Widget>(window, "brush_preview_box");
    brush_type_box = app::find_widget<Widget>(window, "brush_type_box");
    m_onionSkin = app::find_widget<CheckBox>(window, "onionskin");
  }
  catch (...) {
    delete window;
    window = NULL;
    throw;
  }

  /* brush-preview */
  if (first_time) {
    m_brushPreview = new BrushPreview();
    m_brushPreview->min_w = 32 + 4;
    m_brushPreview->min_h = 32 + 4;
    m_brushPreview->setId("brush_preview");
  }
  else {
    m_brushPreview = window->findChild("brush_preview");
  }

  // Current settings
  Tool* current_tool = m_settings->getCurrentTool();
  IToolSettings* tool_settings = m_settings->getToolSettings(current_tool);

  /* brush-type */
  if (first_time) {
    PenType type = tool_settings->getPen()->getType();

    m_brushType = new ButtonSet(3, 1, type,
                                PART_BRUSH_CIRCLE,
                                PART_BRUSH_SQUARE,
                                PART_BRUSH_LINE);

    m_brushType->setId("brush_type");
  }
  else {
    m_brushType = window->findChildT<ButtonSet>("brush_type");
  }

  if (m_docSettings->getTiledMode() != TILED_NONE) {
    m_tiled->setSelected(true);
    if (m_docSettings->getTiledMode() & TILED_X_AXIS) m_tiledX->setSelected(true);
    if (m_docSettings->getTiledMode() & TILED_Y_AXIS) m_tiledY->setSelected(true);
  }

  if (m_docSettings->getSnapToGrid()) m_snapToGrid->setSelected(true);
  if (m_docSettings->getGridVisible()) m_viewGrid->setSelected(true);
  if (m_docSettings->getPixelGridVisible()) m_pixelGrid->setSelected(true);
  if (m_docSettings->getUseOnionskin()) m_onionSkin->setSelected(true);

  if (first_time) {
    // Append children
    brush_preview_box->addChild(m_brushPreview);
    brush_type_box->addChild(m_brushType);

    // Slots
    window->Close.connect(Bind<void>(&ConfigureTools::onWindowClose, this));
    m_tiled->Click.connect(Bind<void>(&ConfigureTools::onTiledClick, this));
    m_tiledX->Click.connect(Bind<void>(&ConfigureTools::onTiledXYClick, this, TILED_X_AXIS, m_tiledX));
    m_tiledY->Click.connect(Bind<void>(&ConfigureTools::onTiledXYClick, this, TILED_Y_AXIS, m_tiledY));
    m_viewGrid->Click.connect(Bind<void>(&ConfigureTools::onViewGridClick, this));
    m_pixelGrid->Click.connect(Bind<void>(&ConfigureTools::onPixelGridClick, this));
    set_grid->Click.connect(Bind<void>(&ConfigureTools::onSetGridClick, this));
    m_snapToGrid->Click.connect(Bind<void>(&ConfigureTools::onSnapToGridClick, this));
    m_onionSkin->Click.connect(Bind<void>(&ConfigureTools::onOnionSkinClick, this));

    App::instance()->Exit.connect(&on_exit_delete_this_widget);
    App::instance()->PenSizeAfterChange.connect(&on_pen_size_after_change);
    App::instance()->CurrentToolChange.connect(&on_current_tool_change);

    // Append hooks
    m_brushSize->Change.connect(Bind<void>(&ConfigureTools::onBrushSizeSliderChange, this));
    m_brushAngle->Change.connect(Bind<void>(&ConfigureTools::onBrushAngleSliderChange, this));
    m_opacity->Change.connect(&ConfigureTools::onOpacitySliderChange, this);
    m_tolerance->Change.connect(&ConfigureTools::onToleranceSliderChange, this);
    m_airSpeed->Change.connect(&ConfigureTools::onAirSpeedSliderChange, this);
    m_sprayWidth->Change.connect(&ConfigureTools::onSprayWidthSliderChange, this);
    m_brushType->ItemChange.connect(&ConfigureTools::onBrushTypeChange, this);
  }

  // Update current pen properties
  on_current_tool_change();

  // Default position
  window->remapWindow();
  window->centerWindow();

  // Load window configuration
  load_window_pos(window, "ConfigureTool");

  window->openWindow();
}

void ConfigureTools::onWindowClose()
{
  save_window_pos(window, "ConfigureTool");
}

void ConfigureTools::onBrushTypeChange()
{
  PenType type = (PenType)m_brushType->getSelectedItem();

  Tool* current_tool = m_settings->getCurrentTool();
  m_settings->getToolSettings(current_tool)
    ->getPen()
    ->setType(type);

  m_brushPreview->invalidate();

  StatusBar::instance()
    ->setStatusText(250,
                    "Pen shape: %s",
                    type == PEN_TYPE_CIRCLE ? "Circle":
                    type == PEN_TYPE_SQUARE ? "Square":
                    type == PEN_TYPE_LINE ? "Line": "Unknown");
}

void ConfigureTools::onBrushSizeSliderChange()
{
  Tool* current_tool = m_settings->getCurrentTool();
  m_settings->getToolSettings(current_tool)
    ->getPen()
    ->setSize(m_brushSize->getValue());

  m_brushPreview->invalidate();
}

void ConfigureTools::onBrushAngleSliderChange()
{
  Tool* current_tool = m_settings->getCurrentTool();
  m_settings->getToolSettings(current_tool)
    ->getPen()
    ->setAngle(m_brushAngle->getValue());

  m_brushPreview->invalidate();
}

void ConfigureTools::onOpacitySliderChange()
{
  Tool* current_tool = m_settings->getCurrentTool();

  m_settings->getToolSettings(current_tool)->setOpacity(m_opacity->getValue());
}

void ConfigureTools::onToleranceSliderChange()
{
  Tool* current_tool = m_settings->getCurrentTool();

  m_settings->getToolSettings(current_tool)->setTolerance(m_tolerance->getValue());
}

void ConfigureTools::onSprayWidthSliderChange()
{
  Tool* current_tool = m_settings->getCurrentTool();

  m_settings->getToolSettings(current_tool)->setSprayWidth(m_sprayWidth->getValue());
}

void ConfigureTools::onAirSpeedSliderChange()
{
  Tool* current_tool = m_settings->getCurrentTool();

  m_settings->getToolSettings(current_tool)->setSpraySpeed(m_airSpeed->getValue());
}

void ConfigureTools::onTiledClick()
{
  bool flag = m_tiled->isSelected();

  m_docSettings->setTiledMode(flag ? TILED_BOTH: TILED_NONE);

  m_tiledX->setSelected(flag);
  m_tiledY->setSelected(flag);
}

void ConfigureTools::onTiledXYClick(int tiled_axis, CheckBox* checkbox)
{
  int tiled_mode = m_docSettings->getTiledMode();

  if (checkbox->isSelected())
    tiled_mode |= tiled_axis;
  else
    tiled_mode &= ~tiled_axis;

  checkbox->findSibling("tiled")->setSelected(tiled_mode != TILED_NONE);

  m_docSettings->setTiledMode((TiledMode)tiled_mode);
}

void ConfigureTools::onSnapToGridClick()
{
  m_docSettings->setSnapToGrid(m_snapToGrid->isSelected());
}

void ConfigureTools::onViewGridClick()
{
  m_docSettings->setGridVisible(m_viewGrid->isSelected());
}

void ConfigureTools::onPixelGridClick()
{
  m_docSettings->setPixelGridVisible(m_pixelGrid->isSelected());
}

void ConfigureTools::onSetGridClick()
{
  try {
    // TODO use the same context as in ConfigureTools::onExecute
    const ActiveDocumentReader document(UIContext::instance());

    if (document && document->isMaskVisible()) {
      const Mask* mask(document->getMask());

      m_docSettings->setGridBounds(mask->getBounds());
    }
    else {
      Command* grid_settings_cmd =
        CommandsModule::instance()->getCommandByName(CommandId::GridSettings);

      UIContext::instance()->executeCommand(grid_settings_cmd, NULL);
    }
  }
  catch (LockedDocumentException& e) {
    Console::showException(e);
  }
}

void ConfigureTools::onOnionSkinClick()
{
  m_docSettings->setUseOnionskin(m_onionSkin->isSelected());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createConfigureToolsCommand()
{
  return new ConfigureTools;
}
