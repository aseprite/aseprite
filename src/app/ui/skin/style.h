/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef APP_UI_SKIN_STYLE_H_INCLUDED
#define APP_UI_SKIN_STYLE_H_INCLUDED

#include "app/ui/skin/skin_part.h"
#include "base/compiler_specific.h"
#include "base/disable_copying.h"
#include "css/compound_style.h"
#include "css/state.h"
#include "css/stateful_style.h"
#include "gfx/fwd.h"
#include "ui/color.h"

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
      BackgroundRule() : m_color(ui::ColorNone) { }

      void setColor(ui::Color color) { m_color = color; }
      void setPart(const SkinPartPtr& part) { m_part = part; }

    protected:
      void onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text) OVERRIDE;

    private:
      ui::Color m_color;
      SkinPartPtr m_part;
    };

    class TextRule : public Rule {
    public:
      explicit TextRule() : m_align(0),
                            m_color(ui::ColorNone) { }

      void setAlign(int align) { m_align = align; }
      void setColor(ui::Color color) { m_color = color; }

    protected:
      void onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text) OVERRIDE;

    private:
      int m_align;
      ui::Color m_color;
    };

    class IconRule : public Rule {
    public:
      explicit IconRule() : m_align(0) { }

      void setAlign(int align) { m_align = align; }
      void setPart(const SkinPartPtr& part) { m_part = part; }

    protected:
      void onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text) OVERRIDE;

    private:
      int m_align;
      SkinPartPtr m_part;
    };

    class Rules {
    public:
      Rules(const css::Query& query);
      ~Rules();

      void paint(ui::Graphics* g,
        const gfx::Rect& bounds,
        const char* text);

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

      Style(css::Sheet& sheet, const std::string& id);
      ~Style();

      void paint(ui::Graphics* g,
        const gfx::Rect& bounds,
        const char* text,
        const State& state);

      const std::string& id() const { return m_id; }

    private:
      typedef std::map<State, Rules*> RulesMap;

      std::string m_id;
      css::CompoundStyle m_compoundStyle;
      RulesMap m_rules;

      static css::State m_hoverState;
      static css::State m_activeState;
      static css::State m_clickedState;
    };

  } // namespace skin
} // namespace app

#endif
