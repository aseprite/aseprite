/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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
#include "app/document.h"
#include "app/ini_file.h"
#include "app/resource_finder.h"
#include "app/settings/document_settings.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui_context.h"
#include "base/observable.h"
#include "doc/context.h"
#include "doc/documents_observer.h"
#include "ui/manager.h"
#include "ui/system.h"

#include <algorithm>
#include <string>

#define DEFAULT_ONIONSKIN_TYPE         Onionskin_Merge
#define DEFAULT_ONIONSKIN_OPACITY_BASE 68
#define DEFAULT_ONIONSKIN_OPACITY_STEP 28

namespace app {

using namespace gfx;
using namespace doc;
using namespace filters;

namespace {

class UIDocumentSettingsImpl : public IDocumentSettings,
                               public base::Observable<DocumentSettingsObserver> {
public:
  UIDocumentSettingsImpl(const doc::Document* doc)
    : m_doc(doc)
    , m_tiledMode(TILED_NONE)
    , m_use_onionskin(false)
    , m_prev_frames_onionskin(1)
    , m_next_frames_onionskin(1)
    , m_onionskin_opacity_base(DEFAULT_ONIONSKIN_OPACITY_BASE)
    , m_onionskin_opacity_step(DEFAULT_ONIONSKIN_OPACITY_STEP)
    , m_onionskinType(DEFAULT_ONIONSKIN_TYPE)
    , m_snapToGrid(false)
    , m_gridVisible(false)
    , m_gridBounds(0, 0, 16, 16)
    , m_gridColor(app::Color::fromRgb(0, 0, 255))
    , m_pixelGridVisible(false)
    , m_pixelGridColor(app::Color::fromRgb(200, 200, 200))
    , m_isLoop(false)
    , m_loopBegin(0)
    , m_loopEnd(1)
    , m_aniDir(AniDir_Normal)
  {
    bool specific_file = false;
    if (m_doc && static_cast<const app::Document*>(m_doc)->isAssociatedToFile()) {
      push_config_state();
      set_config_file(configFileName().c_str());
      specific_file = true;

      m_tiledMode = (TiledMode)get_config_int("Tools", "Tiled", (int)m_tiledMode);
      m_use_onionskin = get_config_bool("Onionskin", "Enabled", m_use_onionskin);
      m_prev_frames_onionskin = get_config_int("Onionskin", "PrevFrames", m_prev_frames_onionskin);
      m_next_frames_onionskin = get_config_int("Onionskin", "NextFrames", m_next_frames_onionskin);
      m_onionskin_opacity_base = get_config_int("Onionskin", "OpacityBase", m_onionskin_opacity_base);
      m_onionskin_opacity_step = get_config_int("Onionskin", "OpacityStep", m_onionskin_opacity_step);
      m_onionskinType = (OnionskinType)get_config_int("Onionskin", "Type", m_onionskinType);
      m_snapToGrid = get_config_bool("Grid", "SnapTo", m_snapToGrid);
      m_gridVisible = get_config_bool("Grid", "Visible", m_gridVisible);
      m_gridBounds = get_config_rect("Grid", "Bounds", m_gridBounds);
      m_pixelGridVisible = get_config_bool("PixelGrid", "Visible", m_pixelGridVisible);

      // Limit values
      m_tiledMode = (TiledMode)MID(0, (int)m_tiledMode, (int)TILED_BOTH);
      m_onionskinType = (OnionskinType)MID(0, (int)m_onionskinType, (int)Onionskin_Last);
    }

    m_gridColor = get_config_color("Grid", "Color", m_gridColor);
    m_pixelGridColor = get_config_color("PixelGrid", "Color", m_pixelGridColor);

    if (specific_file)
      pop_config_state();
  }

  ~UIDocumentSettingsImpl()
  {
    bool specific_file = false;
    if (m_doc) {
      // We don't save the document configuration if it isn't associated with a file.
      if (!static_cast<const app::Document*>(m_doc)->isAssociatedToFile())
        return;

      push_config_state();
      set_config_file(configFileName().c_str());
      specific_file = true;

      set_config_int("Tools", "Tiled", m_tiledMode);
      set_config_bool("Grid", "SnapTo", m_snapToGrid);
      set_config_bool("Grid", "Visible", m_gridVisible);
      set_config_rect("Grid", "Bounds", m_gridBounds);
      set_config_bool("PixelGrid", "Visible", m_pixelGridVisible);

      set_config_bool("Onionskin", "Enabled", m_use_onionskin);
      set_config_int("Onionskin", "PrevFrames", m_prev_frames_onionskin);
      set_config_int("Onionskin", "NextFrames", m_next_frames_onionskin);
      set_config_int("Onionskin", "OpacityBase", m_onionskin_opacity_base);
      set_config_int("Onionskin", "OpacityStep", m_onionskin_opacity_step);
      set_config_int("Onionskin", "Type", m_onionskinType);
    }

    saveSharedSettings();

    if (specific_file) {
      flush_config_file();
      pop_config_state();
    }
  }

  // Tiled mode

  virtual TiledMode getTiledMode() override;
  virtual void setTiledMode(TiledMode mode) override;

  // Grid settings

  virtual bool getSnapToGrid() override;
  virtual bool getGridVisible() override;
  virtual gfx::Rect getGridBounds() override;
  virtual app::Color getGridColor() override;

  virtual void setSnapToGrid(bool state) override;
  virtual void setGridVisible(bool state) override;
  virtual void setGridBounds(const gfx::Rect& rect) override;
  virtual void setGridColor(const app::Color& color) override;

  virtual void snapToGrid(gfx::Point& point) const override;

  // Pixel grid

  virtual bool getPixelGridVisible() override;
  virtual app::Color getPixelGridColor() override;

  virtual void setPixelGridVisible(bool state) override;
  virtual void setPixelGridColor(const app::Color& color) override;

  // Onionskin settings

  virtual bool getUseOnionskin() override;
  virtual int getOnionskinPrevFrames() override;
  virtual int getOnionskinNextFrames() override;
  virtual int getOnionskinOpacityBase() override;
  virtual int getOnionskinOpacityStep() override;
  virtual OnionskinType getOnionskinType() override;

  virtual void setUseOnionskin(bool state) override;
  virtual void setOnionskinPrevFrames(int frames) override;
  virtual void setOnionskinNextFrames(int frames) override;
  virtual void setOnionskinOpacityBase(int base) override;
  virtual void setOnionskinOpacityStep(int step) override;
  virtual void setOnionskinType(OnionskinType type) override;
  virtual void setDefaultOnionskinSettings() override;

  // Animation

  virtual bool getLoopAnimation() override;
  virtual void getLoopRange(doc::FrameNumber* begin, doc::FrameNumber* end) override;
  virtual AniDir getAnimationDirection() override;

  virtual void setLoopAnimation(bool state) override;
  virtual void setLoopRange(doc::FrameNumber begin, doc::FrameNumber end) override;
  virtual void setAnimationDirection(AniDir dir) override;

  virtual void addObserver(DocumentSettingsObserver* observer) override;
  virtual void removeObserver(DocumentSettingsObserver* observer) override;

private:
  void saveSharedSettings() {
    set_config_color("Grid", "Color", m_gridColor);
    set_config_color("PixelGrid", "Color", m_pixelGridColor);
  }

  std::string configFileName() {
    if (!m_doc)
      return "";

    if (m_cfgName.empty()) {
      ResourceFinder rf;
      std::string fn = m_doc->filename();
      for (size_t i=0; i<fn.size(); ++i) {
        if (fn[i] == ' ' || fn[i] == '/' || fn[i] == '\\' || fn[i] == ':' || fn[i] == '.') {
          fn[i] = '-';
        }
      }
      rf.includeUserDir(("files/" + fn + ".ini").c_str());
      m_cfgName = rf.getFirstOrCreateDefault();
    }
    return m_cfgName;
  }

  void redrawDocumentViews() {
    // TODO Redraw only document's views
    ui::Manager::getDefault()->invalidate();
  }

  const doc::Document* m_doc;
  std::string m_cfgName;
  TiledMode m_tiledMode;
  bool m_use_onionskin;
  int m_prev_frames_onionskin;
  int m_next_frames_onionskin;
  int m_onionskin_opacity_base;
  int m_onionskin_opacity_step;
  OnionskinType m_onionskinType;
  bool m_snapToGrid;
  bool m_gridVisible;
  gfx::Rect m_gridBounds;
  app::Color m_gridColor;
  bool m_pixelGridVisible;
  app::Color m_pixelGridColor;
  bool m_isLoop;
  doc::FrameNumber m_loopBegin;
  doc::FrameNumber m_loopEnd;
  AniDir m_aniDir;
};

class UISelectionSettingsImpl
    : public ISelectionSettings
    , public base::Observable<SelectionSettingsObserver> {
public:
  UISelectionSettingsImpl();
  virtual ~UISelectionSettingsImpl();

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
  , m_colorSwatches(NULL)
  , m_selectionSettings(new UISelectionSettingsImpl)
  , m_zoomWithScrollWheel(get_config_bool("Options", "ZoomWithMouseWheel", true))
  , m_showSpriteEditorScrollbars(get_config_bool("Options", "ShowScrollbars", true))
  , m_grabAlpha(get_config_bool("Options", "GrabAlpha", false))
  , m_rightClickMode(static_cast<RightClickMode>(get_config_int("Options", "RightClickMode",
        static_cast<int>(RightClickMode::Default))))
{
  m_colorSwatches = new app::ColorSwatches("Default");
  for (size_t i=0; i<16; ++i)
    m_colorSwatches->addColor(app::Color::fromIndex(i));

  addColorSwatches(m_colorSwatches);
}

UISettingsImpl::~UISettingsImpl()
{
  set_config_bool("Options", "ZoomWithMouseWheel", m_zoomWithScrollWheel);
  set_config_bool("Options", "ShowScrollbars", m_showSpriteEditorScrollbars);
  set_config_bool("Options", "GrabAlpha", m_grabAlpha);
  set_config_int("Options", "RightClickMode", static_cast<int>(m_rightClickMode));

  for (auto it : m_docSettings)
    delete it.second;

  for (auto it : m_toolSettings)
    delete it.second;

  for (auto it : m_colorSwatchesStore)
    delete it;
}

void UISettingsImpl::onRemoveDocument(doc::Document* doc)
{
  auto it = m_docSettings.find(doc->id());
  if (it != m_docSettings.end()) {
    delete it->second;
    m_docSettings.erase(it);
  }
}

//////////////////////////////////////////////////////////////////////
// General settings

size_t UISettingsImpl::undoSizeLimit() const
{
  return ((size_t)get_config_int("Options", "UndoSizeLimit", 8));
}

bool UISettingsImpl::undoGotoModified() const
{
  return get_config_bool("Options", "UndoGotoModified", true);
}

void UISettingsImpl::setUndoSizeLimit(size_t size)
{
  set_config_int("Options", "UndoSizeLimit", size);
}

void UISettingsImpl::setUndoGotoModified(bool state)
{
  set_config_bool("Options", "UndoGotoModified", state);
}

bool UISettingsImpl::getZoomWithScrollWheel()
{
  return m_zoomWithScrollWheel;
}

bool UISettingsImpl::getShowSpriteEditorScrollbars()
{
  return m_showSpriteEditorScrollbars;
}

RightClickMode UISettingsImpl::getRightClickMode()
{
  return m_rightClickMode;
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

void UISettingsImpl::setZoomWithScrollWheel(bool state)
{
  m_zoomWithScrollWheel = state;
}

void UISettingsImpl::setShowSpriteEditorScrollbars(bool state)
{
  m_showSpriteEditorScrollbars = state;

  notifyObservers<bool>(&GlobalSettingsObserver::onSetShowSpriteEditorScrollbars, state);
}

void UISettingsImpl::setRightClickMode(RightClickMode mode)
{
  m_rightClickMode = mode;
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
    // Fire signals (maybe the new selected tool has a different brush size)
    App::instance()->BrushSizeBeforeChange();
    App::instance()->BrushAngleBeforeChange();

    // Change the tool
    m_currentTool = tool;

    App::instance()->CurrentToolChange(); // Fire CurrentToolChange signal
    App::instance()->BrushSizeAfterChange(); // Fire BrushSizeAfterChange signal
    App::instance()->BrushAngleAfterChange(); // Fire BrushAngleAfterChange signal
  }
}

void UISettingsImpl::setColorSwatches(app::ColorSwatches* colorSwatches)
{
  m_colorSwatches = colorSwatches;
  notifyObservers<app::ColorSwatches*>(&GlobalSettingsObserver::onSetColorSwatches, colorSwatches);
}

IDocumentSettings* UISettingsImpl::getDocumentSettings(const doc::Document* document)
{
  doc::ObjectId id;

  if (!document)
    id = doc::NullId;
  else
    id = document->id();

  auto it = m_docSettings.find(id);
  if (it != m_docSettings.end())
    return it->second;

  return m_docSettings[id] = new UIDocumentSettingsImpl(document);
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

IExperimentalSettings* UISettingsImpl::experimental()
{
  return this;
}

bool UISettingsImpl::useNativeCursor() const
{
  return get_config_bool("Options", "NativeCursor", false);
}

void UISettingsImpl::setUseNativeCursor(bool state)
{
  set_config_bool("Options", "NativeCursor", state);
  ui::set_use_native_cursors(state);
}

bool UISettingsImpl::flashLayer() const
{
  return get_config_bool("Options", "FlashLayer", false);
}

void UISettingsImpl::setFlashLayer(bool state)
{
  set_config_bool("Options", "FlashLayer", state);
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

  saveSharedSettings();
}

void UIDocumentSettingsImpl::snapToGrid(gfx::Point& point) const
{
  int w = m_gridBounds.w;
  int h = m_gridBounds.h;
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

  saveSharedSettings();
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

IDocumentSettings::OnionskinType UIDocumentSettingsImpl::getOnionskinType()
{
  return m_onionskinType;
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

void UIDocumentSettingsImpl::setOnionskinType(OnionskinType type)
{
  m_onionskinType = type;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setDefaultOnionskinSettings()
{
  m_onionskinType = DEFAULT_ONIONSKIN_TYPE;
  m_onionskin_opacity_base = DEFAULT_ONIONSKIN_OPACITY_BASE;
  m_onionskin_opacity_step = DEFAULT_ONIONSKIN_OPACITY_STEP;
  redrawDocumentViews();
}

bool UIDocumentSettingsImpl::getLoopAnimation()
{
  return m_isLoop;
}

void UIDocumentSettingsImpl::getLoopRange(doc::FrameNumber* begin, doc::FrameNumber* end)
{
  *begin = m_loopBegin;
  *end = m_loopEnd;
}

IDocumentSettings::AniDir UIDocumentSettingsImpl::getAnimationDirection()
{
  return m_aniDir;
}

void UIDocumentSettingsImpl::setLoopAnimation(bool state)
{
  m_isLoop = state;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setLoopRange(doc::FrameNumber begin, doc::FrameNumber end)
{
  m_loopBegin = begin;
  m_loopEnd = end;
  redrawDocumentViews();
}

void UIDocumentSettingsImpl::setAnimationDirection(AniDir dir)
{
  m_aniDir = dir;
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
// Tools & brush settings

class UIBrushSettingsImpl
  : public IBrushSettings
  , public base::Observable<BrushSettingsObserver> {
private:
  BrushType m_type;
  int m_size;
  int m_angle;
  bool m_fireSignals;

public:
  UIBrushSettingsImpl()
  {
    m_type = kFirstBrushType;
    m_size = 1;
    m_angle = 0;
    m_fireSignals = true;
  }

  ~UIBrushSettingsImpl()
  {
  }

  BrushType getType() { return m_type; }
  int getSize() { return m_size; }
  int getAngle() { return m_angle; }

  void setType(BrushType type)
  {
    m_type = MID(kFirstBrushType, type, kLastBrushType);
    notifyObservers<BrushType>(&BrushSettingsObserver::onSetBrushType, m_type);
  }

  void setSize(int size)
  {
    // Trigger BrushSizeBeforeChange signal
    if (m_fireSignals)
      App::instance()->BrushSizeBeforeChange();

    // Change the size of the brushcil
    m_size = MID(1, size, 32);

    // Trigger BrushSizeAfterChange signal
    if (m_fireSignals)
      App::instance()->BrushSizeAfterChange();
    notifyObservers<int>(&BrushSettingsObserver::onSetBrushSize, m_size);
  }

  void setAngle(int angle)
  {
    // Trigger BrushAngleBeforeChange signal
    if (m_fireSignals)
      App::instance()->BrushAngleBeforeChange();

    m_angle = MID(0, angle, 360);

    // Trigger BrushAngleAfterChange signal
    if (m_fireSignals)
      App::instance()->BrushAngleAfterChange();
  }

  void enableSignals(bool state)
  {
    m_fireSignals = state;
  }

  void addObserver(BrushSettingsObserver* observer) override{
    base::Observable<BrushSettingsObserver>::addObserver(observer);
  }

  void removeObserver(BrushSettingsObserver* observer) override{
    base::Observable<BrushSettingsObserver>::removeObserver(observer);
  }
};

class UIToolSettingsImpl
  : public IToolSettings
  , base::Observable<ToolSettingsObserver> {
  tools::Tool* m_tool;
  UIBrushSettingsImpl m_brush;
  int m_opacity;
  int m_tolerance;
  bool m_contiguous;
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
    m_contiguous = get_config_bool(cfg_section.c_str(), "Contiguous", true);
    m_filled = false;
    m_previewFilled = get_config_bool(cfg_section.c_str(), "PreviewFilled", false);
    m_spray_width = 16;
    m_spray_speed = 32;
    m_inkType = (InkType)get_config_int(cfg_section.c_str(), "InkType", (int)kDefaultInk);
    m_freehandAlgorithm = kDefaultFreehandAlgorithm;

    // Reset invalid configurations for inks.
    if (m_inkType != kDefaultInk &&
        m_inkType != kSetAlphaInk &&
        m_inkType != kLockAlphaInk)
      m_inkType = kDefaultInk;

    m_brush.enableSignals(false);
    m_brush.setType((BrushType)get_config_int(cfg_section.c_str(), "BrushType", (int)kCircleBrushType));
    m_brush.setSize(get_config_int(cfg_section.c_str(), "BrushSize", m_tool->getDefaultBrushSize()));
    m_brush.setAngle(get_config_int(cfg_section.c_str(), "BrushAngle", 0));
    m_brush.enableSignals(true);

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
    set_config_bool(cfg_section.c_str(), "Contiguous", m_contiguous);
    set_config_int(cfg_section.c_str(), "BrushType", m_brush.getType());
    set_config_int(cfg_section.c_str(), "BrushSize", m_brush.getSize());
    set_config_int(cfg_section.c_str(), "BrushAngle", m_brush.getAngle());
    set_config_int(cfg_section.c_str(), "BrushAngle", m_brush.getAngle());
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

  IBrushSettings* getBrush() { return &m_brush; }

  int getOpacity() override { return m_opacity; }
  int getTolerance() override { return m_tolerance; }
  bool getContiguous() override { return m_contiguous; }
  bool getFilled() override { return m_filled; }
  bool getPreviewFilled() override { return m_previewFilled; }
  int getSprayWidth() override { return m_spray_width; }
  int getSpraySpeed() override { return m_spray_speed; }
  InkType getInkType() override { return m_inkType; }
  FreehandAlgorithm getFreehandAlgorithm() override { return m_freehandAlgorithm; }

  void setOpacity(int opacity) override { m_opacity = opacity; }
  void setTolerance(int tolerance) override { m_tolerance = tolerance; }
  void setContiguous(bool state) override { m_contiguous = state; }
  void setFilled(bool state) override { m_filled = state; }
  void setPreviewFilled(bool state) override { m_previewFilled = state; }
  void setSprayWidth(int width) override { m_spray_width = width; }
  void setSpraySpeed(int speed) override { m_spray_speed = speed; }
  void setInkType(InkType inkType) override { m_inkType = inkType; }
  void setFreehandAlgorithm(FreehandAlgorithm algorithm) override {
    m_freehandAlgorithm = algorithm;

    tools::ToolBox* toolBox = App::instance()->getToolBox();
    for (int i=0; i<2; ++i) {
      switch (algorithm) {
        case kDefaultFreehandAlgorithm:
          m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::AsLines));
          m_tool->setTracePolicy(i, tools::TracePolicyAccumulate);
          break;
        case kPixelPerfectFreehandAlgorithm:
          m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::AsPixelPerfect));
          m_tool->setTracePolicy(i, tools::TracePolicyLast);
          break;
        case kDotsFreehandAlgorithm:
          m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::None));
          m_tool->setTracePolicy(i, tools::TracePolicyAccumulate);
          break;
      }
    }
  }

  void addObserver(ToolSettingsObserver* observer) override {
    base::Observable<ToolSettingsObserver>::addObserver(observer);
  }

  void removeObserver(ToolSettingsObserver* observer) override{
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
  set_config_int("Tools", "SelectionMode", (int)m_selectionMode);
  set_config_int("Tools", "RotAlgorithm", (int)m_rotationAlgorithm);
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
