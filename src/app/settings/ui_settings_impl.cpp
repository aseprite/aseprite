// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/settings/ui_settings_impl.h"

#include "app/app.h"
#include "app/color_swatches.h"
#include "app/document.h"
#include "app/ini_file.h"
#include "app/resource_finder.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui_context.h"
#include "base/observable.h"
#include "doc/brush.h"
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
  , m_autoSelectLayer(get_config_bool("Options", "AutoSelectLayer", false))
{
  m_colorSwatches = new app::ColorSwatches("Default");
  for (size_t i=0; i<16; ++i)
    m_colorSwatches->addColor(app::Color::fromIndex(i));

  addColorSwatches(m_colorSwatches);
}

UISettingsImpl::~UISettingsImpl()
{
  set_config_bool("Options", "AutoSelectLayer", m_autoSelectLayer);

  for (auto it : m_toolSettings)
    delete it.second;

  for (auto it : m_colorSwatchesStore)
    delete it;
}

//////////////////////////////////////////////////////////////////////
// General settings

bool UISettingsImpl::getAutoSelectLayer()
{
  return m_autoSelectLayer;
}

app::Color UISettingsImpl::getFgColor()
{
  ColorBar* colorbar = ColorBar::instance();
  return colorbar ? colorbar->getFgColor(): app::Color::fromMask();
}

app::Color UISettingsImpl::getBgColor()
{
  ColorBar* colorbar = ColorBar::instance();
  return colorbar ? colorbar->getBgColor(): app::Color::fromMask();
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

void UISettingsImpl::setAutoSelectLayer(bool state)
{
  m_autoSelectLayer = state;
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

ISelectionSettings* UISettingsImpl::selection()
{
  return m_selectionSettings;
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
    m_size = MID(doc::Brush::kMinBrushSize, size, doc::Brush::kMaxBrushSize);

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
      if (m_tool->getTracePolicy(i) != tools::TracePolicy::Accumulate &&
          m_tool->getTracePolicy(i) != tools::TracePolicy::AccumulateUpdateLast) {
        continue;
      }

      switch (algorithm) {
        case kDefaultFreehandAlgorithm:
          m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::AsLines));
          m_tool->setTracePolicy(i, tools::TracePolicy::Accumulate);
          break;
        case kPixelPerfectFreehandAlgorithm:
          m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::AsPixelPerfect));
          m_tool->setTracePolicy(i, tools::TracePolicy::AccumulateUpdateLast);
          break;
        case kDotsFreehandAlgorithm:
          m_tool->setIntertwine(i, toolBox->getIntertwinerById(tools::WellKnownIntertwiners::None));
          m_tool->setTracePolicy(i, tools::TracePolicy::Accumulate);
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
