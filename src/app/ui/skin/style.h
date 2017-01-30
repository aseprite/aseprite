// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_STYLE_H_INCLUDED
#define APP_UI_SKIN_STYLE_H_INCLUDED
#pragma once

#include "app/ui/skin/background_repeat.h"
#include "app/ui/skin/skin_part.h"
#include "base/disable_copying.h"
#include "css/compound_style.h"
#include "css/state.h"
#include "css/stateful_style.h"
#include "gfx/border.h"
#include "gfx/color.h"
#include "gfx/fwd.h"

#include <map>
#include <string>
#include <vector>

namespace ui {
  class Graphics;
}

namespace app {
  namespace skin {

    class Rule {
    public:
      Rule() { }
      virtual ~Rule();

      void paint(ui::Graphics* g,
                 const gfx::Rect& bounds,
                 const char* text);

    protected:
      virtual void onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text) = 0;
    };

    class BackgroundRule : public Rule {
    public:
      BackgroundRule() : m_color(gfx::ColorNone)
                       , m_repeat(BackgroundRepeat::NO_REPEAT) { }

      void setColor(gfx::Color color) { m_color = color; }
      void setPart(const SkinPartPtr& part) { m_part = part; }
      void setRepeat(BackgroundRepeat repeat) { m_repeat = repeat; }

    protected:
      void onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text) override;

    private:
      gfx::Color m_color;
      SkinPartPtr m_part;
      BackgroundRepeat m_repeat;
    };

    class TextRule : public Rule {
    public:
      explicit TextRule() : m_align(0),
                            m_color(gfx::ColorNone) { }

      void setAlign(int align) { m_align = align; }
      void setColor(gfx::Color color) { m_color = color; }
      void setPadding(const gfx::Border& padding) { m_padding = padding; }

      int align() const { return m_align; }
      gfx::Border padding() const { return m_padding; }

    protected:
      void onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text) override;

    private:
      int m_align;
      gfx::Color m_color;
      gfx::Border m_padding;
    };

    class IconRule : public Rule {
    public:
      explicit IconRule() : m_align(0),
                            m_color(gfx::ColorNone) { }

      void setAlign(int align) { m_align = align; }
      void setPart(const SkinPartPtr& part) { m_part = part; }
      void setX(int x) { m_x = x; }
      void setY(int y) { m_y = y; }
      void setColor(gfx::Color color) { m_color = color; }

      SkinPartPtr getPart() { return m_part; }

    protected:
      void onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text) override;

    private:
      int m_align;
      SkinPartPtr m_part;
      int m_x, m_y;
      gfx::Color m_color;
    };

    class Rules {
    public:
      Rules(const css::Query& query);
      ~Rules();

      void paint(ui::Graphics* g,
        const gfx::Rect& bounds,
        const char* text);

      gfx::Size sizeHint(const char* text, int maxWidth);

    private:
      BackgroundRule* m_background;
      TextRule* m_text;
      IconRule* m_icon;

      DISABLE_COPYING(Rules);
    };

    class Style {
    public:
      typedef css::States State;

      static const css::State& hover() { return m_hoverState; }
      static const css::State& active() { return m_activeState; }
      static const css::State& clicked() { return m_clickedState; }
      static const css::State& disabled() { return m_disabledState; }

      Style(css::Sheet& sheet, const std::string& id);
      ~Style();

      void paint(ui::Graphics* g,
        const gfx::Rect& bounds,
        const char* text,
        const State& state);

      gfx::Size sizeHint(
        const char* text,
        const State& state,
        int maxWidth = 0);

      const std::string& id() const { return m_id; }

    private:
      typedef std::map<State, Rules*> RulesMap;

      Rules* getRulesFromState(const State& state);

      std::string m_id;
      css::CompoundStyle m_compoundStyle;
      RulesMap m_rules;

      static css::State m_hoverState;
      static css::State m_activeState;
      static css::State m_clickedState;
      static css::State m_disabledState;
    };

  } // namespace skin
} // namespace app

#endif
