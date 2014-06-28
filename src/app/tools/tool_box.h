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

#ifndef APP_TOOLS_TOOL_BOX_H_INCLUDED
#define APP_TOOLS_TOOL_BOX_H_INCLUDED
#pragma once

#include <list>
#include <map>
#include <string>

#include "app/tools/tool.h"

class TiXmlElement;

namespace app {
  namespace tools {

    namespace WellKnownTools {
      extern const char* RectangularMarquee;
    };

    namespace WellKnownInks {
      extern const char* Selection;
      extern const char* Paint;
      extern const char* PaintFg;
      extern const char* PaintBg;
      extern const char* PaintOpaque;
      extern const char* PaintSetAlpha;
      extern const char* PaintLockAlpha;
      extern const char* Shading;
      extern const char* Eraser;
      extern const char* ReplaceFgWithBg;
      extern const char* ReplaceBgWithFg;
      extern const char* PickFg;
      extern const char* PickBg;
      extern const char* Zoom;
      extern const char* Scroll;
      extern const char* Move;
      extern const char* Slice;
      extern const char* Blur;
      extern const char* Jumble;
    };

    namespace WellKnownIntertwiners {
      extern const char* None;
      extern const char* AsLines;
      extern const char* AsRectangles;
      extern const char* AsEllipses;
      extern const char* AsBezier;
      extern const char* AsPixelPerfect;
    };

    typedef std::list<Tool*> ToolList;
    typedef ToolList::iterator ToolIterator;
    typedef ToolList::const_iterator ToolConstIterator;

    typedef std::list<ToolGroup*> ToolGroupList;

    // Loads and maintains the group of tools specified in the gui.xml file
    class ToolBox {
    public:
      ToolBox();
      ~ToolBox();

      ToolGroupList::iterator begin_group() { return m_groups.begin(); }
      ToolGroupList::iterator end_group() { return m_groups.end(); }

      ToolIterator begin() { return m_tools.begin(); }
      ToolIterator end() { return m_tools.end(); }
      ToolConstIterator begin() const { return m_tools.begin(); }
      ToolConstIterator end() const { return m_tools.end(); }

      Tool* getToolById(const std::string& id);
      Ink* getInkById(const std::string& id);
      Intertwine* getIntertwinerById(const std::string& id);
      int getGroupsCount() const { return m_groups.size(); }

    private:
      void loadTools();
      void loadToolProperties(TiXmlElement* xmlTool, Tool* tool, int button, const std::string& suffix);

      std::map<std::string, Ink*> m_inks;
      std::map<std::string, Controller*> m_controllers;
      std::map<std::string, PointShape*> m_pointshapers;
      std::map<std::string, Intertwine*> m_intertwiners;

      ToolGroupList m_groups;
      ToolList m_tools;
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_TOOL_BOX_H_INCLUDED
