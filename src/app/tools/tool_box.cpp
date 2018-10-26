// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/tool_box.h"

#include "app/gui_xml.h"
#include "app/i18n/strings.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/intertwine.h"
#include "app/tools/point_shape.h"
#include "app/tools/stroke.h"
#include "app/tools/tool_group.h"
#include "app/tools/tool_loop.h"
#include "base/bind.h"
#include "base/exception.h"
#include "doc/algo.h"
#include "doc/algorithm/floodfill.h"
#include "doc/algorithm/polygon.h"
#include "doc/brush.h"
#include "doc/compressed_image.h"
#include "doc/image_impl.h"
#include "doc/mask.h"
#include "fixmath/fixmath.h"

#include <algorithm>
#include <cstdlib>

#include "app/tools/controllers.h"
#include "app/tools/inks.h"
#include "app/tools/intertwiners.h"
#include "app/tools/point_shapes.h"

namespace app {
namespace tools {

using namespace gfx;

const char* WellKnownTools::RectangularMarquee = "rectangular_marquee";
const char* WellKnownTools::Lasso = "lasso";
const char* WellKnownTools::Pencil = "pencil";
const char* WellKnownTools::Eraser = "eraser";
const char* WellKnownTools::Eyedropper = "eyedropper";
const char* WellKnownTools::Hand = "hand";
const char* WellKnownTools::Move = "move";

const char* WellKnownInks::Selection = "selection";
const char* WellKnownInks::Paint = "paint";
const char* WellKnownInks::PaintFg = "paint_fg";
const char* WellKnownInks::PaintBg = "paint_bg";
const char* WellKnownInks::PaintCopy = "paint_copy";
const char* WellKnownInks::PaintLockAlpha = "paint_lock_alpha";
const char* WellKnownInks::Shading = "shading";
const char* WellKnownInks::Gradient = "gradient";
const char* WellKnownInks::Eraser = "eraser";
const char* WellKnownInks::ReplaceFgWithBg = "replace_fg_with_bg";
const char* WellKnownInks::ReplaceBgWithFg = "replace_bg_with_fg";
const char* WellKnownInks::PickFg = "pick_fg";
const char* WellKnownInks::PickBg = "pick_bg";
const char* WellKnownInks::Zoom = "zoom";
const char* WellKnownInks::Scroll = "scroll";
const char* WellKnownInks::Move = "move";
const char* WellKnownInks::SelectLayerAndMove = "select_layer_and_move";
const char* WellKnownInks::Slice = "slice";
const char* WellKnownInks::MoveSlice = "move_slice";
const char* WellKnownInks::Blur = "blur";
const char* WellKnownInks::Jumble = "jumble";

const char* WellKnownControllers::Freehand = "freehand";
const char* WellKnownControllers::PointByPoint = "point_by_point";
const char* WellKnownControllers::OnePoints = "one_point";
const char* WellKnownControllers::TwoPoints = "two_points";
const char* WellKnownControllers::FourPoints = "four_points";
const char* WellKnownControllers::LineFreehand = "line_freehand";

const char* WellKnownIntertwiners::None = "none";
const char* WellKnownIntertwiners::FirstPoint = "first_point";
const char* WellKnownIntertwiners::AsLines = "as_lines";
const char* WellKnownIntertwiners::AsRectangles = "as_rectangles";
const char* WellKnownIntertwiners::AsEllipses = "as_ellipses";
const char* WellKnownIntertwiners::AsBezier = "as_bezier";
const char* WellKnownIntertwiners::AsPixelPerfect = "as_pixel_perfect";

const char* WellKnownPointShapes::None = "none";
const char* WellKnownPointShapes::Pixel = "pixel";
const char* WellKnownPointShapes::Brush = "brush";
const char* WellKnownPointShapes::FloodFill = "floodfill";
const char* WellKnownPointShapes::Spray = "spray";

namespace {

struct deleter {
  template<typename T>
  void operator()(T* p) { delete p; }

  template<typename A, typename B>
  void operator()(std::pair<A,B>& p) { delete p.second; }
};

} // anonymous namespace

ToolBox::ToolBox()
{
  m_xmlTranslator.setStringIdPrefix("tools");

  m_inks[WellKnownInks::Selection]       = new SelectionInk();
  m_inks[WellKnownInks::Paint]           = new PaintInk(PaintInk::Simple);
  m_inks[WellKnownInks::PaintFg]         = new PaintInk(PaintInk::WithFg);
  m_inks[WellKnownInks::PaintBg]         = new PaintInk(PaintInk::WithBg);
  m_inks[WellKnownInks::PaintCopy]       = new PaintInk(PaintInk::Copy);
  m_inks[WellKnownInks::PaintLockAlpha]  = new PaintInk(PaintInk::LockAlpha);
  m_inks[WellKnownInks::Gradient]        = new GradientInk();
  m_inks[WellKnownInks::Shading]         = new ShadingInk();
  m_inks[WellKnownInks::Eraser]          = new EraserInk(EraserInk::Eraser);
  m_inks[WellKnownInks::ReplaceFgWithBg] = new EraserInk(EraserInk::ReplaceFgWithBg);
  m_inks[WellKnownInks::ReplaceBgWithFg] = new EraserInk(EraserInk::ReplaceBgWithFg);
  m_inks[WellKnownInks::PickFg]          = new PickInk(PickInk::Fg);
  m_inks[WellKnownInks::PickBg]          = new PickInk(PickInk::Bg);
  m_inks[WellKnownInks::Zoom]            = new ZoomInk();
  m_inks[WellKnownInks::Scroll]          = new ScrollInk();
  m_inks[WellKnownInks::Move]            = new MoveInk(false);
  m_inks[WellKnownInks::SelectLayerAndMove] = new MoveInk(true);
  m_inks[WellKnownInks::Slice]           = new SliceInk();
  m_inks[WellKnownInks::Blur]            = new BlurInk();
  m_inks[WellKnownInks::Jumble]          = new JumbleInk();

  m_controllers[WellKnownControllers::Freehand] = new FreehandController();
  m_controllers[WellKnownControllers::PointByPoint] = new PointByPointController();
  m_controllers[WellKnownControllers::OnePoints] = new OnePointController();
  m_controllers[WellKnownControllers::TwoPoints] = new TwoPointsController();
  m_controllers[WellKnownControllers::FourPoints] = new FourPointsController();
  m_controllers[WellKnownControllers::LineFreehand] = new LineFreehandController();

  m_pointshapers[WellKnownPointShapes::None] = new NonePointShape();
  m_pointshapers[WellKnownPointShapes::Pixel] = new PixelPointShape();
  m_pointshapers[WellKnownPointShapes::Brush] = new BrushPointShape();
  m_pointshapers[WellKnownPointShapes::FloodFill] = new FloodFillPointShape();
  m_pointshapers[WellKnownPointShapes::Spray] = new SprayPointShape();

  m_intertwiners[WellKnownIntertwiners::None] = new IntertwineNone();
  m_intertwiners[WellKnownIntertwiners::FirstPoint] = new IntertwineFirstPoint();
  m_intertwiners[WellKnownIntertwiners::AsLines] = new IntertwineAsLines();
  m_intertwiners[WellKnownIntertwiners::AsRectangles] = new IntertwineAsRectangles();
  m_intertwiners[WellKnownIntertwiners::AsEllipses] = new IntertwineAsEllipses();
  m_intertwiners[WellKnownIntertwiners::AsBezier] = new IntertwineAsBezier();
  m_intertwiners[WellKnownIntertwiners::AsPixelPerfect] = new IntertwineAsPixelPerfect();

  loadTools();

  // When the language is change, we reload the toolbox stirngs/tooltips.
  Strings::instance()->LanguageChange.connect(
    [this]{ loadTools(); });
}

ToolBox::~ToolBox()
{
  std::for_each(m_tools.begin(), m_tools.end(), deleter());
  std::for_each(m_groups.begin(), m_groups.end(), deleter());
  std::for_each(m_intertwiners.begin(), m_intertwiners.end(), deleter());
  std::for_each(m_pointshapers.begin(), m_pointshapers.end(), deleter());
  std::for_each(m_controllers.begin(), m_controllers.end(), deleter());
  std::for_each(m_inks.begin(), m_inks.end(), deleter());
}

Tool* ToolBox::getToolById(const std::string& id)
{
  for (ToolIterator it = begin(), end = this->end(); it != end; ++it) {
    Tool* tool = *it;
    if (tool->getId() == id)
      return tool;
  }
  return nullptr;
}

Ink* ToolBox::getInkById(const std::string& id)
{
  return m_inks[id];
}

Controller* ToolBox::getControllerById(const std::string& id)
{
  return m_controllers[id];
}

Intertwine* ToolBox::getIntertwinerById(const std::string& id)
{
  return m_intertwiners[id];
}

PointShape* ToolBox::getPointShapeById(const std::string& id)
{
  return m_pointshapers[id];
}

void ToolBox::loadTools()
{
  LOG("TOOL: Loading tools...\n");

  XmlDocumentRef doc(GuiXml::instance()->doc());
  TiXmlHandle handle(doc.get());

  // For each group
  TiXmlElement* xmlGroup = handle.FirstChild("gui").FirstChild("tools").FirstChild("group").ToElement();
  while (xmlGroup) {
    const char* groupId = xmlGroup->Attribute("id");
    if (!groupId)
      throw base::Exception("The configuration file has a <group> without 'id' or 'text' attributes.");

    LOG(VERBOSE) << "TOOL: Group " << groupId << "\n";

    // Find an existent ToolGroup (this is useful in case we are
    // reloading tool text/tooltips).
    ToolGroup* toolGroup = nullptr;
    for (auto g : m_groups) {
      if (g->id() == groupId) {
        toolGroup = g;
        break;
      }
    }
    if (toolGroup == nullptr) {
      toolGroup = new ToolGroup(groupId);
      m_groups.push_back(toolGroup);
    }

    // For each tool
    TiXmlNode* xmlToolNode = xmlGroup->FirstChild("tool");
    TiXmlElement* xmlTool = xmlToolNode ? xmlToolNode->ToElement(): NULL;
    while (xmlTool) {
      const char* toolId = xmlTool->Attribute("id");
      std::string toolText = m_xmlTranslator(xmlTool, "text");
      std::string toolTips = m_xmlTranslator(xmlTool, "tooltip");
      const char* defaultBrushSize = xmlTool->Attribute("default_brush_size");

      Tool* tool = nullptr;
      for (auto t : m_tools) {
        if (t->getId() == toolId) {
          tool = t;
          break;
        }
      }
      if (tool == nullptr) {
        tool = new Tool(toolGroup, toolId);
        m_tools.push_back(tool);
      }

      tool->setText(toolText);
      tool->setTips(toolTips);
      tool->setDefaultBrushSize(
        defaultBrushSize ? std::strtol(defaultBrushSize, nullptr, 10): 1);

      LOG(VERBOSE) << "TOOL: Tool " << toolId << " in group " << groupId << " found\n";

      loadToolProperties(xmlTool, tool, 0, "left");
      loadToolProperties(xmlTool, tool, 1, "right");

      xmlTool = xmlTool->NextSiblingElement();
    }

    xmlGroup = xmlGroup->NextSiblingElement();
  }

  LOG("TOOL: Done. %d tools, %d groups.\n", m_tools.size(), m_groups.size());
}

void ToolBox::loadToolProperties(TiXmlElement* xmlTool, Tool* tool, int button, const std::string& suffix)
{
  const char* tool_id = tool->getId().c_str();
  const char* fill = xmlTool->Attribute(("fill_"+suffix).c_str());
  const char* ink = xmlTool->Attribute(("ink_"+suffix).c_str());
  const char* controller = xmlTool->Attribute(("controller_"+suffix).c_str());
  const char* pointshape = xmlTool->Attribute(("pointshape_"+suffix).c_str());
  const char* intertwine = xmlTool->Attribute(("intertwine_"+suffix).c_str());
  const char* tracepolicy = xmlTool->Attribute(("tracepolicy_"+suffix).c_str());

  if (!fill) fill = xmlTool->Attribute("fill");
  if (!ink) ink = xmlTool->Attribute("ink");
  if (!controller) controller = xmlTool->Attribute("controller");
  if (!pointshape) pointshape = xmlTool->Attribute("pointshape");
  if (!intertwine) intertwine = xmlTool->Attribute("intertwine");
  if (!tracepolicy) tracepolicy = xmlTool->Attribute("tracepolicy");

  // Fill
  Fill fill_value = FillNone;
  if (fill) {
    if (strcmp(fill, "none") == 0)
      fill_value = FillNone;
    else if (strcmp(fill, "always") == 0)
      fill_value = FillAlways;
    else if (strcmp(fill, "optional") == 0)
      fill_value = FillOptional;
    else
      throw base::Exception("Invalid fill '%s' specified in '%s' tool.\n", fill, tool_id);
  }

  // Find the ink
  std::map<std::string, Ink*>::iterator it_ink
    = m_inks.find(ink ? ink: "");
  if (it_ink == m_inks.end())
    throw base::Exception("Invalid ink '%s' specified in '%s' tool.\n", ink, tool_id);

  // Find the controller
  std::map<std::string, Controller*>::iterator it_controller
    = m_controllers.find(controller ? controller: "none");
  if (it_controller == m_controllers.end())
    throw base::Exception("Invalid controller '%s' specified in '%s' tool.\n", controller, tool_id);

  // Find the point_shape
  std::map<std::string, PointShape*>::iterator it_pointshaper
    = m_pointshapers.find(pointshape ? pointshape: "none");
  if (it_pointshaper == m_pointshapers.end())
    throw base::Exception("Invalid point-shape '%s' specified in '%s' tool.\n", pointshape, tool_id);

  // Find the intertwiner
  std::map<std::string, Intertwine*>::iterator it_intertwiner
    = m_intertwiners.find(intertwine ? intertwine: "none");
  if (it_intertwiner == m_intertwiners.end())
    throw base::Exception("Invalid intertwiner '%s' specified in '%s' tool.\n", intertwine, tool_id);

  // Trace policy
  TracePolicy tracepolicy_value = TracePolicy::Last;
  if (tracepolicy) {
    if (strcmp(tracepolicy, "accumulate") == 0)
      tracepolicy_value = TracePolicy::Accumulate;
    else if (strcmp(tracepolicy, "last") == 0)
      tracepolicy_value = TracePolicy::Last;
    else if (strcmp(tracepolicy, "overlap") == 0)
      tracepolicy_value = TracePolicy::Overlap;
    else
      throw base::Exception("Invalid trace-policy '%s' specified in '%s' tool.\n", tracepolicy, tool_id);
  }

  // Setup the tool properties
  tool->setFill(button, fill_value);
  tool->setInk(button, it_ink->second);
  tool->setController(button, it_controller->second);
  tool->setPointShape(button, it_pointshaper->second);
  tool->setIntertwine(button, it_intertwiner->second);
  tool->setTracePolicy(button, tracepolicy_value);
}

} // namespace tools
} // namespace app
