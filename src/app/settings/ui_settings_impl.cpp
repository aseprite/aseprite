/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/settings/ui_settings_impl.h"

#include "app/app.h"
#include "app/color_swatches.h"
#include "app/ini_file.h"
#include "app/settings/document_settings.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "base/observable.h"
#include "ui/manager.h"

#include <algorithm>
#include <allegro/color.h>
#include <string>

namespace app {

using namespace gfx;
using namespace raster;
using namespace filters;

namespace {

class UIDocumentSettingsImpl : public IDocumentSettings,
                               public base::Observable<DocumentSettingsObserver> {
public:
  UIDocumentSettingsImpl()
    : m_tiledMode((TiledMode)get_config_int("Tools", "Tiled", (int)TILED_NONE))
    , m_use_onionskin(get_config_bool("Onionskin", "Enabled", false))
    , m_prev_frames_onionskin(get_config_int("Onionskin", "PrevFrames", 1))
    , m_next_frames_onionskin(get_config_int("Onionskin", "NextFrames", 0))
    , m_onionskin_opacity_base(get_config_int("Onionskin", "OpacityBase", 128))
    , m_onionskin_opacity_step(get_config_int("Onionskin", "OpacityStep", 32))
    , m_snapToGrid(get_config_bool("Grid", "SnapTo", false))
    , m_gridVisible(get_config_bool("Grid", "Visible", false))
    , m_gridBounds(get_config_rect("Grid", "Bounds", Rect(0, 0, 16, 16)))
    , m_gridColor(get_config_color("Grid", "Color", app::Color::fromRgb(0, 0, 255)))
    , m_pixelGridColor(get_config_color("PixelGrid", "Color", app::Color::fromRgb(200, 200, 200)))
    , m_pixelGridVisible(get_config_bool("PixelGrid", "Visible", false))
  {
    m_tiledMode = (TiledMode)MID(0, (int)m_tiledMode, (int)TILED_BOTH);
  }

  ~UIDocumentSettingsImpl()
  {
    set_config_int("Tools", "Tiled", m_tiledMode);
    set_config_bool("Grid", "SnapTo", m_snapToGrid);
    set_config_bool("Grid", "Visible", m_gridVisible);
    set_config_rect("Grid", "Bounds", m_gridBounds);
    set_config_color("Grid", "Color", m_gridColor);
    set_config_bool("PixelGrid", "Visible", m_pixelGridVisible);
    set_config_color("PixelGrid", "Color", m_pixelGridColor);

    set_config_bool("Onionskin", "Enabled", m_use_onionskin);
    set_config_int("Onionskin", "PrevFrames", m_prev_frames_onionskin);
    set_config_int("Onionskin", "NextFrames", m_next_frames_onionskin);
    set_config_int("Onionskin", "OpacityBase", m_onionskin_opacity_base);
    set_config_int("Onionskin", "OpacityStep", m_onionskin_opacity_step);
  }

  // Tiled mode

  virtual TiledMode getTiledMode() OVERRIDE;
  virtual void setTiledMode(TiledMode mode) OVERRIDE;

  // Grid settings

  virtual bool getSnapToGrid() OVERRIDE;
  virtual bool getGridVisible() OVERRIDE;
  virtual gfx::Rect getGridBounds() OVERRIDE;
  virtual app::Color getGridColor() OVERRIDE;

  virtual void setSnapToGrid(bool state) OVERRIDE;
  virtual void setGridVisible(bool state) OVERRIDE;
  virtual void setGridBounds(const gfx::Rect& rect) OVERRIDE;
  virtual void setGridColor(const app::Color& color) OVERRIDE;

  virtual void snapToGrid(gfx::Point& point) const OVERRIDE;

  // Pixel grid

  virtual bool getPixelGridVisible() OVERRIDE;
  virtual app::Color getPixelGridColor() OVERRIDE;

  virtual void setPixelGridVisible(bool state) OVERRIDE;
  virtual void setPixelGridColor(const app::Color& color) OVERRIDE;

  // Onionskin settings

  virtual bool getUseOnionskin() OVERRIDE;
  virtual int getOnionskinPrevFrames() OVERRIDE;
  virtual int getOnionskinNextFrames() OVERRIDE;
  virtual int getOnionskinOpacityBase() OVERRIDE;
  virtual int getOnionskinOpacityStep() OVERRIDE;

  virtual void setUseOnionskin(bool state) OVERRIDE;
  virtual void setOnionskinPrevFrames(int frames) OVERRIDE;
  virtual void setOnionskinNextFrames(int frames) OVERRIDE;
  virtual void setOnionskinOpacityBase(int base) OVERRIDE;
  virtual void setOnionskinOpacityStep(int step) OVERRIDE;

  virtual void addObserver(DocumentSettingsObserver* observer) OVERRIDE;
  virtual void removeObserver(DocumentSettingsObserver* observer) OVERRIDE;

private:
  void redrawDocumentViews() {
    // TODO Redraw only document's views
    ui::Manager::getDefault()->invalidate();
  }

  TiledMode m_tiledMode;
  bool m_use_onionskin;
  int m_prev_frames_onionskin;
  int m_next_frames_onionskin;
  int m_onionskin_opacity_base;
  int m_onionskin_opacity_step;
  bool m_snapToGrid;
  bool m_gridVisible;
  gfx::Rect m_gridBounds;
  app::Color m_gridColor;
  bool m_pixelGridVisible;
  app::Color m_pixelGridColor;
};

class UISelectionSettingsImpl
    : public ISelectionSettings
    , public base::Observable<SelectionSettingsObserver> {
public:
  UISelectionSettingsImpl();
  ~UISelectionSettingsImpl();

  SelectionMode getSelectionMode();
  app::Color getMoveTransparentColor();
  RotationAlgorithm getRotationAlgorithm();

  void setSelectionMode(SelectionMode mode);
  void setMoveTransparentColor(app::Color color);
  void setRotationAlgorithm(RotationAlgorithm algorithm);

  void addObserver(SelectionSettingsObserver* observer);
  void removeObserver(SelectionSettingsObserver* observer);

private:
  SelectionMode m_selectionMode;
  app::Color m_moveTransparentColor;
  RotationAlgorithm m_rotationAlgorithm;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// UISettingsImpl

UISettingsImpl::UISettingsImpl()
  : m_currentTool(NULL)
  , m_globalDocumentSettings(new UIDocumentSettingsImpl)
  , m_colorSwatches(NULL)
  , m_selectionSettings(new UISelectionSettingsImpl)
  , m_showSpriteEditorScrollbars(get_config_bool("Options", "ShowScrollbars", true))
  , m_grabAlpha(get_config_bool("Options", "GrabAlpha", false))
{
  m_colorSwatches = new app::ColorSwatches("Default");
  for (size_t i=0; i<16; ++i)
    m_colorSwatches->addColor(app::Color::fromIndex(i));

  addColorSwatches(m_colorSwatches);
}

UISettingsImpl::~UISettingsImpl()
{
  set_config_bool("Options", "ShowScrollbars", m_showSpriteEditorScrollbars);
  set_config_bool("Options", "GrabAlpha", m_grabAlpha);

  delete m_globalDocumentSettings;

  // Delete all tool settings.
  for (std::map<std::string, IToolSettings*>::iterator
         it = m_toolSettings.begin(), end = m_toolSettings.end(); it != end; ++it) {
    delete it->second;
  }

  // Delete all color swatches
  for (std::vector<app::ColorSwatches*>::iterator
         it = m_colorSwatchesStore.begin(), end = m_colorSwatchesStore.end();
       it != end; ++it) {
    delete *it;
  }
}

//////////////////////////////////////////////////////////////////////
// General settings

bool UISettingsImpl::getShowSpriteEditorScrollbars()
{
  return m_showSpriteEditorScrollbars;
}

bool UISettingsImpl::getGrabAlpha()
{
  return m_grabAlpha;
}

app::Color UISettingsImpl::getFgColor()
{
  return ColorBar::instance()->getFgColor();
}

app::Color UISettingsImpl::getBgColor()
{
  return ColorBar::instance()->getBgColor();
}

tools::Tool* UISettingsImpl::getCurrentTool()
{
  if (!m_currentTool)
    m_currentTool = App::instance()->getToolBox()->getToolById("pencil");

  return m_currentTool;
}

app::ColorSwatches* UISettingsImpl::getColorSwatches()
{
  return m_colorSwatches;
}

void UISettingsImpl::setShowSpriteEditorScrollbars(bool state)
{
  m_showSpriteEditorScrollbars = state;

  notifyObservers<bool>(&GlobalSettingsObserver::onSetShowSpriteEditorScrollbars, state);
}

void UISettingsImpl::setGrabAlpha(bool state)
{
  m_grabAlpha = state;

  notifyObservers<bool>(&GlobalSettingsObserver::onSetGrabAlpha, state);
}

void UISettingsImpl::setFgColor(const app::Color& color)
{
  ColorBar::instance()->setFgColor(color);
}

void UISettingsImpl::setBgColor(const app::Color& color)
{
  ColorBar::instance()->setBgColor(color);
}

void UISettingsImpl::setCurrentTool(tools::Tool* tool)
{
  if (m_currentTool != tool) {
    // Fire signals (maybe the new selected tool has a different pen size)
    App::instance()->PenSizeBeforeChange();
    App::instance()->PenAngleBeforeChange();

    // Change the tool
    m_currentTool = tool;

    App::instance()->CurrentToolChange(); // Fire CurrentToolChange signal
    App::instance()->PenSizeAfterChange(); // Fire PenSizeAfterChange signal
    App::instance()->PenAngleAfterChange(); // Fire PenAngleAfterChange signal
  }
}

void UISettingsImpl::setColorSwatches(app::ColorSwatches* colorSwatches)
{
  m_colorSwatches = colorSwatches;
  notifyObservers<app::ColorSwatches*>(&GlobalSettingsObserver::onSetColorSwatches, colorSwatches);
}

IDocumentSettings* UISettingsImpl::getDocumentSettings(const Document* document)
{
  return m_globalDocumentSettings;
}

IColorSwatchesStore* UISettingsImpl::getColorSwatchesStore()
{
  return this;
}

void UISettingsImpl::addColorSwatches(app::ColorSwatches* colorSwatches)
{
  m_colorSwatchesStore.push_back(colorSwatches);
}

void UISettingsImpl::removeColorSwatches(app::ColorSwatches* colorSwatches)
{
  std::vector<app::ColorSwatches*>::iterator it =
    std::find(m_colorSwatchesStore.begin(),
              m_colorSwatchesStore.end(),
              colorSwatches);

  ASSERT(it != m_colorSwatchesStore.end());

  if (it != m_colorSwatchesStore.end())
    m_colorSwatchesStore.erase(it);
}

void UISettingsImpl::addObserver(GlobalSettingsObserver* observer) {
  base::Observable<GlobalSettingsObserver>::addObserver(observer);
}

void UISettingsImpl::removeObserver(GlobalSettingsObserver* observer) {
  base::Observable<GlobalSettingsObserver>::removeObserver(observer);
}

ISelectionSettings* UISettingsImpl::selection()
{
  return m_selectionSettings;
}

//////////////////////////////////////////////////////////////////////
// IDocumentSettings implementation

TiledMode UIDocumentSettingsImpl::getTiledMode()
{
  return m_tiledMode;
}

void UIDocumentSettingsImpl::setTiledMode(TiledMode mode)
{
  m_tiledMode = mode;
  notifyObservers<TiledMode>(&DocumentSettingsObserver::onSetTiledMode, mode);
}

bool UIDocumentSettingsImpl::getSnapToGrid()
{
  return m_snapToGrid;
}

bool UIDocumentSettingsImpl::getGridVisible()
{
  return m_gridVisible;
}

Rect UIDocumentSettingsImpl::getGridBounds()
{
  return m_gridBounds;
}

app::Color UIDocumentSettingsImpl::getGridColor()
{
  return m_gridColor;
}

void UIDocumentSettingsImpl::setSnapToGrid(bool state)
{
  m_snapToGrid = state;
  notifyObservers<bool>(&DocumentSettingsObserver::onSetSnapToGrid, state);
}

void UIDocumentSettingsImpl::setGridVisible(bool state)
{
  m_gridVisible = state;
  notifyObservers<bool>(&DocumentSettingsObserver::onSetGridVisible, state);
}

void UIDocumentSettingsImpl::setGridBounds(const Rect& rect)
{
  m_gridBounds = rect;
  notifyObservers<const Rect&>(&DocumentSettingsObserver::onSetGridBounds, rect);
}

void UIDocumentSettingsImpl::setGridColor(const app::Color& color)
{
  m_gridColor = color;
  notifyObservers<const app::Color&>(&DocumentSettingsObserver::onSetGridColor, color);
}

void UIDocumentSettingsImpl::snapToGrid(gfx::Point& point) const
{
  register int w = m_gridBounds.w;
  register int h = m_gridBounds.h;
  div_t d, dx, dy;

  dx = div(m_gridBounds.x, w);
  dy = div(m_gridBounds.y, h);

  d = div(point.x-dx.rem, w);
  point.x = dx.rem + d.quot*w + ((d.rem > w/2)? w: 0);

  d = div(point.y-dy.rem, h);
  point.y = dy.rem + d.quot*h + ((d.rem > h/2)? h: 0);
}

bool UIDocumentSettingsImpl::getPixelGridVisible()
{
  return m_pixelGridVisible;
}

app::Color UIDocumentSettingsImpl::getPixelGridColor()
{
  return m_pixelGridColor;
}

void UIDocumentSettingsImpl::setPixelGridVisible(bool state)
{
  m_pixelGridVisible = state;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setPixelGridColor(const app::Color& color)
{
  m_pixelGridColor = color;
  redrawDocumentViews();
}

bool UIDocumentSettingsImpl::getUseOnionskin()
{
  return m_use_onionskin;
}

int UIDocumentSettingsImpl::getOnionskinPrevFrames()
{
  return m_prev_frames_onionskin;
}

int UIDocumentSettingsImpl::getOnionskinNextFrames()
{
  return m_next_frames_onionskin;
}

int UIDocumentSettingsImpl::getOnionskinOpacityBase()
{
  return m_onionskin_opacity_base;
}

int UIDocumentSettingsImpl::getOnionskinOpacityStep()
{
  return m_onionskin_opacity_step;
}

void UIDocumentSettingsImpl::setUseOnionskin(bool state)
{
  m_use_onionskin = state;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setOnionskinPrevFrames(int frames)
{
  m_prev_frames_onionskin = frames;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setOnionskinNextFrames(int frames)
{
  m_next_frames_onionskin = frames;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setOnionskinOpacityBase(int base)
{
  m_onionskin_opacity_base = base;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setOnionskinOpacityStep(int step)
{
  m_onionskin_opacity_step = step;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::addObserver(DocumentSettingsObserver* observer)
{
  base::Observable<DocumentSettingsObserver>::addObserver(observer);
}

void UIDocumentSettingsImpl::removeObserver(DocumentSettingsObserver* observer)
{
  base::Observable<DocumentSettingsObserver>::removeObserver(observer);
}

//////////////////////////////////////////////////////////////////////
// Tools & pen settings

class UIPenSettingsImpl
  : public IPenSettings
  , public base::Observable<PenSettingsObserver> {
private:
  PenType m_type;
  int m_size;
  int m_angle;
  bool m_fireSignals;

public:
  UIPenSettingsImpl()
  {
    m_type = PEN_TYPE_FIRST;
    m_size = 1;
    m_angle = 0;
    m_fireSignals = true;
  }

  ~UIPenSettingsImpl()
  {
  }

  PenType getType() { return m_type; }
  int getSize() { return m_size; }
  int getAngle() { return m_angle; }

  void setType(PenType type)
  {
    m_type = MID(PEN_TYPE_FIRST, type, PEN_TYPE_LAST);
    notifyObservers<PenType>(&PenSettingsObserver::onSetPenType, m_type);
  }

  void setSize(int size)
  {
    // Trigger PenSizeBeforeChange signal
    if (m_fireSignals)
      App::instance()->PenSizeBeforeChange();

    // Change the size of the pencil
    m_size = MID(1, size, 32);

    // Trigger PenSizeAfterChange signal
    if (m_fireSignals)
      App::instance()->PenSizeAfterChange();
    notifyObservers<int>(&PenSettingsObserver::onSetPenSize, m_size);
  }

  void setAngle(int angle)
  {
    // Trigger PenAngleBeforeChange signal
    if (m_fireSignals)
      App::instance()->PenAngleBeforeChange();

    m_angle = MID(0, angle, 360);

    // Trigger PenAngleAfterChange signal
    if (m_fireSignals)
      App::instance()->PenAngleAfterChange();
  }

  void enableSignals(bool state)
  {
    m_fireSignals = state;
  }

  void addObserver(PenSettingsObserver* observer) OVERRIDE{
    base::Observable<PenSettingsObserver>::addObserver(observer);
  }

  void removeObserver(PenSettingsObserver* observer) OVERRIDE{
    base::Observable<PenSettingsObserver>::removeObserver(observer);
  }
};

class UIToolSettingsImpl
  : public IToolSettings
  , base::Observable<ToolSettingsObserver> {
  tools::Tool* m_tool;
  UIPenSettingsImpl m_pen;
  int m_opacity;
  int m_tolerance;
  bool m_filled;
  bool m_previewFilled;
  int m_spray_width;
  int m_spray_speed;
  InkType m_inkType;
  FreehandAlgorithm m_freehandAlgorithm;

public:

  UIToolSettingsImpl(tools::Tool* tool)
    : m_tool(tool)
  {
    std::string cfg_section(getCfgSection());

    m_opacity = get_config_int(cfg_section.c_str(), "Opacity", 255);
    m_opacity = MID(0, m_opacity, 255);
    m_tolerance = get_config_int(cfg_section.c_str(), "Tolerance", 0);
    m_tolerance = MID(0, m_tolerance, 255);
    m_filled = false;
    m_previewFilled = get_config_bool(cfg_section.c_str(), "PreviewFilled", false);
    m_spray_width = 16;
    m_spray_speed = 32;
    m_inkType = (InkType)get_config_int(cfg_section.c_str(), "InkType", (int)kDefaultInk);
    m_freehandAlgorithm = kDefaultFreehandAlgorithm;

    // Reset invalid configurations for inks.
    if (m_inkType != kDefaultInk && m_inkType != kPutAlphaInk)
      m_inkType = kDefaultInk;

    m_pen.enableSignals(false);
    m_pen.setType((PenType)get_config_int(cfg_section.c_str(), "PenType", (int)PEN_TYPE_CIRCLE));
    m_pen.setSize(get_config_int(cfg_section.c_str(), "PenSize", m_tool->getDefaultPenSize()));
    m_pen.setAngle(get_config_int(cfg_section.c_str(), "PenAngle", 0));
    m_pen.enableSignals(true);

    if (m_tool->getPointShape(0)->isSpray() ||
        m_tool->getPointShape(1)->isSpray()) {
      m_spray_width = get_config_int(cfg_section.c_str(), "SprayWidth", m_spray_width);
      m_spray_speed = get_config_int(cfg_section.c_str(), "SpraySpeed", m_spray_speed);
    }

    if (m_tool->getController(0)->isFreehand() ||
        m_tool->getController(1)->isFreehand()) {
      m_freehandAlgorithm = (FreehandAlgorithm)get_config_int(cfg_section.c_str(), "FreehandAlgorithm", (int)kDefaultFreehandAlgorithm);
      setFreehandAlgorithm(m_freehandAlgorithm);
    }
  }

  ~UIToolSettingsImpl()
  {
    std::string cfg_section(getCfgSection());

    set_config_int(cfg_section.c_str(), "Opacity", m_opacity);
    set_config_int(cfg_section.c_str(), "Tolerance", m_tolerance);
    set_config_int(cfg_section.c_str(), "PenType", m_pen.getType());
    set_config_int(cfg_section.c_str(), "PenSize", m_pen.getSize());
    set_config_int(cfg_section.c_str(), "PenAngle", m_pen.getAngle());
    set_config_int(cfg_section.c_str(), "PenAngle", m_pen.getAngle());
    set_config_int(cfg_section.c_str(), "InkType", m_inkType);
    set_config_bool(cfg_section.c_str(), "PreviewFilled", m_previewFilled);

    if (m_tool->getPointShape(0)->isSpray() ||
        m_tool->getPointShape(1)->isSpray()) {
      set_config_int(cfg_section.c_str(), "SprayWidth", m_spray_width);
      set_config_int(cfg_section.c_str(), "SpraySpeed", m_spray_speed);
    }

    if (m_tool->getController(0)->isFreehand() ||
        m_tool->getController(1)->isFreehand()) {
      set_config_int(cfg_section.c_str(), "FreehandAlgorithm", m_freehandAlgorithm);
    }
  }

  IPenSettings* getPen() { return &m_pen; }

  int getOpacity() OVERRIDE { return m_opacity; }
  int getTolerance() OVERRIDE { return m_tolerance; }
  bool getFilled() OVERRIDE { return m_filled; }
  bool getPreviewFilled() OVERRIDE { return m_previewFilled; }
  int getSprayWidth() OVERRIDE { return m_spray_width; }
  int getSpraySpeed() OVERRIDE { return m_spray_speed; }
  InkType getInkType() OVERRIDE { return m_inkType; }
  FreehandAlgorithm getFreehandAlgorithm() OVERRIDE { return m_freehandAlgorithm; }

  void setOpacity(int opacity) OVERRIDE { m_opacity = opacity; }
  void setTolerance(int tolerance) OVERRIDE { m_tolerance = tolerance; }
  void setFilled(bool state) OVERRIDE { m_filled = state; }
  void setPreviewFilled(bool state) OVERRIDE { m_previewFilled = state; }
  void setSprayWidth(int width) OVERRIDE { m_spray_width = width; }
  void setSpraySpeed(int speed) OVERRIDE { m_spray_speed = speed; }
  void setInkType(InkType inkType) OVERRIDE { m_inkType = inkType; }
  void setFreehandAlgorithm(FreehandAlgorithm algorithm) OVERRIDE {
    m_freehandAlgorithm = algorithm;

    tools::ToolBox* toolBox = App::instance()->getToolBox();
    for (int i=0; i<2; ++i) {
      if (algorithm == kPixelPerfectFreehandAlgorithm) {
        m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::AsPixelPerfect));
        m_tool->setTracePolicy(i, tools::TracePolicyLast);
      }
      else {
        m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::AsLines));
        m_tool->setTracePolicy(i, tools::TracePolicyAccumulate);
      }
    }
  }

  void addObserver(ToolSettingsObserver* observer) OVERRIDE {
    base::Observable<ToolSettingsObserver>::addObserver(observer);
  }

  void removeObserver(ToolSettingsObserver* observer) OVERRIDE{
    base::Observable<ToolSettingsObserver>::removeObserver(observer);
  }

private:
  std::string getCfgSection() const {
    return std::string("Tool:") + m_tool->getId();
  }
};

IToolSettings* UISettingsImpl::getToolSettings(tools::Tool* tool)
{
  ASSERT(tool != NULL);

  std::map<std::string, IToolSettings*>::iterator
    it = m_toolSettings.find(tool->getId());

  if (it != m_toolSettings.end()) {
    return it->second;
  }
  else {
    IToolSettings* tool_settings = new UIToolSettingsImpl(tool);
    m_toolSettings[tool->getId()] = tool_settings;
    return tool_settings;
  }
}

//////////////////////////////////////////////////////////////////////
// Selection Settings

namespace {

UISelectionSettingsImpl::UISelectionSettingsImpl() :
  m_selectionMode(kDefaultSelectionMode),
  m_moveTransparentColor(app::Color::fromMask()),
  m_rotationAlgorithm(kFastRotationAlgorithm)
{
  m_selectionMode = (SelectionMode)get_config_int("Tools", "SelectionMode", m_selectionMode);
  m_selectionMode = MID(
    kFirstSelectionMode,
    m_selectionMode,
    kLastSelectionMode);

  m_rotationAlgorithm = (RotationAlgorithm)get_config_int("Tools", "RotAlgorithm", m_rotationAlgorithm);
  m_rotationAlgorithm = MID(
    kFirstRotationAlgorithm,
    m_rotationAlgorithm,
    kLastRotationAlgorithm);
}

UISelectionSettingsImpl::~UISelectionSettingsImpl()
{
}

SelectionMode UISelectionSettingsImpl::getSelectionMode()
{
  return m_selectionMode;
}

app::Color UISelectionSettingsImpl::getMoveTransparentColor()
{
  return m_moveTransparentColor;
}

RotationAlgorithm UISelectionSettingsImpl::getRotationAlgorithm()
{
  return m_rotationAlgorithm;
}

void UISelectionSettingsImpl::setSelectionMode(SelectionMode mode)
{
  m_selectionMode = mode;
  notifyObservers(&SelectionSettingsObserver::onSetSelectionMode, mode);
}

void UISelectionSettingsImpl::setMoveTransparentColor(app::Color color)
{
  m_moveTransparentColor = color;
  notifyObservers(&SelectionSettingsObserver::onSetMoveTransparentColor, color);
}

void UISelectionSettingsImpl::setRotationAlgorithm(RotationAlgorithm algorithm)
{
  m_rotationAlgorithm = algorithm;
  set_config_int("Tools", "RotAlgorithm", m_rotationAlgorithm);
  notifyObservers(&SelectionSettingsObserver::onSetRotationAlgorithm, algorithm);
}

void UISelectionSettingsImpl::addObserver(SelectionSettingsObserver* observer)
{
  base::Observable<SelectionSettingsObserver>::addObserver(observer);
}

void UISelectionSettingsImpl::removeObserver(SelectionSettingsObserver* observer)
{
  base::Observable<SelectionSettingsObserver>::removeObserver(observer);
}

} // anonymous namespace
} // namespace app
