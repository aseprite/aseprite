// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TOOL_H_INCLUDED
#define APP_TOOLS_TOOL_H_INCLUDED
#pragma once

#include <string>

#include "app/tools/fill.h"
#include "app/tools/trace_policy.h"

namespace app {
  namespace tools {

    class Controller;
    class Ink;
    class Intertwine;
    class PointShape;
    class ToolGroup;

    // A drawing tool
    class Tool {
    public:

      Tool(ToolGroup* group, const std::string& id)
        : m_group(group)
        , m_id(id)
        , m_default_brush_size(1)
      { }

      virtual ~Tool()
      { }

      const ToolGroup* getGroup() const { return m_group; }
      const std::string& getId() const { return m_id; }
      const std::string& getText() const { return m_text; }
      const std::string& getTips() const { return m_tips; }
      int getDefaultBrushSize() const { return m_default_brush_size; }

      void setText(const std::string& text) { m_text = text; }
      void setTips(const std::string& tips) { m_tips = tips; }
      void setDefaultBrushSize(const int default_brush_size) {
        m_default_brush_size = default_brush_size;
      }

      Fill getFill(int button) { return m_button[button].m_fill; }
      Ink* getInk(int button) { return m_button[button].m_ink; }
      Controller* getController(int button) { return m_button[button].m_controller; }
      PointShape* getPointShape(int button) { return m_button[button].m_point_shape; }
      Intertwine* getIntertwine(int button) { return m_button[button].m_intertwine; }
      TracePolicy getTracePolicy(int button) { return m_button[button].m_trace_policy; }

      void setFill(int button, Fill fill) { m_button[button].m_fill = fill; }
      void setInk(int button, Ink* ink) { m_button[button].m_ink = ink; }
      void setController(int button, Controller* controller) { m_button[button].m_controller = controller; }
      void setPointShape(int button, PointShape* point_shape) { m_button[button].m_point_shape = point_shape; }
      void setIntertwine(int button, Intertwine* intertwine) { m_button[button].m_intertwine = intertwine; }
      void setTracePolicy(int button, TracePolicy trace_policy) { m_button[button].m_trace_policy = trace_policy; }

    private:
      ToolGroup* m_group;
      std::string m_id;
      std::string m_text;
      std::string m_tips;
      int m_default_brush_size;

      struct {
        Fill m_fill;
        Ink* m_ink;
        Controller* m_controller;
        PointShape* m_point_shape;
        Intertwine* m_intertwine;
        TracePolicy m_trace_policy;
      } m_button[2]; // Two buttons: [0] left and [1] right

    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_TOOL_H_INCLUDED
