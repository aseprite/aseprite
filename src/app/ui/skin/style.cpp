// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
css::State Style::m_disabledState("disabled");

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

  if (m_part && m_part->countBitmaps() > 0) {
    if (m_part->countBitmaps() == 1) {
      if (!gfx::is_transparent(m_color))
        g->fillRect(m_color, bounds);

      she::Surface* bmp = m_part->bitmap(0);

      if (m_repeat == BackgroundRepeat::NO_REPEAT) {
        g->drawRgbaSurface(bmp, bounds.x, bounds.y);
      }
      else {
        ui::IntersectClip clip(g, bounds);
        if (!clip)
          return;

        for (int y=bounds.y; y<bounds.y2(); y+=bmp->height()) {
          for (int x=bounds.x; x<bounds.x2(); x+=bmp->width()) {
            g->drawRgbaSurface(bmp, x, y);
            if (m_repeat == BackgroundRepeat::REPEAT_Y)
              break;
          }
          if (m_repeat == BackgroundRepeat::REPEAT_X)
            break;
        }
      }
    }
    else if (m_part->countBitmaps() == 8) {
      theme->drawRect(g, bounds, m_part.get(), m_color);
    }
  }
  else if (!gfx::is_transparent(m_color)) {
    g->fillRect(m_color, bounds);
  }
}

void TextRule::onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text)
{
  SkinTheme* theme = static_cast<SkinTheme*>(ui::CurrentTheme::get());

  if (text) {
    g->drawAlignedUIString(text,
      (gfx::is_transparent(m_color) ?
        theme->colors.text():
        m_color),
      gfx::ColorNone,
      gfx::Rect(bounds).shrink(m_padding), m_align);
  }
}

void IconRule::onPaint(ui::Graphics* g, const gfx::Rect& bounds, const char* text)
{
  she::Surface* bmp = m_part->bitmap(0);
  int x, y;

  if (m_align & ui::RIGHT)
    x = bounds.x2() - bmp->width();
  else if (m_align & ui::CENTER)
    x = bounds.x + bounds.w/2 - bmp->width()/2;
  else
    x = bounds.x;

  if (m_align & ui::BOTTOM)
    y = bounds.y2() - bmp->height();
  else if (m_align & ui::MIDDLE)
    y = bounds.y + bounds.h/2 - bmp->height()/2;
  else
    y = bounds.y;

  x += m_x;
  y += m_y;

  g->drawRgbaSurface(bmp, x, y);
}

Rules::Rules(const css::Query& query) :
  m_background(NULL),
  m_text(NULL),
  m_icon(NULL)
{
  css::Value backgroundColor = query[StyleSheet::backgroundColorRule()];
  css::Value backgroundPart = query[StyleSheet::backgroundPartRule()];
  css::Value backgroundRepeat = query[StyleSheet::backgroundRepeatRule()];
  css::Value iconAlign = query[StyleSheet::iconAlignRule()];
  css::Value iconPart = query[StyleSheet::iconPartRule()];
  css::Value iconX = query[StyleSheet::iconXRule()];
  css::Value iconY = query[StyleSheet::iconYRule()];
  css::Value textAlign = query[StyleSheet::textAlignRule()];
  css::Value textColor = query[StyleSheet::textColorRule()];
  css::Value paddingLeft = query[StyleSheet::paddingLeftRule()];
  css::Value paddingTop = query[StyleSheet::paddingTopRule()];
  css::Value paddingRight = query[StyleSheet::paddingRightRule()];
  css::Value paddingBottom = query[StyleSheet::paddingBottomRule()];
  css::Value none;

  if (backgroundColor != none
    || backgroundPart != none
    || backgroundRepeat != none) {
    m_background = new BackgroundRule();
    m_background->setColor(StyleSheet::convertColor(backgroundColor));
    m_background->setPart(StyleSheet::convertPart(backgroundPart));
    m_background->setRepeat(StyleSheet::convertRepeat(backgroundRepeat));
  }

  if (iconAlign != none
    || iconPart != none
    || iconX != none
    || iconY != none) {
    m_icon = new IconRule();
    m_icon->setAlign((int)iconAlign.number());
    m_icon->setPart(StyleSheet::convertPart(iconPart));
    m_icon->setX((int)iconX.number()*ui::guiscale());
    m_icon->setY((int)iconY.number()*ui::guiscale());
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
        int(paddingLeft.number()),
        int(paddingTop.number()),
        int(paddingRight.number()),
        int(paddingBottom.number()))*ui::guiscale());
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

gfx::Size Rules::sizeHint(const char* text, int maxWidth)
{
  gfx::Size sz(0, 0);
  if (m_icon) {
    sz.w += m_icon->getPart()->bitmap(0)->width();
    sz.h = m_icon->getPart()->bitmap(0)->height();
  }
  if (m_text && text) {
    ui::ScreenGraphics g;
    gfx::Size textSize = g.fitString(text, maxWidth, m_text->align());
    if (sz.w > 0) sz.w += 2*ui::guiscale();    // TODO text separation
    sz.w += textSize.w;
    sz.h = MAX(sz.h, textSize.h);

    sz.w += m_text->padding().left() + m_text->padding().right();
    sz.h += m_text->padding().top() + m_text->padding().bottom();
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

gfx::Size Style::sizeHint(
  const char* text,
  const State& state,
  int maxWidth)
{
  return getRulesFromState(state)->sizeHint(text, maxWidth);
}

} // namespace skin
} // namespace app
