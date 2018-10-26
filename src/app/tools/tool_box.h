// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TOOL_BOX_H_INCLUDED
#define APP_TOOLS_TOOL_BOX_H_INCLUDED
#pragma once

#include <list>
#include <map>
#include <string>

#include "app/i18n/xml_translator.h"
#include "app/tools/tool.h"

class TiXmlElement;

namespace app {
  namespace tools {

    namespace WellKnownTools {
      extern const char* RectangularMarquee;
      extern const char* Lasso;
      extern const char* Pencil;
      extern const char* Eraser;
      extern const char* Eyedropper;
      extern const char* Hand;
      extern const char* Move;
    };

    namespace WellKnownInks {
      extern const char* Selection;
      extern const char* Paint;
      extern const char* PaintFg;
      extern const char* PaintBg;
      extern const char* PaintCopy;
      extern const char* PaintLockAlpha;
      extern const char* Shading;
      extern const char* Gradient;
      extern const char* Eraser;
      extern const char* ReplaceFgWithBg;
      extern const char* ReplaceBgWithFg;
      extern const char* PickFg;
      extern const char* PickBg;
      extern const char* Zoom;
      extern const char* Scroll;
      extern const char* Move;
      extern const char* SelectLayerAndMove;
      extern const char* Slice;
      extern const char* MoveSlice;
      extern const char* Blur;
      extern const char* Jumble;
    };

    namespace WellKnownControllers {
      extern const char* Freehand;
      extern const char* PointByPoint;
      extern const char* OnePoints;
      extern const char* TwoPoints;
      extern const char* FourPoints;
      extern const char* LineFreehand;
    };

    namespace WellKnownIntertwiners {
      extern const char* None;
      extern const char* FirstPoint;
      extern const char* AsLines;
      extern const char* AsRectangles;
      extern const char* AsEllipses;
      extern const char* AsBezier;
      extern const char* AsPixelPerfect;
    };

    namespace WellKnownPointShapes {
      extern const char* None;
      extern const char* Pixel;
      extern const char* Brush;
      extern const char* FloodFill;
      extern const char* Spray;
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
      Controller* getControllerById(const std::string& id);
      Intertwine* getIntertwinerById(const std::string& id);
      PointShape* getPointShapeById(const std::string& id);
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
      XmlTranslator m_xmlTranslator;
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_TOOL_BOX_H_INCLUDED
