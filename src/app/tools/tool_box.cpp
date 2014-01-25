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

#include "app/tools/tool_box.h"

#include "app/gui_xml.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/intertwine.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool_group.h"
#include "app/tools/tool_loop.h"
#include "base/exception.h"
#include "raster/algo.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/pen.h"

#include <algorithm>
#include <allegro/file.h>
#include <allegro/fixed.h>
#include <allegro/fmaths.h>

#include "app/tools/controllers.h"
#include "app/tools/inks.h"
#include "app/tools/intertwiners.h"
#include "app/tools/point_shapes.h"

namespace app {
namespace tools {

using namespace gfx;
  
const char* WellKnownTools::RectangularMarquee = "rectangular_marquee";

const char* WellKnownInks::Selection = "selection";
const char* WellKnownInks::Paint = "paint";
const char* WellKnownInks::PaintFg = "paint_fg";
const char* WellKnownInks::PaintBg = "paint_bg";
const char* WellKnownInks::PaintOpaque = "paint_opaque";
const char* WellKnownInks::PaintPutAlpha = "paint_put_alpha";
const char* WellKnownInks::Shading = "shading";
const char* WellKnownInks::Eraser = "eraser";
const char* WellKnownInks::ReplaceFgWithBg = "replace_fg_with_bg";
const char* WellKnownInks::ReplaceBgWithFg = "replace_bg_with_fg";
const char* WellKnownInks::PickFg = "pick_fg";
const char* WellKnownInks::PickBg = "pick_bg";
const char* WellKnownInks::Scroll = "scroll";
const char* WellKnownInks::Move = "move";
const char* WellKnownInks::Blur = "blur";
const char* WellKnownInks::Jumble = "jumble";

const char* WellKnownIntertwiners::None = "none";
const char* WellKnownIntertwiners::AsLines = "as_lines";
const char* WellKnownIntertwiners::AsRectangles = "as_rectangles";
const char* WellKnownIntertwiners::AsEllipses = "as_ellipses";
const char* WellKnownIntertwiners::AsBezier = "as_bezier";
const char* WellKnownIntertwiners::AsPixelPerfect = "as_pixel_perfect";

ToolBox::ToolBox()
{
  PRINTF("Toolbox module: installing\n");

  m_inks[WellKnownInks::Selection]       = new SelectionInk();
  m_inks[WellKnownInks::Paint]           = new PaintInk(PaintInk::Merge);
  m_inks[WellKnownInks::PaintFg]         = new PaintInk(PaintInk::WithFg);
  m_inks[WellKnownInks::PaintBg]         = new PaintInk(PaintInk::WithBg);
  m_inks[WellKnownInks::PaintOpaque]     = new PaintInk(PaintInk::Opaque);
  m_inks[WellKnownInks::PaintPutAlpha]   = new PaintInk(PaintInk::PutAlpha);
  m_inks[WellKnownInks::Shading]         = new ShadingInk();
  m_inks[WellKnownInks::Eraser]          = new EraserInk(EraserInk::Eraser);
  m_inks[WellKnownInks::ReplaceFgWithBg] = new EraserInk(EraserInk::ReplaceFgWithBg);
  m_inks[WellKnownInks::ReplaceBgWithFg] = new EraserInk(EraserInk::ReplaceBgWithFg);
  m_inks[WellKnownInks::PickFg]          = new PickInk(PickInk::Fg);
  m_inks[WellKnownInks::PickBg]          = new PickInk(PickInk::Bg);
  m_inks[WellKnownInks::Scroll]          = new ScrollInk();
  m_inks[WellKnownInks::Move]            = new MoveInk();
  m_inks[WellKnownInks::Blur]            = new BlurInk();
  m_inks[WellKnownInks::Jumble]          = new JumbleInk();

  m_controllers["freehand"]              = new FreehandController();
  m_controllers["point_by_point"]        = new PointByPointController();
  m_controllers["one_point"]             = new OnePointController();
  m_controllers["two_points"]            = new TwoPointsController();
  m_controllers["four_points"]           = new FourPointsController();

  m_pointshapers["none"]                 = new NonePointShape();
  m_pointshapers["pixel"]                = new PixelPointShape();
  m_pointshapers["pen"]                  = new PenPointShape();
  m_pointshapers["floodfill"]            = new FloodFillPointShape();
  m_pointshapers["spray"]                = new SprayPointShape();

  m_intertwiners[WellKnownIntertwiners::None] = new IntertwineNone();
  m_intertwiners[WellKnownIntertwiners::AsLines] = new IntertwineAsLines();
  m_intertwiners[WellKnownIntertwiners::AsRectangles] = new IntertwineAsRectangles();
  m_intertwiners[WellKnownIntertwiners::AsEllipses] = new IntertwineAsEllipses();
  m_intertwiners[WellKnownIntertwiners::AsBezier] = new IntertwineAsBezier();
  m_intertwiners[WellKnownIntertwiners::AsPixelPerfect] = new IntertwineAsPixelPerfect();

  loadTools();

  PRINTF("Toolbox module: installed\n");
}

struct deleter {
  template<typename T>
  void operator()(T* p) { delete p; }

  template<typename A, typename B>
  void operator()(std::pair<A,B>& p) { delete p.second; }
};

ToolBox::~ToolBox()
{
  PRINTF("Toolbox module: uninstalling\n");

  std::for_each(m_tools.begin(), m_tools.end(), deleter());
  std::for_each(m_groups.begin(), m_groups.end(), deleter());
  std::for_each(m_intertwiners.begin(), m_intertwiners.end(), deleter());
  std::for_each(m_pointshapers.begin(), m_pointshapers.end(), deleter());
  std::for_each(m_controllers.begin(), m_controllers.end(), deleter());
  std::for_each(m_inks.begin(), m_inks.end(), deleter());

  PRINTF("Toolbox module: uninstalled\n");
}

Tool* ToolBox::getToolById(const std::string& id)
{
  for (ToolIterator it = begin(), end = this->end(); it != end; ++it) {
    Tool* tool = *it;
    if (tool->getId() == id)
      return tool;
  }
  // PRINTF("Error get_tool_by_name() with '%s'\n", name.c_str());
  // ASSERT(false);
  return NULL;
}

Ink* ToolBox::getInkById(const std::string& id)
{
  return m_inks[id];
}

Intertwine* ToolBox::getIntertwinerById(const std::string& id)
{
  return m_intertwiners[id];
}

void ToolBox::loadTools()
{
  PRINTF("Loading Aseprite tools\n");

  XmlDocumentRef doc(GuiXml::instance()->doc());
  TiXmlHandle handle(doc);

  // For each group
  TiXmlElement* xmlGroup = handle.FirstChild("gui").FirstChild("tools").FirstChild("group").ToElement();
  while (xmlGroup) {
    const char* group_id = xmlGroup->Attribute("id");
    const char* group_text = xmlGroup->Attribute("text");

    PRINTF(" - New group '%s'\n", group_id);

    if (!group_id || !group_text)
      throw base::Exception("The configuration file has a <group> without 'id' or 'text' attributes.");

    ToolGroup* tool_group = new ToolGroup(group_id, group_text);

    // For each tool
    TiXmlNode* xmlToolNode = xmlGroup->FirstChild("tool");
    TiXmlElement* xmlTool = xmlToolNode ? xmlToolNode->ToElement(): NULL;
    while (xmlTool) {
      const char* tool_id = xmlTool->Attribute("id");
      const char* tool_text = xmlTool->Attribute("text");
      const char* tool_tips = xmlTool->FirstChild("tooltip") ? ((TiXmlElement*)xmlTool->FirstChild("tooltip"))->GetText(): "";
      const char* default_pen_size = xmlTool->Attribute("default_pen_size");

      Tool* tool = new Tool(tool_group, tool_id, tool_text, tool_tips,
                            default_pen_size ? strtol(default_pen_size, NULL, 10): 1);

      PRINTF(" - New tool '%s' in group '%s' found\n", tool_id, group_id);

      loadToolProperties(xmlTool, tool, 0, "left");
      loadToolProperties(xmlTool, tool, 1, "right");

      m_tools.push_back(tool);

      xmlTool = xmlTool->NextSiblingElement();
    }

    m_groups.push_back(tool_group);
    xmlGroup = xmlGroup->NextSiblingElement();
  }
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
  TracePolicy tracepolicy_value = TracePolicyLast;
  if (tracepolicy) {
    if (strcmp(tracepolicy, "accumulative") == 0)
      tracepolicy_value = TracePolicyAccumulate;
    else if (strcmp(tracepolicy, "last") == 0)
      tracepolicy_value = TracePolicyLast;
    else if (strcmp(tracepolicy, "overlap") == 0)
      tracepolicy_value = TracePolicyOverlap;
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
