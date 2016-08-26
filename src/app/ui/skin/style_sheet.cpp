// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
css::Rule StyleSheet::m_backgroundRepeatRule("background-repeat");
css::Rule StyleSheet::m_iconAlignRule("icon-align");
css::Rule StyleSheet::m_iconPartRule("icon-part");
css::Rule StyleSheet::m_iconXRule("icon-x");
css::Rule StyleSheet::m_iconYRule("icon-y");
css::Rule StyleSheet::m_textAlignRule("text-align");
css::Rule StyleSheet::m_textColorRule("text-color");
css::Rule StyleSheet::m_paddingLeftRule("padding-left");
css::Rule StyleSheet::m_paddingTopRule("padding-top");
css::Rule StyleSheet::m_paddingRightRule("padding-right");
css::Rule StyleSheet::m_paddingBottomRule("padding-bottom");

StyleSheet::StyleSheet()
{
  m_sheet = new css::Sheet;
  m_sheet->addRule(&m_backgroundColorRule);
  m_sheet->addRule(&m_backgroundPartRule);
  m_sheet->addRule(&m_backgroundRepeatRule);
  m_sheet->addRule(&m_iconAlignRule);
  m_sheet->addRule(&m_iconPartRule);
  m_sheet->addRule(&m_iconXRule);
  m_sheet->addRule(&m_iconYRule);
  m_sheet->addRule(&m_textAlignRule);
  m_sheet->addRule(&m_textColorRule);
  m_sheet->addRule(&m_paddingLeftRule);
  m_sheet->addRule(&m_paddingTopRule);
  m_sheet->addRule(&m_paddingRightRule);
  m_sheet->addRule(&m_paddingBottomRule);
}

StyleSheet::~StyleSheet()
{
  // Destroy skin::Styles
  for (StyleMap::iterator it = m_styles.begin(), end = m_styles.end();
       it != end; ++it)
    delete it->second;

  // Destroy css::Styles
  for (std::vector<css::Style*>::iterator it = m_cssStyles.begin(), end = m_cssStyles.end();
       it != end; ++it)
    delete *it;

  delete m_sheet;
}

void StyleSheet::addCssStyle(css::Style* style)
{
  m_sheet->addStyle(style);
  m_cssStyles.push_back(style);
}

const css::Style* StyleSheet::getCssStyle(const std::string& id)
{
  return m_sheet->getStyle(id);
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
    if (!part)
      throw base::Exception("Unknown part '%s'\n", part_id.c_str());
  }
  return part;
}

// static
gfx::Color StyleSheet::convertColor(const css::Value& value)
{
  gfx::Color color = gfx::ColorNone;
  if (value.type() == css::Value::String) {
    const std::string& color_id = value.string();
    color = get_color_by_id(color_id);
    if (color == gfx::ColorNone)
      throw base::Exception("Unknown color '%s'\n", color_id.c_str());
  }
  return color;
}

// static
BackgroundRepeat StyleSheet::convertRepeat(const css::Value& value)
{
  BackgroundRepeat repeat = BackgroundRepeat::NO_REPEAT;
  if (value.type() == css::Value::String) {
    const std::string& id = value.string();
    if (id == "repeat")
      repeat = BackgroundRepeat::REPEAT;
    else if (id == "repeat-x")
      repeat = BackgroundRepeat::REPEAT_X;
    else if (id == "repeat-y")
      repeat = BackgroundRepeat::REPEAT_Y;
    else if (id == "no_repeat")
      repeat = BackgroundRepeat::NO_REPEAT;
    else
      throw base::Exception("Unknown repeat value '%s'\n", id.c_str());
  }
  return repeat;
}

} // namespace skin
} // namespace app
