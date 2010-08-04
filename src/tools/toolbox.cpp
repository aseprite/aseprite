/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <algorithm>
#include <allegro/file.h>
#include <allegro/fixed.h>
#include <allegro/fmaths.h>

#include "ase_exception.h"
#include "raster/algo.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "resource_finder.h"
#include "tools/toolbox.h"

#include "tinyxml.h"

#include "tools/controllers.h"
#include "tools/inks.h"
#include "tools/intertwiners.h"
#include "tools/point_shapes.h"

//////////////////////////////////////////////////////////////////////

ToolBox::ToolBox()
{
  PRINTF("Toolbox module: installing\n");

  m_inks["selection"]			 = new SelectionInk();
  m_inks["paint"]			 = new PaintInk(PaintInk::Normal);
  m_inks["paint_fg"]			 = new PaintInk(PaintInk::WithFg);
  m_inks["paint_bg"]			 = new PaintInk(PaintInk::WithBg);
  m_inks["eraser"]			 = new EraserInk(EraserInk::Eraser);
  m_inks["replace_fg_with_bg"]		 = new EraserInk(EraserInk::ReplaceFgWithBg);
  m_inks["replace_bg_with_fg"]		 = new EraserInk(EraserInk::ReplaceBgWithFg);
  m_inks["pick_fg"]			 = new PickInk(PickInk::Fg);
  m_inks["pick_bg"]			 = new PickInk(PickInk::Bg);
  m_inks["scroll"]			 = new ScrollInk();
  m_inks["move"]			 = new MoveInk();
  m_inks["blur"]			 = new BlurInk();
  m_inks["jumble"]			 = new JumbleInk();

  m_controllers["freehand"]		 = new FreehandController();
  m_controllers["point_by_point"]	 = new PointByPointController();
  m_controllers["one_point"]		 = new OnePointController();
  m_controllers["two_points"]		 = new TwoPointsController();
  m_controllers["four_points"]		 = new FourPointsController();

  m_pointshapers["none"]		 = new NonePointShape();
  m_pointshapers["pixel"]		 = new PixelPointShape();
  m_pointshapers["pen"]			 = new PenPointShape();
  m_pointshapers["floodfill"]		 = new FloodFillPointShape();
  m_pointshapers["spray"]		 = new SprayPointShape();

  m_intertwiners["none"]		 = new IntertwineNone();
  m_intertwiners["as_lines"]		 = new IntertwineAsLines();
  m_intertwiners["as_rectangles"]	 = new IntertwineAsRectangles();
  m_intertwiners["as_ellipses"]		 = new IntertwineAsEllipses();
  m_intertwiners["as_bezier"]		 = new IntertwineAsBezier();

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
  for (ToolIterator it = begin(); it != end(); ++it) {
    Tool* tool = *it;
    if (tool->getId() == id)
      return tool;
  }
  // PRINTF("Error get_tool_by_name() with '%s'\n", name.c_str());
  // ASSERT(false);
  return NULL;
}

void ToolBox::loadTools()
{
  PRINTF("Loading ASE tools\n");

  ResourceFinder rf;
  rf.findInDataDir("gui.xml");

  while (const char* path = rf.next()) {
    PRINTF("Trying to load tools from \"%s\"...\n", path);

    if (!exists(path))
      continue;

    PRINTF(" - \"%s\" found\n", path);

    TiXmlDocument doc;
    if (!doc.LoadFile(path))
      throw ase_exception(&doc);

    // For each group
    TiXmlHandle handle(&doc);
    TiXmlElement* xmlGroup = handle.FirstChild("gui").FirstChild("tools").FirstChild("group").ToElement();
    while (xmlGroup) {
      const char* group_id = xmlGroup->Attribute("id");
      const char* group_text = xmlGroup->Attribute("text");

      PRINTF(" - New group '%s'\n", group_id);

      if (!group_id || !group_text)
	throw ase_exception("The configuration file has a <group> without 'id' or 'text' attributes.");

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
  ToolFill fill_value = TOOL_FILL_NONE;
  if (fill) {
    if (strcmp(fill, "none") == 0)
      fill_value = TOOL_FILL_NONE;
    else if (strcmp(fill, "always") == 0)
      fill_value = TOOL_FILL_ALWAYS;
    else if (strcmp(fill, "optional") == 0)
      fill_value = TOOL_FILL_OPTIONAL;
    else
      throw ase_exception("Invalid fill '%s' specified in '%s' tool.\n", fill, tool_id);
  }

  // Find the ink
  std::map<std::string, ToolInk*>::iterator it_ink
    = m_inks.find(ink ? ink: "");
  if (it_ink == m_inks.end())
    throw ase_exception("Invalid ink '%s' specified in '%s' tool.\n", ink, tool_id);

  // Find the controller
  std::map<std::string, ToolController*>::iterator it_controller
    = m_controllers.find(controller ? controller: "none");
  if (it_controller == m_controllers.end())
    throw ase_exception("Invalid controller '%s' specified in '%s' tool.\n", controller, tool_id);

  // Find the point_shape
  std::map<std::string, ToolPointShape*>::iterator it_pointshaper
    = m_pointshapers.find(pointshape ? pointshape: "none");
  if (it_pointshaper == m_pointshapers.end())
    throw ase_exception("Invalid point-shape '%s' specified in '%s' tool.\n", pointshape, tool_id);

  // Find the intertwiner
  std::map<std::string, ToolIntertwine*>::iterator it_intertwiner
    = m_intertwiners.find(intertwine ? intertwine: "none");
  if (it_intertwiner == m_intertwiners.end())
    throw ase_exception("Invalid intertwiner '%s' specified in '%s' tool.\n", intertwine, tool_id);

  // Trace policy
  ToolTracePolicy tracepolicy_value = TOOL_TRACE_POLICY_LAST;
  if (tracepolicy) {
    if (strcmp(tracepolicy, "accumulative") == 0)
      tracepolicy_value = TOOL_TRACE_POLICY_ACCUMULATE;
    else if (strcmp(tracepolicy, "last") == 0)
      tracepolicy_value = TOOL_TRACE_POLICY_LAST;
    else
      throw ase_exception("Invalid trace-policy '%s' specified in '%s' tool.\n", tracepolicy, tool_id);
  }

  // Setup the tool properties
  tool->setFill(button, fill_value);
  tool->setInk(button, it_ink->second);
  tool->setController(button, it_controller->second);
  tool->setPointShape(button, it_pointshaper->second);
  tool->setIntertwine(button, it_intertwiner->second);
  tool->setTracePolicy(button, tracepolicy_value);
}
