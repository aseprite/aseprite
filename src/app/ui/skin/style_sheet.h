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

#ifndef APP_UI_SKIN_STYLE_SHEET_H_INCLUDED
#define APP_UI_SKIN_STYLE_SHEET_H_INCLUDED

#include "app/ui/skin/skin_part.h"
#include "css/rule.h"
#include "css/state.h"
#include "ui/color.h"

#include <map>
#include <string>

class TiXmlElement;

namespace css {
  class Sheet;
  class Value;
}

namespace app {
  namespace skin {

    class Style;

    class StyleSheet {
    public:
      StyleSheet();
      ~StyleSheet();

      static css::Rule& backgroundColorRule() { return m_backgroundColorRule; }
      static css::Rule& backgroundPartRule() { return m_backgroundPartRule; }
      static css::Rule& iconAlignRule() { return m_iconAlignRule; }
      static css::Rule& iconPartRule() { return m_iconPartRule; }
      static css::Rule& textAlignRule() { return m_textAlignRule; }
      static css::Rule& textColorRule() { return m_textColorRule; }

      css::Sheet& sheet() { return *m_sheet; }

      Style* getStyle(const std::string& id);

      static SkinPartPtr convertPart(const css::Value& value);
      static ui::Color convertColor(const css::Value& value);

    private:
      typedef std::map<std::string, Style*> StyleMap;

      void destroyAllStyles();

      static css::Rule m_backgroundColorRule;
      static css::Rule m_backgroundPartRule;
      static css::Rule m_iconAlignRule;
      static css::Rule m_iconPartRule;
      static css::Rule m_textAlignRule;
      static css::Rule m_textColorRule;
      css::Sheet* m_sheet;
      StyleMap m_styles;
    };

  } // namespace skin
} // namespace app

#endif
