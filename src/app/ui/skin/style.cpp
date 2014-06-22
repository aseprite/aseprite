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

#include "app/ui/skin/style.h"

#include "app/ui/skin/skin_theme.h"
#include "css/sheet.h"
#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/theme.h"

namespace app {
namespace skin {

css::State Style::m_hoverState("hover");
css::State Style::m_activeState("active");
css::State Style::m_clickedState("clicked");

Rule::~Rule()
{
}

void Rule::paint(ui::Graphics* g,
                 const gfx::Rect& bounds,
                 const char* text)
{
  onPaint(g, bounds, text);
}

void BackgroundRule::onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text)
{
  SkinTheme* theme = static_cast<SkinTheme*>(ui::CurrentTheme::get());

  if (m_part != NULL && m_part->size() > 0) {
    if (m_part->size() == 1) {
      if (!ui::is_transparent(m_color))
        g->fillRect(m_color, bounds);

      g->drawRgbaSurface(m_part->getBitmap(0), bounds.x, bounds.y);
    }
    else if (m_part->size() == 8) {
      theme->draw_bounds_nw(g, bounds, m_part, m_color);
    }
  }
  else if (!ui::is_transparent(m_color)) {
    g->fillRect(m_color, bounds);
  }
}

void TextRule::onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text)
{
  SkinTheme* theme = static_cast<SkinTheme*>(ui::CurrentTheme::get());

  if (text) {
    g->drawAlignedUIString(text,
      (ui::is_transparent(m_color) ?
        theme->getColor(ThemeColor::Text):
        m_color),
      ui::ColorNone,
      gfx::Rect(bounds).shrink(m_padding), m_align);
  }
}

void IconRule::onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text)
{
  she::Surface* bmp = m_part->getBitmap(0);
  int x, y;

  if (m_align & JI_RIGHT)
    x = bounds.x2() - bmp->width();
  else if (m_align & JI_CENTER)
    x = bounds.x + bounds.w/2 - bmp->width()/2;
  else
    x = bounds.x;

  if (m_align & JI_BOTTOM)
    y = bounds.y2() - bmp->height();
  else if (m_align & JI_MIDDLE)
    y = bounds.y + bounds.h/2 - bmp->height()/2;
  else
    y = bounds.y;

  g->drawRgbaSurface(bmp, x, y);
}

Rules::Rules(const css::Query& query) :
  m_background(NULL),
  m_text(NULL),
  m_icon(NULL)
{
  css::Value backgroundColor = query[StyleSheet::backgroundColorRule()];
  css::Value backgroundPart = query[StyleSheet::backgroundPartRule()];
  css::Value iconAlign = query[StyleSheet::iconAlignRule()];
  css::Value iconPart = query[StyleSheet::iconPartRule()];
  css::Value textAlign = query[StyleSheet::textAlignRule()];
  css::Value textColor = query[StyleSheet::textColorRule()];
  css::Value paddingLeft = query[StyleSheet::paddingLeftRule()];
  css::Value paddingTop = query[StyleSheet::paddingTopRule()];
  css::Value paddingRight = query[StyleSheet::paddingRightRule()];
  css::Value paddingBottom = query[StyleSheet::paddingBottomRule()];
  css::Value none;

  if (backgroundColor != none
    || backgroundPart != none) {
    m_background = new BackgroundRule();
    m_background->setColor(StyleSheet::convertColor(backgroundColor));
    m_background->setPart(StyleSheet::convertPart(backgroundPart));
  }

  if (iconAlign != none
    || iconPart != none) {
    m_icon = new IconRule();
    m_icon->setAlign((int)iconAlign.number());
    m_icon->setPart(StyleSheet::convertPart(iconPart));
  }

  if (textAlign != none
    || textColor != none
    || paddingLeft != none
    || paddingTop != none
    || paddingRight != none
    || paddingBottom != none) {
    m_text = new TextRule();
    m_text->setAlign((int)textAlign.number());
    m_text->setColor(StyleSheet::convertColor(textColor));
    m_text->setPadding(gfx::Border(
        paddingLeft.number(), paddingTop.number(),
        paddingRight.number(), paddingBottom.number())*ui::jguiscale());
  }
}

Rules::~Rules()
{
  delete m_background;
  delete m_text;
  delete m_icon;
}

void Rules::paint(ui::Graphics* g,
  const gfx::Rect& bounds,
  const char* text)
{
  if (m_background) m_background->paint(g, bounds, text);
  if (m_icon) m_icon->paint(g, bounds, text);
  if (m_text) m_text->paint(g, bounds, text);
}

gfx::Size Rules::preferredSize(const char* text)
{
  gfx::Size sz(0, 0);
  if (m_icon) {
    sz.w += m_icon->getPart()->getBitmap(0)->width();
    sz.h = m_icon->getPart()->getBitmap(0)->height();
  }
  if (m_text && text) {
    ui::ScreenGraphics g;
    gfx::Size textSize = g.measureUIString(text);
    //if (sz.w > 0) sz.w += 2;    // TODO text separation
    sz.w += textSize.w;
    sz.h = MAX(sz.h, textSize.h);
  }
  return sz;
}

Style::Style(css::Sheet& sheet, const std::string& id)
  : m_id(id)
  , m_compoundStyle(sheet.compoundStyle(id))
{
}

Style::~Style()
{
  for (RulesMap::iterator it = m_rules.begin(), end = m_rules.end();
       it != end; ++it) {
    delete it->second;
  }
}

Rules* Style::getRulesFromState(const State& state)
{
  Rules* rules = NULL;

  RulesMap::iterator it = m_rules.find(state);
  if (it != m_rules.end()) {
    rules = it->second;
  }
  else {
    rules = new Rules(m_compoundStyle[state]);
    m_rules[state] = rules;
  }

  return rules;
}

void Style::paint(ui::Graphics* g,
  const gfx::Rect& bounds,
  const char* text,
  const State& state)
{
  getRulesFromState(state)->paint(g, bounds, text);
}

gfx::Size Style::preferredSize(
  const char* text,
  const State& state)
{
  return getRulesFromState(state)->preferredSize(text);
}

} // namespace skin
} // namespace app
