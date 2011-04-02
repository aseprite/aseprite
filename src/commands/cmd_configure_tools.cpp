/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "base/bind.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "console.h"
#include "document_wrappers.h"
#include "gfx/size.h"
#include "gui/gui.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/rootmenu.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "settings/settings.h"
#include "skin/skin_parts.h"
#include "tools/ink.h"
#include "tools/point_shape.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "widgets/color_button.h"
#include "widgets/editor/editor.h"
#include "widgets/groupbut.h"
#include "widgets/statebar.h"

#include <allegro.h>

using namespace gfx;
using namespace tools;

static Frame* window = NULL;

static bool brush_preview_msg_proc(JWidget widget, JMessage msg);

static bool window_close_hook(JWidget widget, void *data);
static bool brush_type_change_hook(JWidget widget, void *data);

// Slot for App::Exit signal 
static void on_exit_delete_this_widget()
{
  ASSERT(window != NULL);
  jwidget_free(window);
}

static void on_pen_size_after_change()
{
  Slider* brush_size = window->findChildT<Slider>("brush_size");
  Widget* brush_preview = window->findChild("brush_preview");

  ASSERT(brush_size != NULL);

  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IToolSettings* tool_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool);

  brush_size->setValue(tool_settings->getPen()->getSize());

  // Regenerate the preview
  brush_preview->invalidate();
}

static void on_current_tool_change()
{
  Slider* brush_size = window->findChildT<Slider>("brush_size");
  Slider* brush_angle = window->findChildT<Slider>("brush_angle");
  Widget* brush_type = window->findChild("brush_type");
  Widget* brush_preview = window->findChild("brush_preview");
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
  group_button_select(brush_type, tool_settings->getPen()->getType());

  // Regenerate the preview
  brush_preview->invalidate();

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
      
    window->move_window(rect);
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
  Slider* m_brushSize;
  Slider* m_brushAngle;
  Slider* m_opacity;
  Slider* m_tolerance;
  Slider* m_sprayWidth;
  Slider* m_airSpeed;

  void onTiledClick();
  void onTiledXYClick(int tiled_axis, CheckBox* checkbox);
  void onViewGridClick();
  void onPixelGridClick();
  void onSetGridClick();
  void onSnapToGridClick();
  void onOnionSkinClick();
  void onBrushSizeSliderChange(Widget* brush_preview);
  void onBrushAngleSliderChange(Widget* brush_preview);
  void onOpacitySliderChange();
  void onToleranceSliderChange();
  void onSprayWidthSliderChange();
  void onAirSpeedSliderChange();

};

ConfigureTools::ConfigureTools()
  : Command("ConfigureTools",
	    "Configure Tools",
	    CmdUIOnlyFlag)
{
}

void ConfigureTools::onExecute(Context* context)
{
  Button* set_grid;
  JWidget brush_preview_box;
  JWidget brush_type_box, brush_type;
  JWidget brush_preview;
  bool first_time = false;

  if (!window) {
    window = static_cast<Frame*>(load_widget("tools_configuration.xml", "configure_tool"));
    first_time = true;
  }
  /* if the window is opened, close it */
  else if (window->isVisible()) {
    window->closeWindow(NULL);
    return;
  }

  try {
    get_widgets(window,
		"tiled", &m_tiled,
		"tiled_x", &m_tiledX,
		"tiled_y", &m_tiledY,
		"snap_to_grid", &m_snapToGrid,
		"view_grid", &m_viewGrid,
		"pixel_grid", &m_pixelGrid,
		"set_grid", &set_grid,
		"brush_size", &m_brushSize,
		"brush_angle", &m_brushAngle,
		"opacity", &m_opacity,
		"tolerance", &m_tolerance,
		"spray_width", &m_sprayWidth,
		"air_speed", &m_airSpeed,
		"brush_preview_box", &brush_preview_box,
		"brush_type_box", &brush_type_box,
		"onionskin", &m_onionSkin, NULL);
  }
  catch (...) {
    jwidget_free(window);
    window = NULL;
    throw;
  }

  /* brush-preview */
  if (first_time) {
    brush_preview = new Widget(JI_WIDGET);
    brush_preview->min_w = 32 + 4;
    brush_preview->min_h = 32 + 4;

    brush_preview->setName("brush_preview");
    jwidget_add_hook(brush_preview, JI_WIDGET,
		     brush_preview_msg_proc, NULL);
  }
  else {
    brush_preview = window->findChild("brush_preview");
  }

  // Current settings
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();
  IToolSettings* tool_settings = settings->getToolSettings(current_tool);

  /* brush-type */
  if (first_time) {
    PenType type = tool_settings->getPen()->getType();

    brush_type = group_button_new(3, 1, type,
				  PART_BRUSH_CIRCLE,
				  PART_BRUSH_SQUARE,
				  PART_BRUSH_LINE);

    brush_type->setName("brush_type");
  }
  else {
    brush_type = window->findChild("brush_type");
  }

  if (settings->getTiledMode() != TILED_NONE) {
    m_tiled->setSelected(true);
    if (settings->getTiledMode() & TILED_X_AXIS) m_tiledX->setSelected(true);
    if (settings->getTiledMode() & TILED_Y_AXIS) m_tiledY->setSelected(true);
  }
      
  if (settings->getSnapToGrid()) m_snapToGrid->setSelected(true);
  if (settings->getGridVisible()) m_viewGrid->setSelected(true);
  if (settings->getPixelGridVisible()) m_pixelGrid->setSelected(true);
  if (settings->getUseOnionskin()) m_onionSkin->setSelected(true);

  if (first_time) {
    // Append children
    brush_preview_box->addChild(brush_preview);
    brush_type_box->addChild(brush_type);

    // Slots
    window->Close.connect(Bind<bool>(&window_close_hook, (JWidget)window, (void*)0));
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
    m_brushSize->Change.connect(Bind<void>(&ConfigureTools::onBrushSizeSliderChange, this, brush_preview));
    m_brushAngle->Change.connect(Bind<void>(&ConfigureTools::onBrushAngleSliderChange, this, brush_preview));
    m_opacity->Change.connect(&ConfigureTools::onOpacitySliderChange, this);
    m_tolerance->Change.connect(&ConfigureTools::onToleranceSliderChange, this);
    m_airSpeed->Change.connect(&ConfigureTools::onAirSpeedSliderChange, this);
    m_sprayWidth->Change.connect(&ConfigureTools::onSprayWidthSliderChange, this);

    HOOK(brush_type, SIGNAL_GROUP_BUTTON_CHANGE, brush_type_change_hook, brush_preview);
  }

  // Update current pen properties
  on_current_tool_change();

  // Default position
  window->remap_window();
  window->center_window();

  // Load window configuration
  load_window_pos(window, "ConfigureTool");

  window->open_window_bg();
}

static bool brush_preview_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DRAW: {
      BITMAP *bmp = create_bitmap(jrect_w(widget->rc),
				  jrect_h(widget->rc));

      Tool* current_tool = UIContext::instance()
	->getSettings()
	->getCurrentTool();

      IPenSettings* pen_settings = UIContext::instance()
	->getSettings()
	->getToolSettings(current_tool)
	->getPen();

      ASSERT(pen_settings != NULL);

      Pen* pen = new Pen(pen_settings->getType(),
			 pen_settings->getSize(),
			 pen_settings->getAngle());

      clear_to_color(bmp, makecol(0, 0, 0));
      image_to_allegro(pen->get_image(), bmp,
		       bmp->w/2 - pen->get_size()/2,
		       bmp->h/2 - pen->get_size()/2, NULL);
      blit(bmp, ji_screen, 0, 0, widget->rc->x1, widget->rc->y1,
	   bmp->w, bmp->h);
      destroy_bitmap(bmp);

      delete pen;
      return true;
    }
  }

  return false;
}

static bool window_close_hook(JWidget widget, void *data)
{
  /* isn't running anymore */
/*   window = NULL; */

  /* save window configuration */
  save_window_pos(widget, "ConfigureTool");

  return false;
}

static bool brush_type_change_hook(JWidget widget, void *data)
{
  PenType type = (PenType)group_button_get_selected(widget);

  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen()
    ->setType(type);

  ((Widget*)data)->invalidate();

  app_get_statusbar()
    ->setStatusText(250,
		    "Pen shape: %s",
		    type == PEN_TYPE_CIRCLE ? "Circle":
		    type == PEN_TYPE_SQUARE ? "Square":
		    type == PEN_TYPE_LINE ? "Line": "Unknown");

  return true;
}

void ConfigureTools::onBrushSizeSliderChange(Widget* brush_preview)
{
  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen()
    ->setSize(m_brushSize->getValue());

  brush_preview->invalidate();
}

void ConfigureTools::onBrushAngleSliderChange(Widget* brush_preview)
{
  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen()
    ->setAngle(m_brushAngle->getValue());

  brush_preview->invalidate();
}

void ConfigureTools::onOpacitySliderChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();

  settings->getToolSettings(current_tool)->setOpacity(m_opacity->getValue());
}

void ConfigureTools::onToleranceSliderChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();

  settings->getToolSettings(current_tool)->setTolerance(m_tolerance->getValue());
}

void ConfigureTools::onSprayWidthSliderChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();

  settings->getToolSettings(current_tool)->setSprayWidth(m_sprayWidth->getValue());
}

void ConfigureTools::onAirSpeedSliderChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();

  settings->getToolSettings(current_tool)->setSpraySpeed(m_airSpeed->getValue());
}

void ConfigureTools::onTiledClick()
{
  bool flag = m_tiled->isSelected();

  UIContext::instance()->getSettings()->setTiledMode(flag ? TILED_BOTH: TILED_NONE);

  m_tiledX->setSelected(flag);
  m_tiledY->setSelected(flag);
}

void ConfigureTools::onTiledXYClick(int tiled_axis, CheckBox* checkbox)
{
  int tiled_mode = UIContext::instance()->getSettings()->getTiledMode();

  if (checkbox->isSelected())
    tiled_mode |= tiled_axis;
  else
    tiled_mode &= ~tiled_axis;

  checkbox->findSibling("tiled")->setSelected(tiled_mode != TILED_NONE);

  UIContext::instance()->getSettings()->setTiledMode((TiledMode)tiled_mode);
}

void ConfigureTools::onSnapToGridClick()
{
  UIContext::instance()->getSettings()->setSnapToGrid(m_snapToGrid->isSelected());
}

void ConfigureTools::onViewGridClick()
{
  UIContext::instance()->getSettings()->setGridVisible(m_viewGrid->isSelected());
  refresh_all_editors();
}

void ConfigureTools::onPixelGridClick()
{
  UIContext::instance()->getSettings()->setPixelGridVisible(m_pixelGrid->isSelected());
  refresh_all_editors();
}

void ConfigureTools::onSetGridClick()
{
  try {
    // TODO use the same context as in ConfigureTools::onExecute
    const ActiveDocumentReader document(UIContext::instance());

    if (document && document->isMaskVisible()) {
      const Mask* mask(document->getMask());
      Rect bounds(mask->x, mask->y, mask->w, mask->h);

      UIContext::instance()->getSettings()->setGridBounds(bounds);

      if (UIContext::instance()->getSettings()->getGridVisible())
	refresh_all_editors();
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
  UIContext::instance()->getSettings()->setUseOnionskin(m_onionSkin->isSelected());
  refresh_all_editors();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createConfigureToolsCommand()
{
  return new ConfigureTools;
}
