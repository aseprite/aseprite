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

#include "app/ui/skin/style_sheet.h"

#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "base/exception.h"
#include "css/sheet.h"

#include "tinyxml.h"

namespace app {
namespace skin {

css::Rule StyleSheet::m_backgroundColorRule("background-color");
css::Rule StyleSheet::m_backgroundPartRule("background-part");
css::Rule StyleSheet::m_iconAlignRule("icon-align");
css::Rule StyleSheet::m_iconPartRule("icon-part");
css::Rule StyleSheet::m_textAlignRule("text-align");
css::Rule StyleSheet::m_textColorRule("text-color");

StyleSheet::StyleSheet()
{
  m_sheet = new css::Sheet;
  m_sheet->addRule(&m_backgroundColorRule);
  m_sheet->addRule(&m_backgroundPartRule);
  m_sheet->addRule(&m_iconAlignRule);
  m_sheet->addRule(&m_iconPartRule);
  m_sheet->addRule(&m_textAlignRule);
  m_sheet->addRule(&m_textColorRule);
}

StyleSheet::~StyleSheet()
{
  destroyAllStyles();
  delete m_sheet;
}

void StyleSheet::destroyAllStyles()
{
  for (StyleMap::iterator it = m_styles.begin(), end = m_styles.end();
       it != end; ++it)
    delete it->second;
}

Style* StyleSheet::getStyle(const std::string& id)
{
  Style* style = NULL;

  StyleMap::iterator it = m_styles.find(id);
  if (it != m_styles.end())
    style = it->second;
  else {
    style = new Style(*m_sheet, id);
    m_styles[id] = style;
  }

  return style;
}

// static
SkinPartPtr StyleSheet::convertPart(const css::Value& value)
{
  SkinPartPtr part;
  if (value.type() == css::Value::String) {
    const std::string& part_id = value.string();
    part = get_part_by_id(part_id);
    if (part == NULL)
      throw base::Exception("Unknown part '%s'\n", part_id.c_str());
  }
  return part;
}

// static
ui::Color StyleSheet::convertColor(const css::Value& value)
{
  ui::Color color;
  if (value.type() == css::Value::String) {
    const std::string& color_id = value.string();
    color = get_color_by_id(color_id);
    if (color == ui::ColorNone)
      throw base::Exception("Unknown color '%s'\n", color_id.c_str());
  }
  return color;
}

} // namespace skin
} // namespace app
