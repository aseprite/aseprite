// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/console.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/button_icon_impl.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_slider_property.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui/skin/style_sheet.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/shared_ptr.h"
#include "base/string.h"
#include "css/sheet.h"
#include "gfx/border.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "she/draw_text.h"
#include "she/font.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include "tinyxml.h"

#define BGCOLOR                 (getWidgetBgColor(widget))

namespace app {
namespace skin {

using namespace gfx;
using namespace ui;

const char* SkinTheme::kThemesFolderName = "themes";

static const char* cursor_names[kCursorTypes] = {
  "null",                       // kNoCursor
  "normal",                     // kArrowCursor
  "normal_add",                 // kArrowPlusCursor
  "crosshair",                  // kCrosshairCursor
  "forbidden",                  // kForbiddenCursor
  "hand",                       // kHandCursor
  "scroll",                     // kScrollCursor
  "move",                       // kMoveCursor

  "size_ns",                    // kSizeNSCursor
  "size_we",                    // kSizeWECursor

  "size_n",                     // kSizeNCursor
  "size_ne",                    // kSizeNECursor
  "size_e",                     // kSizeECursor
  "size_se",                    // kSizeSECursor
  "size_s",                     // kSizeSCursor
  "size_sw",                    // kSizeSWCursor
  "size_w",                     // kSizeWCursor
  "size_nw",                    // kSizeNWCursor

  "rotate_n",                   // kRotateNCursor
  "rotate_ne",                  // kRotateNECursor
  "rotate_e",                   // kRotateECursor
  "rotate_se",                  // kRotateSECursor
  "rotate_s",                   // kRotateSCursor
  "rotate_sw",                  // kRotateSWCursor
  "rotate_w",                   // kRotateWCursor
  "rotate_nw",                  // kRotateNWCursor

  "eyedropper",                 // kEyedropperCursor
  "magnifier"                   // kMagnifierCursor
};

static css::Value value_or_none(const char* valueStr)
{
  if (strcmp(valueStr, "none") == 0)
    return css::Value();
  else
    return css::Value(valueStr);
}

// static
SkinTheme* SkinTheme::instance()
{
  return static_cast<SkinTheme*>(ui::Manager::getDefault()->theme());
}

SkinTheme::SkinTheme()
  : m_cursors(ui::kCursorTypes, NULL)
{
  m_defaultFont = nullptr;
  m_miniFont = nullptr;

  // Initialize all graphics in NULL (these bitmaps are loaded from the skin)
  m_sheet = NULL;
}

SkinTheme::~SkinTheme()
{
  // Delete all cursors.
  for (size_t c=0; c<m_cursors.size(); ++c)
    delete m_cursors[c];

  for (std::map<std::string, she::Surface*>::iterator
         it = m_toolicon.begin(); it != m_toolicon.end(); ++it) {
    it->second->dispose();
  }

  if (m_sheet)
    m_sheet->dispose();

  m_parts_by_id.clear();

  // Destroy fonts
  if (m_defaultFont)
    m_defaultFont->dispose();

  if (m_miniFont)
    m_miniFont->dispose();
}

void SkinTheme::onRegenerate()
{
  Preferences& pref = Preferences::instance();

  // First we load the skin from default theme, which is more proper
  // to have every single needed skin part/color/dimension.
  loadAll(pref.theme.selected.defaultValue());

  // Then we load the selected theme to redefine default theme parts.
  if (pref.theme.selected.defaultValue() != pref.theme.selected()) {
    try {
      loadAll(pref.theme.selected());
    }
    catch (const std::exception& e) {
      LOG("SKIN: Error loading user-theme: %s\n", e.what());

      if (ui::get_theme())
        Console::showException(e);

      // We can continue, as we've already loaded the default theme
      // anyway. Here we restore the setting to its default value.
      pref.theme.selected(pref.theme.selected.defaultValue());
    }
  }
}

void SkinTheme::loadAll(const std::string& skinId)
{
  LOG("SKIN: Loading theme %s\n", skinId.c_str());

  loadSheet(skinId);
  loadFonts(skinId);
  loadXml(skinId);
}

void SkinTheme::loadSheet(const std::string& skinId)
{
  // Load the skin sheet
  std::string sheet_filename(themeFileName(skinId, "sheet.png"));
  ResourceFinder rf;
  rf.includeDataDir(sheet_filename.c_str());
  if (!rf.findFirst())
    throw base::Exception("File %s not found", sheet_filename.c_str());

  try {
    if (m_sheet) {
      m_sheet->dispose();
      m_sheet = nullptr;
    }
    m_sheet = she::instance()->loadRgbaSurface(rf.filename().c_str());
  }
  catch (...) {
    throw base::Exception("Error loading %s file", sheet_filename.c_str());
  }
}

void SkinTheme::loadFonts(const std::string& skinId)
{
  if (m_defaultFont) m_defaultFont->dispose();
  if (m_miniFont) m_miniFont->dispose();

  Preferences& pref = Preferences::instance();

  m_defaultFont = loadFont(pref.theme.font(), themeFileName(skinId, "font.png"));
  m_miniFont = loadFont(pref.theme.miniFont(), themeFileName(skinId, "minifont.png"));
}

void SkinTheme::loadXml(const std::string& skinId)
{
  // Load the skin XML
  std::string xml_filename(themeFileName(skinId, "theme.xml"));
  ResourceFinder rf;
  rf.includeDataDir(xml_filename.c_str());
  if (!rf.findFirst())
    return;

  XmlDocumentRef doc = open_xml(rf.filename());
  TiXmlHandle handle(doc.get());

  // Load dimension
  {
    TiXmlElement* xmlDim = handle
      .FirstChild("theme")
      .FirstChild("dimensions")
      .FirstChild("dim").ToElement();
    while (xmlDim) {
      std::string id = xmlDim->Attribute("id");
      uint32_t value = strtol(xmlDim->Attribute("value"), NULL, 10);

      LOG(VERBOSE) << "SKIN: Loading dimension '" << id << "\n";

      m_dimensions_by_id[id] = value;
      xmlDim = xmlDim->NextSiblingElement();
    }
  }

  // Load colors
  {
    TiXmlElement* xmlColor = handle
      .FirstChild("theme")
      .FirstChild("colors")
      .FirstChild("color").ToElement();
    while (xmlColor) {
      std::string id = xmlColor->Attribute("id");
      uint32_t value = strtol(xmlColor->Attribute("value")+1, NULL, 16);
      gfx::Color color = gfx::rgba(
        (value & 0xff0000) >> 16,
        (value & 0xff00) >> 8,
        (value & 0xff));

      LOG(VERBOSE) << "SKIN: Loading color " << id << "\n";

      m_colors_by_id[id] = color;
      xmlColor = xmlColor->NextSiblingElement();
    }
  }

  // Load cursors
  {
    TiXmlElement* xmlCursor = handle
      .FirstChild("theme")
      .FirstChild("cursors")
      .FirstChild("cursor").ToElement();
    while (xmlCursor) {
      std::string id = xmlCursor->Attribute("id");
      int x = strtol(xmlCursor->Attribute("x"), NULL, 10);
      int y = strtol(xmlCursor->Attribute("y"), NULL, 10);
      int w = strtol(xmlCursor->Attribute("w"), NULL, 10);
      int h = strtol(xmlCursor->Attribute("h"), NULL, 10);
      int focusx = strtol(xmlCursor->Attribute("focusx"), NULL, 10);
      int focusy = strtol(xmlCursor->Attribute("focusy"), NULL, 10);
      int c;

      LOG(VERBOSE) << "SKIN: Loading cursor " << id << "\n";

      for (c=0; c<kCursorTypes; ++c) {
        if (id != cursor_names[c])
          continue;

        delete m_cursors[c];
        m_cursors[c] = NULL;

        she::Surface* slice = sliceSheet(NULL, gfx::Rect(x, y, w, h));

        m_cursors[c] = new Cursor(slice,
          gfx::Point(focusx*guiscale(), focusy*guiscale()));
        break;
      }

      if (c == kCursorTypes) {
        throw base::Exception("Unknown cursor specified in '%s':\n"
                              "<cursor id='%s' ... />\n", xml_filename.c_str(), id.c_str());
      }

      xmlCursor = xmlCursor->NextSiblingElement();
    }
  }

  // Load tool icons
  {
    TiXmlElement* xmlIcon = handle
      .FirstChild("theme")
      .FirstChild("tools")
      .FirstChild("tool").ToElement();
    while (xmlIcon) {
      // Get the tool-icon rectangle
      const char* id = xmlIcon->Attribute("id");
      int x = strtol(xmlIcon->Attribute("x"), NULL, 10);
      int y = strtol(xmlIcon->Attribute("y"), NULL, 10);
      int w = strtol(xmlIcon->Attribute("w"), NULL, 10);
      int h = strtol(xmlIcon->Attribute("h"), NULL, 10);

      LOG(VERBOSE) << "SKIN: Loading tool icon " << id << "\n";

      // Crop the tool-icon from the sheet
      m_toolicon[id] = sliceSheet(
        m_toolicon[id], gfx::Rect(x, y, w, h));

      xmlIcon = xmlIcon->NextSiblingElement();
    }
  }

  // Load parts
  {
    TiXmlElement* xmlPart = handle
      .FirstChild("theme")
      .FirstChild("parts")
      .FirstChild("part").ToElement();
    while (xmlPart) {
      // Get the tool-icon rectangle
      const char* part_id = xmlPart->Attribute("id");
      int x = strtol(xmlPart->Attribute("x"), NULL, 10);
      int y = strtol(xmlPart->Attribute("y"), NULL, 10);
      int w = xmlPart->Attribute("w") ? strtol(xmlPart->Attribute("w"), NULL, 10): 0;
      int h = xmlPart->Attribute("h") ? strtol(xmlPart->Attribute("h"), NULL, 10): 0;

      LOG(VERBOSE) << "SKIN: Loading part " << part_id << "\n";

      SkinPartPtr part = m_parts_by_id[part_id];
      if (!part)
        part = m_parts_by_id[part_id] = SkinPartPtr(new SkinPart);

      if (w > 0 && h > 0) {
        part->setSpriteBounds(gfx::Rect(x, y, w, h));
        part->setBitmap(0,
          sliceSheet(part->bitmap(0), gfx::Rect(x, y, w, h)));
      }
      else if (xmlPart->Attribute("w1")) { // 3x3-1 part (NW, N, NE, E, SE, S, SW, W)
        int w1 = strtol(xmlPart->Attribute("w1"), NULL, 10);
        int w2 = strtol(xmlPart->Attribute("w2"), NULL, 10);
        int w3 = strtol(xmlPart->Attribute("w3"), NULL, 10);
        int h1 = strtol(xmlPart->Attribute("h1"), NULL, 10);
        int h2 = strtol(xmlPart->Attribute("h2"), NULL, 10);
        int h3 = strtol(xmlPart->Attribute("h3"), NULL, 10);

        part->setSpriteBounds(gfx::Rect(x, y, w1+w2+w3, h1+h2+h3));
        part->setSlicesBounds(gfx::Rect(w1, h1, w2, h2));

        part->setBitmap(0, sliceSheet(part->bitmap(0), gfx::Rect(x, y, w1, h1))); // NW
        part->setBitmap(1, sliceSheet(part->bitmap(1), gfx::Rect(x+w1, y, w2, h1))); // N
        part->setBitmap(2, sliceSheet(part->bitmap(2), gfx::Rect(x+w1+w2, y, w3, h1))); // NE
        part->setBitmap(3, sliceSheet(part->bitmap(3), gfx::Rect(x+w1+w2, y+h1, w3, h2))); // E
        part->setBitmap(4, sliceSheet(part->bitmap(4), gfx::Rect(x+w1+w2, y+h1+h2, w3, h3))); // SE
        part->setBitmap(5, sliceSheet(part->bitmap(5), gfx::Rect(x+w1, y+h1+h2, w2, h3))); // S
        part->setBitmap(6, sliceSheet(part->bitmap(6), gfx::Rect(x, y+h1+h2, w1, h3))); // SW
        part->setBitmap(7, sliceSheet(part->bitmap(7), gfx::Rect(x, y+h1, w1, h2))); // W
      }

      xmlPart = xmlPart->NextSiblingElement();
    }
  }

  // Load old stylesheet
  {
    TiXmlElement* xmlStyle = handle
      .FirstChild("theme")
      .FirstChild("stylesheet")
      .FirstChild("style").ToElement();
    while (xmlStyle) {
      const char* style_id = xmlStyle->Attribute("id");
      const char* base_id = xmlStyle->Attribute("base");
      const css::Style* base = NULL;

      if (base_id)
        base = m_stylesheet.getCssStyle(base_id);

      css::Style* style = new css::Style(style_id, base);
      m_stylesheet.addCssStyle(style);

      TiXmlElement* xmlRule = xmlStyle->FirstChildElement();
      while (xmlRule) {
        const std::string ruleName = xmlRule->Value();

        LOG(VERBOSE) << "SKIN: Rule " << ruleName
                     << " for " << style_id << "\n";

        // TODO This code design to read styles could be improved.

        const char* part_id = xmlRule->Attribute("part");
        const char* color_id = xmlRule->Attribute("color");

        // Style align
        int align = 0;
        const char* halign = xmlRule->Attribute("align");
        const char* valign = xmlRule->Attribute("valign");
        const char* wordwrap = xmlRule->Attribute("wordwrap");
        if (halign) {
          if (strcmp(halign, "left") == 0) align |= LEFT;
          else if (strcmp(halign, "right") == 0) align |= RIGHT;
          else if (strcmp(halign, "center") == 0) align |= CENTER;
        }
        if (valign) {
          if (strcmp(valign, "top") == 0) align |= TOP;
          else if (strcmp(valign, "bottom") == 0) align |= BOTTOM;
          else if (strcmp(valign, "middle") == 0) align |= MIDDLE;
        }
        if (wordwrap && strcmp(wordwrap, "true") == 0)
          align |= WORDWRAP;

        if (ruleName == "background") {
          const char* repeat_id = xmlRule->Attribute("repeat");

          if (color_id) (*style)[StyleSheet::backgroundColorRule()] = value_or_none(color_id);
          if (part_id) (*style)[StyleSheet::backgroundPartRule()] = value_or_none(part_id);
          if (repeat_id) (*style)[StyleSheet::backgroundRepeatRule()] = value_or_none(repeat_id);
        }
        else if (ruleName == "icon") {
          if (align) (*style)[StyleSheet::iconAlignRule()] = css::Value(align);
          if (part_id) (*style)[StyleSheet::iconPartRule()] = css::Value(part_id);
          if (color_id) (*style)[StyleSheet::iconColorRule()] = value_or_none(color_id);

          const char* x = xmlRule->Attribute("x");
          const char* y = xmlRule->Attribute("y");
          if (x) (*style)[StyleSheet::iconXRule()] = css::Value(strtol(x, NULL, 10));
          if (y) (*style)[StyleSheet::iconYRule()] = css::Value(strtol(y, NULL, 10));
        }
        else if (ruleName == "text") {
          if (color_id) (*style)[StyleSheet::textColorRule()] = css::Value(color_id);
          if (align) (*style)[StyleSheet::textAlignRule()] = css::Value(align);

          const char* l = xmlRule->Attribute("padding-left");
          const char* t = xmlRule->Attribute("padding-top");
          const char* r = xmlRule->Attribute("padding-right");
          const char* b = xmlRule->Attribute("padding-bottom");

          if (l) (*style)[StyleSheet::paddingLeftRule()] = css::Value(strtol(l, NULL, 10));
          if (t) (*style)[StyleSheet::paddingTopRule()] = css::Value(strtol(t, NULL, 10));
          if (r) (*style)[StyleSheet::paddingRightRule()] = css::Value(strtol(r, NULL, 10));
          if (b) (*style)[StyleSheet::paddingBottomRule()] = css::Value(strtol(b, NULL, 10));
        }

        xmlRule = xmlRule->NextSiblingElement();
      }

      xmlStyle = xmlStyle->NextSiblingElement();
    }
  }

  // Load new styles
  {
    TiXmlElement* xmlStyle = handle
      .FirstChild("theme")
      .FirstChild("styles")
      .FirstChild("style").ToElement();
    while (xmlStyle) {
      const char* style_id = xmlStyle->Attribute("id");
      if (!style_id) {
        throw base::Exception("<style> without 'id' attribute in '%s'\n",
                              xml_filename.c_str());
      }

      const char* extends_id = xmlStyle->Attribute("extends");
      const ui::Style* base = nullptr;
      if (extends_id)
        base = m_styles[extends_id];

      ui::Style* style = m_styles[style_id];
      if (!style) {
        m_styles[style_id] = style = new ui::Style(base);
        style->setId(style_id);
      }
      else {
        *style = ui::Style(base);
      }

      // Margin
      {
        const char* m = xmlStyle->Attribute("margin");
        const char* l = xmlStyle->Attribute("margin-left");
        const char* t = xmlStyle->Attribute("margin-top");
        const char* r = xmlStyle->Attribute("margin-right");
        const char* b = xmlStyle->Attribute("margin-bottom");
        gfx::Border margin = ui::Style::UndefinedBorder();
        if (m || l) margin.left(std::strtol(l ? l: m, nullptr, 10));
        if (m || t) margin.top(std::strtol(t ? t: m, nullptr, 10));
        if (m || r) margin.right(std::strtol(r ? r: m, nullptr, 10));
        if (m || b) margin.bottom(std::strtol(b ? b: m, nullptr, 10));
        style->setMargin(margin*guiscale());
      }

      // Border
      {
        const char* m = xmlStyle->Attribute("border");
        const char* l = xmlStyle->Attribute("border-left");
        const char* t = xmlStyle->Attribute("border-top");
        const char* r = xmlStyle->Attribute("border-right");
        const char* b = xmlStyle->Attribute("border-bottom");
        gfx::Border border = ui::Style::UndefinedBorder();
        if (m || l) border.left(std::strtol(l ? l: m, nullptr, 10));
        if (m || t) border.top(std::strtol(t ? t: m, nullptr, 10));
        if (m || r) border.right(std::strtol(r ? r: m, nullptr, 10));
        if (m || b) border.bottom(std::strtol(b ? b: m, nullptr, 10));
        style->setBorder(border*guiscale());
      }

      // Padding
      {
        const char* m = xmlStyle->Attribute("padding");
        const char* l = xmlStyle->Attribute("padding-left");
        const char* t = xmlStyle->Attribute("padding-top");
        const char* r = xmlStyle->Attribute("padding-right");
        const char* b = xmlStyle->Attribute("padding-bottom");
        gfx::Border padding = ui::Style::UndefinedBorder();
        if (m || l) padding.left(std::strtol(l ? l: m, nullptr, 10));
        if (m || t) padding.top(std::strtol(t ? t: m, nullptr, 10));
        if (m || r) padding.right(std::strtol(r ? r: m, nullptr, 10));
        if (m || b) padding.bottom(std::strtol(b ? b: m, nullptr, 10));
        style->setPadding(padding*guiscale());
      }

      TiXmlElement* xmlLayer = xmlStyle->FirstChildElement();
      while (xmlLayer) {
        const std::string layerName = xmlLayer->Value();

        LOG(VERBOSE) << "SKIN: Layer " << layerName
                     << " for " << style_id << "\n";

        ui::Style::Layer layer;

        // Layer type
        if (layerName == "background") {
          layer.setType(ui::Style::Layer::Type::kBackground);
        }
        else if (layerName == "border") {
          layer.setType(ui::Style::Layer::Type::kBorder);
        }
        else if (layerName == "icon") {
          layer.setType(ui::Style::Layer::Type::kIcon);
        }
        else if (layerName == "text") {
          layer.setType(ui::Style::Layer::Type::kText);
        }
        else if (layerName == "newlayer") {
          layer.setType(ui::Style::Layer::Type::kNewLayer);
        }

        // Parse state condition
        const char* stateValue = xmlLayer->Attribute("state");
        if (stateValue) {
          std::string state(stateValue);
          int flags = 0;
          if (state.find("disabled") != std::string::npos) flags |= ui::Style::Layer::kDisabled;
          if (state.find("selected") != std::string::npos) flags |= ui::Style::Layer::kSelected;
          if (state.find("focus") != std::string::npos) flags |= ui::Style::Layer::kFocus;
          if (state.find("mouse") != std::string::npos) flags |= ui::Style::Layer::kMouse;
          layer.setFlags(flags);
        }

        // Align
        const char* alignValue = xmlLayer->Attribute("align");
        if (alignValue) {
          std::string alignString(alignValue);
          int align = 0;
          if (alignString.find("left") != std::string::npos) align |= LEFT;
          if (alignString.find("center") != std::string::npos) align |= CENTER;
          if (alignString.find("right") != std::string::npos) align |= RIGHT;
          if (alignString.find("top") != std::string::npos) align |= TOP;
          if (alignString.find("middle") != std::string::npos) align |= MIDDLE;
          if (alignString.find("bottom") != std::string::npos) align |= BOTTOM;
          if (alignString.find("wordwrap") != std::string::npos) align |= WORDWRAP;
          layer.setAlign(align);
        }

        // Color
        const char* colorId = xmlLayer->Attribute("color");
        if (colorId) {
          auto it = m_colors_by_id.find(colorId);
          if (it != m_colors_by_id.end())
            layer.setColor(it->second);
          else {
            throw base::Exception("Color <%s color='%s' ...> was not found in '%s'\n",
                                  layerName.c_str(), colorId,
                                  xml_filename.c_str());
          }
        }

        // Sprite sheet
        const char* partId = xmlLayer->Attribute("part");
        if (partId) {
          auto it = m_parts_by_id.find(partId);
          if (it != m_parts_by_id.end()) {
            SkinPartPtr part = it->second;
            if (part) {
              if (layer.type() == ui::Style::Layer::Type::kIcon)
                layer.setIcon(part->bitmap(0));
              else {
                layer.setSpriteSheet(m_sheet);
                layer.setSpriteBounds(part->spriteBounds());
                layer.setSlicesBounds(part->slicesBounds());
              }
            }
          }
          else {
            throw base::Exception("Part <%s part='%s' ...> was not found in '%s'\n",
                                  layerName.c_str(), partId,
                                  xml_filename.c_str());
          }
        }

        if (layer.type() != ui::Style::Layer::Type::kNone)
          style->addLayer(layer);

        xmlLayer = xmlLayer->NextSiblingElement();
      }

      xmlStyle = xmlStyle->NextSiblingElement();
    }
  }

  ThemeFile<SkinTheme>::updateInternals();
}

she::Surface* SkinTheme::sliceSheet(she::Surface* sur, const gfx::Rect& bounds)
{
  if (sur && (sur->width() != bounds.w ||
              sur->height() != bounds.h)) {
    sur->dispose();
    sur = NULL;
  }

  if (!sur)
    sur = she::instance()->createRgbaSurface(bounds.w, bounds.h);

  {
    she::SurfaceLock lockSrc(m_sheet);
    she::SurfaceLock lockDst(sur);
    m_sheet->blitTo(sur, bounds.x, bounds.y, 0, 0, bounds.w, bounds.h);
  }

  sur->applyScale(guiscale());
  return sur;
}

she::Font* SkinTheme::getWidgetFont(const Widget* widget) const
{
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery && skinPropery->hasMiniFont())
    return getMiniFont();
  else
    return getDefaultFont();
}

Cursor* SkinTheme::getCursor(CursorType type)
{
  if (type == kNoCursor) {
    return NULL;
  }
  else {
    ASSERT(type >= kFirstCursorType && type <= kLastCursorType);
    return m_cursors[type];
  }
}

void SkinTheme::initWidget(Widget* widget)
{
#define BORDER(n)                               \
  widget->setBorder(gfx::Border(n))

#define BORDER4(L,T,R,B)                                \
  widget->setBorder(gfx::Border((L), (T), (R), (B)))

  int scale = guiscale();

  switch (widget->type()) {

    case kBoxWidget:
      BORDER(0);
      widget->setChildSpacing(4 * scale);
      break;

    case kButtonWidget:
      widget->setStyle(newStyles.button());
      break;

    case kCheckWidget:
      BORDER(2 * scale);
      widget->setChildSpacing(4 * scale);

      static_cast<ButtonBase*>(widget)->setIconInterface
        (new ButtonIconImpl(parts.checkNormal(),
                            parts.checkSelected(),
                            parts.checkDisabled(),
                            LEFT | MIDDLE));
      break;

    case kEntryWidget:
      BORDER4(
        parts.sunkenNormal()->bitmapW()->width(),
        parts.sunkenNormal()->bitmapN()->height(),
        parts.sunkenNormal()->bitmapE()->width(),
        parts.sunkenNormal()->bitmapS()->height());
      widget->setChildSpacing(3 * scale);
      break;

    case kGridWidget:
      BORDER(0);
      widget->setChildSpacing(4 * scale);
      break;

    case kLabelWidget:
      widget->setStyle(newStyles.label());
      break;

    case kLinkLabelWidget:
      widget->setStyle(newStyles.link());
      break;

    case kListBoxWidget:
      BORDER(0);
      widget->setChildSpacing(0);
      break;

    case kListItemWidget:
      BORDER(1 * scale);
      break;

    case kComboBoxWidget: {
      ComboBox* combobox = static_cast<ComboBox*>(widget);
      Button* button = combobox->getButtonWidget();
      button->setStyle(newStyles.comboboxButton());
      break;
    }

    case kMenuWidget:
      widget->setStyle(newStyles.menu());
      break;

    case kMenuBarWidget:
      widget->setStyle(newStyles.menubar());
      break;

    case kMenuBoxWidget:
      widget->setStyle(newStyles.menubox());
      break;

    case kMenuItemWidget:
      BORDER(2 * scale);
      widget->setChildSpacing(18 * scale);
      break;

    case kSplitterWidget:
      BORDER(0);
      widget->setChildSpacing(3 * scale);
      break;

    case kRadioWidget:
      BORDER(2 * scale);
      widget->setChildSpacing(4 * scale);

      static_cast<ButtonBase*>(widget)->setIconInterface
        (new ButtonIconImpl(parts.radioNormal(),
                            parts.radioSelected(),
                            parts.radioDisabled(),
                            LEFT | MIDDLE));
      break;

    case kSeparatorWidget:
      // Frame
      if ((widget->align() & HORIZONTAL) &&
          (widget->align() & VERTICAL)) {
        BORDER(4 * scale);
      }
      // Horizontal bar
      else if (widget->align() & HORIZONTAL) {
        BORDER4(2 * scale, 4 * scale, 2 * scale, 0);
      }
      // Vertical bar
      else {
        BORDER4(4 * scale, 2 * scale, 1 * scale, 2 * scale);
      }
      break;

    case kSliderWidget:
      BORDER4(
        parts.sliderEmpty()->bitmapW()->width()-1*scale,
        parts.sliderEmpty()->bitmapN()->height(),
        parts.sliderEmpty()->bitmapE()->width()-1*scale,
        parts.sliderEmpty()->bitmapS()->height()-1*scale);
      widget->setChildSpacing(widget->textHeight());
      widget->setAlign(CENTER | MIDDLE);
      break;

    case kTextBoxWidget:
      BORDER(4*guiscale());
      widget->setChildSpacing(0);
      widget->setBgColor(colors.textboxFace());
      break;

    case kViewWidget:
      widget->setChildSpacing(0);
      widget->setBgColor(colors.windowFace());
      widget->setStyle(newStyles.view());
      break;

    case kViewScrollbarWidget:
      BORDER(1 * scale);
      widget->setChildSpacing(0);
      break;

    case kViewViewportWidget:
      BORDER(0);
      widget->setChildSpacing(0);
      break;

    case kManagerWidget:
      widget->setStyle(newStyles.desktop());
      break;

    case kWindowWidget:
      if (TipWindow* window = dynamic_cast<TipWindow*>(widget)) {
        window->setStyle(newStyles.tooltipWindow());
        window->setArrowStyle(newStyles.tooltipWindowArrow());
        window->textBox()->setStyle(SkinTheme::instance()->newStyles.tooltipText());
      }
      else if (dynamic_cast<TransparentPopupWindow*>(widget)) {
        widget->setStyle(newStyles.transparentPopupWindow());
      }
      else if (dynamic_cast<PopupWindow*>(widget)) {
        widget->setStyle(newStyles.popupWindow());
      }
      else if (static_cast<Window*>(widget)->isDesktop()) {
        widget->setStyle(newStyles.desktop());
      }
      else {
        if (widget->hasText()) {
          widget->setStyle(newStyles.windowWithTitle());
        }
        else {
          widget->setStyle(newStyles.windowWithoutTitle());
        }
      }
      break;

    case kWindowTitleLabelWidget:
      widget->setStyle(SkinTheme::instance()->newStyles.windowTitleLabel());
      break;

    case kWindowCloseButtonWidget:
      widget->setStyle(SkinTheme::instance()->newStyles.windowCloseButton());
      break;

    default:
      break;
  }
}

void SkinTheme::getWindowMask(Widget* widget, Region& region)
{
  region = widget->bounds();
}

int SkinTheme::getScrollbarSize()
{
  return dimensions.scrollbarSize();
}

gfx::Size SkinTheme::getEntryCaretSize(Widget* widget)
{
  if (widget->font()->type() == she::FontType::kTrueType)
    return gfx::Size(2*guiscale(), widget->textHeight());
  else
    return gfx::Size(2*guiscale(), widget->textHeight()+2*guiscale());
}

void SkinTheme::paintBox(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.graphics();

  if (!widget->isTransparent() &&
      !is_transparent(BGCOLOR)) {
    g->fillRect(BGCOLOR, g->getClipBounds());
  }
}

void SkinTheme::paintCheckBox(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();
  IButtonIcon* iconInterface = widget->iconInterface();
  gfx::Rect box, text, icon;
  gfx::Color bg;

  widget->getTextIconInfo(&box, &text, &icon,
    iconInterface ? iconInterface->iconAlign(): 0,
    iconInterface ? iconInterface->size().w: 0,
    iconInterface ? iconInterface->size().h: 0);

  // Check box look
  LookType look = NormalLook;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery)
    look = skinPropery->getLook();

  // Background
  g->fillRect(bg = BGCOLOR, bounds);

  // Mouse
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      g->fillRect(bg = colors.checkHotFace(), bounds);
    else if (widget->hasFocus())
      g->fillRect(bg = colors.checkFocusFace(), bounds);
  }

  // Text
  drawText(g, nullptr, ColorNone, ColorNone, widget, text, 0,
           widget->mnemonic());

  // Paint the icon
  if (iconInterface)
    paintIcon(widget, g, iconInterface, icon.x, icon.y);

  // Draw focus
  if (look != WithoutBordersLook &&
      (widget->hasFocus() || (iconInterface &&
                              widget->text().empty() &&
                              widget->hasMouseOver()))) {
    drawRect(g, bounds, parts.checkFocus().get());
  }
}

void SkinTheme::paintGrid(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.graphics();

  if (!is_transparent(BGCOLOR))
    g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintEntry(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Entry* widget = static_cast<Entry*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();

  // Outside borders
  g->fillRect(BGCOLOR, bounds);

  bool isMiniLook = false;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  drawRect(g, bounds,
    (widget->hasFocus() ?
     (isMiniLook ? parts.sunkenMiniFocused().get(): parts.sunkenFocused().get()):
     (isMiniLook ? parts.sunkenMiniNormal().get() : parts.sunkenNormal().get())));

  drawEntryText(g, widget);
}

namespace {

class DrawEntryTextDelegate : public she::DrawTextDelegate {
public:
  DrawEntryTextDelegate(Entry* widget, Graphics* graphics,
                        const gfx::Point& pos, const int h)
    : m_widget(widget)
    , m_graphics(graphics)
    , m_caretDrawn(false)
      // m_lastX is an absolute position on screen
    , m_lastX(pos.x+m_widget->bounds().x)
    , m_y(pos.y)
    , m_h(h)
  {
    m_widget->getEntryThemeInfo(&m_index, &m_caret, &m_state, &m_selbeg, &m_selend);
  }

  int index() const { return m_index; }
  bool caretDrawn() const { return m_caretDrawn; }
  const gfx::Rect& textBounds() const { return m_textBounds; }

  void preProcessChar(const base::utf8_const_iterator& it,
                      const base::utf8_const_iterator& end,
                      int& chr,
                      gfx::Color& fg,
                      gfx::Color& bg,
                      bool& drawChar,
                      bool& moveCaret) override {
    if (m_widget->isPassword())
      chr = '*';

    // Normal text
    auto& colors = SkinTheme::instance()->colors;
    bg = ColorNone;
    fg = colors.text();

    // Selected
    if ((m_index >= m_selbeg) &&
        (m_index <= m_selend)) {
      if (m_widget->hasFocus())
        bg = colors.selected();
      else
        bg = colors.disabled();
      fg = colors.background();
    }

    // Disabled
    if (!m_widget->isEnabled()) {
      bg = ColorNone;
      fg = colors.disabled();
    }

    drawChar = true;
    moveCaret = true;
    m_bg = bg;
  }

  bool preDrawChar(const gfx::Rect& charBounds) override {
    m_textBounds |= charBounds;
    m_charStartX = charBounds.x;

    if (charBounds.x2()-m_widget->bounds().x < m_widget->clientBounds().x2()) {
      if (m_bg != ColorNone) {
        // Fill background e.g. needed for selected/highlighted
        // regions with TTF fonts where the char is smaller than the
        // text bounds [m_y,m_y+m_h)
        gfx::Rect fillThisRect(m_lastX-m_widget->bounds().x,
                               m_y, charBounds.x2()-m_lastX, m_h);
        if (charBounds != fillThisRect)
          m_graphics->fillRect(m_bg, fillThisRect);
      }
      m_lastX = charBounds.x2();
      return true;
    }
    else
      return false;
  }

  void postDrawChar(const gfx::Rect& charBounds) override {
    // Caret
    if (m_state &&
        m_index == m_caret &&
        m_widget->hasFocus() &&
        m_widget->isEnabled()) {
      SkinTheme::instance()->drawEntryCaret(
        m_graphics, m_widget,
        m_charStartX-m_widget->bounds().x, m_y);
      m_caretDrawn = true;
    }

    ++m_index;
  }

private:
  Entry* m_widget;
  Graphics* m_graphics;
  int m_index;
  int m_caret;
  int m_state;
  int m_selbeg;
  int m_selend;
  gfx::Rect m_textBounds;
  bool m_caretDrawn;
  gfx::Color m_bg;
  int m_lastX; // Last position used to fill the background
  int m_y, m_h;
  int m_charStartX;
};

} // anonymous namespace

void SkinTheme::drawEntryText(ui::Graphics* g, ui::Entry* widget)
{
  // Draw the text
  gfx::Rect bounds = widget->getEntryTextBounds();

  DrawEntryTextDelegate delegate(widget, g, bounds.origin(), widget->textHeight());
  int scroll = delegate.index();

  const std::string& textString = widget->text();
  base::utf8_const_iterator utf8_it((textString.begin()));
  int textlen = base::utf8_length(textString);
  scroll = MIN(scroll, textlen);
  if (scroll)
    utf8_it += scroll;

  g->drawText(utf8_it,
              base::utf8_const_iterator(textString.end()),
              colors.text(), ColorNone,
              bounds.origin(), &delegate);

  bounds.x += delegate.textBounds().w;

  // Draw suffix if there is enough space
  if (!widget->getSuffix().empty()) {
    Rect sufBounds(bounds.x, bounds.y,
                   bounds.x2()-widget->childSpacing()*guiscale()-bounds.x,
                   widget->textHeight());
    IntersectClip clip(g, sufBounds & widget->clientChildrenBounds());
    if (clip) {
      drawText(
        g, widget->getSuffix().c_str(),
        colors.entrySuffix(), ColorNone,
        widget, sufBounds, 0, 0);
    }
  }

  // Draw caret at the end of the text
  if (!delegate.caretDrawn()) {
    gfx::Rect charBounds(bounds.x+widget->bounds().x,
                         bounds.y+widget->bounds().y, 0, widget->textHeight());
    delegate.preDrawChar(charBounds);
    delegate.postDrawChar(charBounds);
  }
}

void SkinTheme::paintListBox(PaintEvent& ev)
{
  Graphics* g = ev.graphics();

  g->fillRect(colors.background(), g->getClipBounds());
}

void SkinTheme::paintListItem(ui::PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();
  Graphics* g = ev.graphics();
  gfx::Color fg, bg;

  if (!widget->isEnabled()) {
    bg = colors.face();
    fg = colors.disabled();
  }
  else if (widget->isSelected()) {
    fg = colors.listitemSelectedText();
    bg = colors.listitemSelectedFace();
  }
  else {
    fg = colors.listitemNormalText();
    bg = colors.listitemNormalFace();
  }

  g->fillRect(bg, bounds);

  if (widget->hasText()) {
    bounds.shrink(widget->border());
    drawText(g, nullptr, fg, bg, widget, bounds, 0, 0);
  }
}

void SkinTheme::paintMenu(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.graphics();

  g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintMenuItem(ui::PaintEvent& ev)
{
  int scale = guiscale();
  Graphics* g = ev.graphics();
  MenuItem* widget = static_cast<MenuItem*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();
  gfx::Color fg, bg;
  int c, bar;

  // TODO ASSERT?
  if (!widget->parent()->parent())
    return;

  bar = (widget->parent()->parent()->type() == kMenuBarWidget);

  // Colors
  if (!widget->isEnabled()) {
    fg = ColorNone;
    bg = colors.menuitemNormalFace();
  }
  else {
    if (widget->isHighlighted()) {
      fg = colors.menuitemHighlightText();
      bg = colors.menuitemHighlightFace();
    }
    else if (widget->hasMouse()) {
      fg = colors.menuitemHotText();
      bg = colors.menuitemHotFace();
    }
    else {
      fg = colors.menuitemNormalText();
      bg = colors.menuitemNormalFace();
    }
  }

  // Background
  g->fillRect(bg, bounds);

  // Draw an indicator for selected items
  if (widget->isSelected()) {
    she::Surface* icon =
      (widget->isEnabled() ?
       parts.checkSelected()->bitmap(0):
       parts.checkDisabled()->bitmap(0));

    int x = bounds.x+4*scale-icon->width()/2;
    int y = bounds.y+bounds.h/2-icon->height()/2;
    g->drawRgbaSurface(icon, x, y);
  }

  // Text
  if (bar)
    widget->setAlign(CENTER | MIDDLE);
  else
    widget->setAlign(LEFT | MIDDLE);

  Rect pos = bounds;
  if (!bar)
    pos.offset(widget->childSpacing()/2, 0);
  drawText(g, nullptr, fg, ColorNone, widget, pos, 0,
           widget->mnemonic());

  // For menu-box
  if (!bar) {
    // Draw the arrown (to indicate which this menu has a sub-menu)
    if (widget->getSubmenu()) {
      // Enabled
      if (widget->isEnabled()) {
        for (c=0; c<3*scale; c++)
          g->drawVLine(fg,
            bounds.x2()-3*scale-c,
            bounds.y+bounds.h/2-c, 2*c+1);
      }
      // Disabled
      else {
        for (c=0; c<3*scale; c++)
          g->drawVLine(colors.background(),
            bounds.x2()-3*scale-c+1,
            bounds.y+bounds.h/2-c+1, 2*c+1);

        for (c=0; c<3*scale; c++)
          g->drawVLine(colors.disabled(),
            bounds.x2()-3*scale-c,
            bounds.y+bounds.h/2-c, 2*c+1);
      }
    }
    // Draw the keyboard shortcut
    else if (AppMenuItem* appMenuItem = dynamic_cast<AppMenuItem*>(widget)) {
      if (appMenuItem->key() && !appMenuItem->key()->accels().empty()) {
        int old_align = appMenuItem->align();

        pos = bounds;
        pos.w -= widget->childSpacing()/4;

        std::string buf = appMenuItem->key()->accels().front().toString();

        widget->setAlign(RIGHT | MIDDLE);
        drawText(g, buf.c_str(), fg, ColorNone, widget, pos, 0, 0);
        widget->setAlign(old_align);
      }
    }
  }
}

void SkinTheme::paintSplitter(PaintEvent& ev)
{
  Graphics* g = ev.graphics();

  g->fillRect(colors.splitterNormalFace(), g->getClipBounds());
}

void SkinTheme::paintRadioButton(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();
  IButtonIcon* iconInterface = widget->iconInterface();
  gfx::Color bg = BGCOLOR;

  gfx::Rect box, text, icon;
  widget->getTextIconInfo(&box, &text, &icon,
    iconInterface ? iconInterface->iconAlign(): 0,
    iconInterface ? iconInterface->size().w: 0,
    iconInterface ? iconInterface->size().h: 0);

  // Background
  g->fillRect(bg, g->getClipBounds());

  // Mouse
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      g->fillRect(bg = colors.radioHotFace(), bounds);
    else if (widget->hasFocus())
      g->fillRect(bg = colors.radioFocusFace(), bounds);
  }

  // Text
  drawText(g, nullptr, ColorNone, ColorNone, widget, text, 0, widget->mnemonic());

  // Icon
  if (iconInterface)
    paintIcon(widget, g, iconInterface, icon.x, icon.y);

  // Focus
  if (widget->hasFocus())
    drawRect(g, bounds, parts.radioFocus().get());
}

void SkinTheme::paintSeparator(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();

  // background
  g->fillRect(BGCOLOR, bounds);

  if (widget->align() & HORIZONTAL) {
    int h = parts.separatorHorz()->bitmap(0)->height();
    drawHline(g, gfx::Rect(bounds.x, bounds.y+bounds.h/2-h/2,
                           bounds.w, h),
              parts.separatorHorz().get());
  }

  if (widget->align() & VERTICAL) {
    int w = parts.separatorVert()->bitmap(0)->width();
    drawVline(g, gfx::Rect(bounds.x+bounds.w/2-w/2, bounds.y,
                           w, bounds.h),
              parts.separatorVert().get());
  }

  // text
  if (widget->hasText()) {
    int h = widget->textHeight();
    Rect r(
      bounds.x + widget->border().left()/2 + h/2,
      bounds.y + bounds.h/2 - h/2,
      widget->textWidth(), h);

    drawText(g, nullptr,
             colors.separatorLabel(), BGCOLOR,
             widget, r, 0, widget->mnemonic());
  }
}

void SkinTheme::paintSlider(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Slider* widget = static_cast<Slider*>(ev.getSource());
  Rect bounds = widget->clientBounds();
  int min, max, value;

  // Outside borders
  gfx::Color bgcolor = widget->bgColor();
  if (!is_transparent(bgcolor))
    g->fillRect(bgcolor, bounds);

  widget->getSliderThemeInfo(&min, &max, &value);

  Rect rc(Rect(bounds).shrink(widget->border()));
  int x;
  if (min != max)
    x = rc.x + rc.w * (value-min) / (max-min);
  else
    x = rc.x;

  rc = widget->clientBounds();

  // The mini-look is used for sliders with tiny borders.
  bool isMiniLook = false;

  // The BG painter is used for sliders without a number-indicator and
  // customized background (e.g. RGB sliders)
  ISliderBgPainter* bgPainter = NULL;

  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  SkinSliderPropertyPtr skinSliderPropery = widget->getProperty(SkinSliderProperty::Name);
  if (skinSliderPropery)
    bgPainter = skinSliderPropery->getBgPainter();

  // Draw customized background
  if (bgPainter) {
    SkinPartPtr nw = parts.miniSliderEmpty();
    she::Surface* thumb =
      (widget->hasFocus() ? parts.miniSliderThumbFocused()->bitmap(0):
                            parts.miniSliderThumb()->bitmap(0));

    // Draw background
    g->fillRect(BGCOLOR, rc);

    // Draw thumb
    int thumb_y = rc.y;
    if (rc.h > thumb->height()*3)
      rc.shrink(Border(0, thumb->height(), 0, 0));

    // Draw borders
    if (rc.h > 4*guiscale()) {
      rc.shrink(Border(3, 0, 3, 1) * guiscale());
      drawRect(g, rc, nw.get());
    }

    // Draw background (using the customized ISliderBgPainter implementation)
    rc.shrink(Border(1, 1, 1, 2) * guiscale());
    if (!rc.isEmpty())
      bgPainter->paint(widget, g, rc);

    g->drawRgbaSurface(thumb, x-thumb->width()/2, thumb_y);
  }
  else {
    // Draw borders
    SkinPartPtr full_part;
    SkinPartPtr empty_part;

    if (isMiniLook) {
      full_part = widget->hasMouseOver() ? parts.miniSliderFullFocused():
                                           parts.miniSliderFull();
      empty_part = widget->hasMouseOver() ? parts.miniSliderEmptyFocused():
                                            parts.miniSliderEmpty();
    }
    else {
      full_part = widget->hasFocus() ? parts.sliderFullFocused():
                                       parts.sliderFull();
      empty_part = widget->hasFocus() ? parts.sliderEmptyFocused():
                                        parts.sliderEmpty();
    }

    if (value == min)
      drawRect(g, rc, empty_part.get());
    else if (value == max)
      drawRect(g, rc, full_part.get());
    else
      drawRect2(g, rc, x,
                full_part.get(), empty_part.get());

    // Draw text
    std::string old_text = widget->text();
    widget->setTextQuiet(widget->convertValueToText(value));

    {
      IntersectClip clip(g, Rect(rc.x, rc.y, x-rc.x, rc.h));
      if (clip) {
        drawText(g, nullptr,
                 colors.sliderFullText(), ColorNone,
                 widget, rc, 0, widget->mnemonic());
      }
    }

    {
      IntersectClip clip(g, Rect(x+1, rc.y, rc.w-(x-rc.x+1), rc.h));
      if (clip) {
        drawText(g, nullptr,
                 colors.sliderEmptyText(),
                 ColorNone, widget, rc, 0, widget->mnemonic());
      }
    }

    widget->setTextQuiet(old_text.c_str());
  }
}

void SkinTheme::paintComboBoxEntry(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Entry* widget = static_cast<Entry*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();

  // Outside borders
  g->fillRect(BGCOLOR, bounds);

  drawRect(g, bounds,
           (widget->hasFocus() ?
            parts.sunken2Focused().get():
            parts.sunken2Normal().get()));

  drawEntryText(g, widget);
}

void SkinTheme::paintTextBox(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());

  Theme::drawTextBox(g, widget, nullptr, nullptr,
                     BGCOLOR, colors.textboxText());
}

void SkinTheme::paintViewScrollbar(PaintEvent& ev)
{
  ScrollBar* widget = static_cast<ScrollBar*>(ev.getSource());
  Graphics* g = ev.graphics();
  int pos, len;

  bool isMiniLook = false;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  skin::Style* bgStyle;
  skin::Style* thumbStyle;

  if (widget->isTransparent()) {
    bgStyle = styles.transparentScrollbar();
    thumbStyle = styles.transparentScrollbarThumb();
  }
  else if (isMiniLook) {
    bgStyle = styles.miniScrollbar();
    thumbStyle = styles.miniScrollbarThumb();
  }
  else {
    bgStyle = styles.scrollbar();
    thumbStyle = styles.scrollbarThumb();
  }

  widget->getScrollBarThemeInfo(&pos, &len);

  Style::State state;
  if (widget->hasMouse()) state += Style::hover();

  gfx::Rect rc = widget->clientBounds();
  bgStyle->paint(g, rc, NULL, state);

  // Horizontal bar
  if (widget->align() & HORIZONTAL) {
    rc.x += pos;
    rc.w = len;
  }
  // Vertical bar
  else {
    rc.y += pos;
    rc.h = len;
  }

  thumbStyle->paint(g, rc, NULL, state);
}

void SkinTheme::paintViewViewport(PaintEvent& ev)
{
  Viewport* widget = static_cast<Viewport*>(ev.getSource());
  Graphics* g = ev.graphics();
  gfx::Color bg = BGCOLOR;

  if (!is_transparent(bg))
    g->fillRect(bg, widget->clientBounds());
}

gfx::Color SkinTheme::getWidgetBgColor(Widget* widget)
{
  gfx::Color c = widget->bgColor();
  bool decorative = widget->isDecorative();

  if (!is_transparent(c) || widget->type() == kWindowWidget)
    return c;
  else if (decorative)
    return colors.selected();
  else
    return colors.face();
}

void SkinTheme::drawText(Graphics* g, const char *t, gfx::Color fg_color, gfx::Color bg_color,
                         Widget* widget, const Rect& rc,
                         int selected_offset, int mnemonic)
{
  if (t || widget->hasText()) {
    Rect textrc;

    g->setFont(widget->font());

    if (!t)
      t = widget->text().c_str();

    textrc.setSize(g->measureUIText(t));

    // Horizontally text alignment

    if (widget->align() & RIGHT)
      textrc.x = rc.x + rc.w - textrc.w - 1;
    else if (widget->align() & CENTER)
      textrc.x = rc.center().x - textrc.w/2;
    else
      textrc.x = rc.x;

    // Vertically text alignment

    if (widget->align() & BOTTOM)
      textrc.y = rc.y + rc.h - textrc.h - 1;
    else if (widget->align() & MIDDLE)
      textrc.y = rc.center().y - textrc.h/2;
    else
      textrc.y = rc.y;

    if (widget->isSelected()) {
      textrc.x += selected_offset;
      textrc.y += selected_offset;
    }

    // Background
    if (!is_transparent(bg_color)) {
      if (!widget->isEnabled())
        g->fillRect(bg_color, Rect(textrc).inflate(guiscale(), guiscale()));
      else
        g->fillRect(bg_color, textrc);
    }

    // Text
    Rect textWrap = textrc.createIntersection(
      // TODO add ui::Widget::getPadding() property
      // Rect(widget->clientBounds()).shrink(widget->border()));
      widget->clientBounds()).inflate(0, 1*guiscale());

    IntersectClip clip(g, textWrap);
    if (clip) {
      if (!widget->isEnabled()) {
        // Draw white part
        g->drawUIText(
          t,
          colors.background(),
          gfx::ColorNone,
          textrc.origin() + Point(guiscale(), guiscale()),
          mnemonic);
      }

      g->drawUIText(
        t,
        (!widget->isEnabled() ?
         colors.disabled():
         (gfx::geta(fg_color) > 0 ? fg_color :
                                    colors.text())),
        bg_color, textrc.origin(),
        mnemonic);
    }
  }
}

void SkinTheme::drawEntryCaret(ui::Graphics* g, Entry* widget, int x, int y)
{
  gfx::Color color = colors.text();
  int textHeight = widget->textHeight();
  gfx::Size caretSize = getEntryCaretSize(widget);

  for (int u=x; u<x+caretSize.w; ++u)
    g->drawVLine(color, u, y+textHeight/2-caretSize.h/2, caretSize.h);
}

she::Surface* SkinTheme::getToolIcon(const char* toolId) const
{
  std::map<std::string, she::Surface*>::const_iterator it = m_toolicon.find(toolId);
  if (it != m_toolicon.end())
    return it->second;
  else
    return NULL;
}

void SkinTheme::drawRect(Graphics* g, const Rect& rc,
                         she::Surface* nw, she::Surface* n, she::Surface* ne,
                         she::Surface* e, she::Surface* se, she::Surface* s,
                         she::Surface* sw, she::Surface* w)
{
  int x, y;

  // Top

  g->drawRgbaSurface(nw, rc.x, rc.y);
  {
    IntersectClip clip(g, Rect(rc.x+nw->width(), rc.y,
        rc.w-nw->width()-ne->width(), rc.h));
    if (clip) {
      for (x = rc.x+nw->width();
           x < rc.x+rc.w-ne->width();
           x += n->width()) {
        g->drawRgbaSurface(n, x, rc.y);
      }
    }
  }

  g->drawRgbaSurface(ne, rc.x+rc.w-ne->width(), rc.y);

  // Bottom

  g->drawRgbaSurface(sw, rc.x, rc.y+rc.h-sw->height());
  {
    IntersectClip clip(g, Rect(rc.x+sw->width(), rc.y,
        rc.w-sw->width()-se->width(), rc.h));
    if (clip) {
      for (x = rc.x+sw->width();
           x < rc.x+rc.w-se->width();
           x += s->width()) {
        g->drawRgbaSurface(s, x, rc.y+rc.h-s->height());
      }
    }
  }

  g->drawRgbaSurface(se, rc.x+rc.w-se->width(), rc.y+rc.h-se->height());
  {
    IntersectClip clip(g, Rect(rc.x, rc.y+nw->height(),
        rc.w, rc.h-nw->height()-sw->height()));
    if (clip) {
      // Left
      for (y = rc.y+nw->height();
           y < rc.y+rc.h-sw->height();
           y += w->height()) {
        g->drawRgbaSurface(w, rc.x, y);
      }

      // Right
      for (y = rc.y+ne->height();
           y < rc.y+rc.h-se->height();
           y += e->height()) {
        g->drawRgbaSurface(e, rc.x+rc.w-e->width(), y);
      }
    }
  }
}

void SkinTheme::drawRect(ui::Graphics* g, const gfx::Rect& rc,
                         SkinPart* skinPart, const bool drawCenter)
{
  Theme::drawSlices(g, m_sheet, rc,
                    skinPart->spriteBounds(),
                    skinPart->slicesBounds(), drawCenter);
}

void SkinTheme::drawRect2(Graphics* g, const Rect& rc, int x_mid,
                          SkinPart* nw1, SkinPart* nw2)
{
  Rect rc2(rc.x, rc.y, x_mid-rc.x+1, rc.h);
  {
    IntersectClip clip(g, rc2);
    if (clip)
      drawRect(g, rc, nw1);
  }

  rc2.x += rc2.w;
  rc2.w = rc.w - rc2.w;

  IntersectClip clip(g, rc2);
  if (clip)
    drawRect(g, rc, nw2);
}

void SkinTheme::drawHline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* part)
{
  int x;

  for (x = rc.x;
       x < rc.x2()-part->size().w;
       x += part->size().w) {
    g->drawRgbaSurface(part->bitmap(0), x, rc.y);
  }

  if (x < rc.x2()) {
    Rect rc2(x, rc.y, rc.w-(x-rc.x), part->size().h);
    IntersectClip clip(g, rc2);
    if (clip)
      g->drawRgbaSurface(part->bitmap(0), x, rc.y);
  }
}

void SkinTheme::drawVline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* part)
{
  int y;

  for (y = rc.y;
       y < rc.y2()-part->size().h;
       y += part->size().h) {
    g->drawRgbaSurface(part->bitmap(0), rc.x, y);
  }

  if (y < rc.y2()) {
    Rect rc2(rc.x, y, part->size().w, rc.h-(y-rc.y));
    IntersectClip clip(g, rc2);
    if (clip)
      g->drawRgbaSurface(part->bitmap(0), rc.x, y);
  }
}

void SkinTheme::paintProgressBar(ui::Graphics* g, const gfx::Rect& rc0, double progress)
{
  g->drawRect(colors.text(), rc0);

  gfx::Rect rc = rc0;
  rc.shrink(1);

  int u = (int)((double)rc.w*progress);
  u = MID(0, u, rc.w);

  if (u > 0)
    g->fillRect(colors.selected(), gfx::Rect(rc.x, rc.y, u, rc.h));

  if (1+u < rc.w)
    g->fillRect(colors.background(), gfx::Rect(rc.x+u, rc.y, rc.w-u, rc.h));
}

void SkinTheme::paintIcon(Widget* widget, Graphics* g, IButtonIcon* iconInterface, int x, int y)
{
  she::Surface* icon_bmp = NULL;

  // enabled
  if (widget->isEnabled()) {
    if (widget->isSelected())   // selected
      icon_bmp = iconInterface->selectedIcon();
    else
      icon_bmp = iconInterface->normalIcon();
  }
  // disabled
  else {
    icon_bmp = iconInterface->disabledIcon();
  }

  if (icon_bmp)
    g->drawRgbaSurface(icon_bmp, x, y);
}

she::Font* SkinTheme::loadFont(const std::string& userFont, const std::string& themeFont)
{
  // Directories to find the font
  ResourceFinder rf;
  if (!userFont.empty())
    rf.addPath(userFont.c_str());
  rf.includeDataDir(themeFont.c_str());

  // Try to load the font
  while (rf.next()) {
    try {
      she::Font* f = she::instance()->loadSpriteSheetFont(rf.filename().c_str(), guiscale());
      if (f->isScalable())
        f->setSize(8);
      return f;
    }
    catch (const std::exception&) {
      // Do nothing
    }
  }

  return nullptr;
}

std::string SkinTheme::themeFileName(const std::string& skinId,
                                     const std::string& fileName) const
{
  std::string path = base::join_path(SkinTheme::kThemesFolderName, skinId);
  path = base::join_path(path, fileName);
  return path;
}

} // namespace skin
} // namespace app
