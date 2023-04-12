// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/skin_theme.h"

#include "app/app.h"
#include "app/console.h"
#include "app/extensions.h"
#include "app/font_path.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/font_data.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_slider_property.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/string.h"
#include "base/utf8_decode.h"
#include "gfx/border.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "os/draw_text.h"
#include "os/font.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include "tinyxml.h"

#include <algorithm>
#include <cstring>
#include <memory>

#define BGCOLOR                 (getWidgetBgColor(widget))

namespace app {
namespace skin {

using namespace gfx;
using namespace ui;

// TODO For backward compatibility, in future versions we should remove this (extensions are preferred)
const char* SkinTheme::kThemesFolderName = "themes";

// Offers backward compatibility with old themes, copying missing
// styles (raw XML <style> elements) from the default theme to the
// current theme. This must be done so all <style> elements in the new
// theme use colors and parts from the new theme instead of the
// default one (as when they were loaded).
class app::skin::SkinTheme::BackwardCompatibility {

  enum class State {
    // When we are loading the default theme
    LoadingStyles,
    // When we are loading the selected theme (so we must copy missing
    // styles from the previously loaded default theme)
    CopyingStyles,
  };

  State m_state = State::LoadingStyles;

  // Loaded XML <style> element from the original theme (cloned
  // elements).  Must be in order to insert them in the same order in
  // the selected theme.
  std::vector<std::unique_ptr<TiXmlElement>> m_styles;

public:
  void copyingStyles() {
    m_state = State::CopyingStyles;
  }

  // Called for each <style> element found in theme.xml.
  void onStyle(TiXmlElement* xmlStyle) {
    // Loading <style> from the default theme
    if (m_state == State::LoadingStyles)
      m_styles.emplace_back((TiXmlElement*)xmlStyle->Clone());
  }

  void removeExistentStyles(TiXmlElement* xmlStyle) {
    if (m_state != State::CopyingStyles)
      return;

    while (xmlStyle) {
      const char* s = xmlStyle->Attribute("id");
      if (!s)
        break;
      std::string styleId = s;

      // Remove any existent style in the selected theme.
      auto it = std::find_if(m_styles.begin(),
                             m_styles.end(),
                             [styleId](auto& style){
                               return (style->Attribute("id") == styleId);
                             });
      if (it != m_styles.end())
        m_styles.erase(it);

      xmlStyle = xmlStyle->NextSiblingElement();
    }
  }

  // Copies all missing <style> elements to the new theme. xmlStyles
  // is the <styles> element from the theme.xml of the selected theme
  // (non the default one).
  void copyMissingStyles(TiXmlNode* xmlStyles) {
    if (m_state != State::CopyingStyles)
      return;

    for (auto& style : m_styles) {
      LOG(VERBOSE, "THEME: Copying <style id='%s'> from default theme\n",
          style->Attribute("id"));

      // InsertEndChild() clones the node
      xmlStyles->InsertEndChild(*style.get());
    }
  }
};

static const char* g_cursor_names[kCursorTypes] = {
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
};

static FontData* load_font(std::map<std::string, FontData*>& fonts,
                           const TiXmlElement* xmlFont,
                           const std::string& xmlFilename)
{
  const char* fontRef = xmlFont->Attribute("font");
  if (fontRef) {
    auto it = fonts.find(fontRef);
    if (it == fonts.end())
      throw base::Exception("Font named '%s' not found\n", fontRef);
    return it->second;
  }

  const char* nameStr = xmlFont->Attribute("name");
  if (!nameStr)
    throw base::Exception("No \"name\" or \"font\" attributes specified on <font>");

  std::string name(nameStr);

  // Use cached font data
  auto it = fonts.find(name);
  if (it != fonts.end())
    return it->second;

  LOG(VERBOSE, "THEME: Loading font '%s'\n", name.c_str());

  const char* typeStr = xmlFont->Attribute("type");
  if (!typeStr)
    throw base::Exception("<font> without 'type' attribute in '%s'\n",
                          xmlFilename.c_str());

  std::string type(typeStr);
  std::string xmlDir(base::get_file_path(xmlFilename));
  std::unique_ptr<FontData> font(nullptr);

  if (type == "spritesheet") {
    const char* fileStr = xmlFont->Attribute("file");
    if (fileStr) {
      font.reset(new FontData(os::FontType::SpriteSheet));
      font->setFilename(base::join_path(xmlDir, fileStr));
    }
  }
  else if (type == "truetype") {
    const char* platformFileAttrName =
#ifdef _WIN32
      "file_win"
#elif defined __APPLE__
      "file_mac"
#else
      "file_linux"
#endif
      ;

    const char* platformFileStr = xmlFont->Attribute(platformFileAttrName);
    const char* fileStr = xmlFont->Attribute("file");
    bool antialias = true;
    if (xmlFont->Attribute("antialias"))
      antialias = bool_attr(xmlFont, "antialias", false);

    std::string fontFilename;
    if (platformFileStr)
      fontFilename = app::find_font(xmlDir, platformFileStr);
    if (fileStr && fontFilename.empty())
      fontFilename = app::find_font(xmlDir, fileStr);

    // The filename can be empty if the font was not found, anyway we
    // want to keep the font information (e.g. to use the fallback
    // information of this font).
    font.reset(new FontData(os::FontType::FreeType));
    font->setFilename(fontFilename);
    font->setAntialias(antialias);

    if (!fontFilename.empty())
      LOG(VERBOSE, "THEME: Font file '%s' found\n", fontFilename.c_str());
  }
  else {
    throw base::Exception("Invalid type=\"%s\" in '%s' for <font name=\"%s\" ...>\n",
                          type.c_str(), xmlFilename.c_str(), name.c_str());
  }

  FontData* result = nullptr;
  if (font) {
    fonts[name] = result = font.get();
    font.release();

    // Fallback font
    const TiXmlElement* xmlFallback =
      (const TiXmlElement*)xmlFont->FirstChild("fallback");
    if (xmlFallback) {
      FontData* fallback = load_font(fonts, xmlFallback, xmlFilename);
      if (fallback) {
        int size = 10;
        const char* sizeStr = xmlFont->Attribute("size");
        if (sizeStr)
          size = std::strtol(sizeStr, nullptr, 10);

        result->setFallback(fallback, size);
      }
    }
  }
  return result;
}

// static
SkinTheme* SkinTheme::instance()
{
  if (auto mgr = ui::Manager::getDefault())
    return SkinTheme::get(mgr);
  else
    return nullptr;
}

// static
SkinTheme* SkinTheme::get(const ui::Widget* widget)
{
  ASSERT(widget);
  ASSERT(widget->theme());
  ASSERT(dynamic_cast<SkinTheme*>(widget->theme()));
  return static_cast<SkinTheme*>(widget->theme());
}

SkinTheme::SkinTheme()
  : m_sheet(nullptr)
  , m_defaultFont(nullptr)
  , m_miniFont(nullptr)
  , m_preferredScreenScaling(-1)
  , m_preferredUIScaling(-1)
{
  m_standardCursors.fill(nullptr);
}

SkinTheme::~SkinTheme()
{
  // Delete all cursors.
  for (auto& it : m_cursors)
    delete it.second;           // Delete cursor

  m_unscaledSheet.reset();
  m_sheet.reset();
  m_parts_by_id.clear();

  // Delete all styles.
  for (auto style : m_styles)
    delete style.second;
  m_styles.clear();

  // Destroy fonts
  for (auto& kv : m_fonts)
    delete kv.second;          // Delete all FontDatas
  m_fonts.clear();
}

void SkinTheme::onRegenerateTheme()
{
  Preferences& pref = Preferences::instance();
  BackwardCompatibility backward;

  // First we load the skin from default theme, which is more proper
  // to have every single needed skin part/color/dimension.
  loadAll(pref.theme.selected.defaultValue(), &backward);

  // Then we load the selected theme to redefine default theme parts.
  if (pref.theme.selected.defaultValue() != pref.theme.selected()) {
    try {
      backward.copyingStyles();
      loadAll(pref.theme.selected(), &backward);
    }
    catch (const std::exception& e) {
      LOG("THEME: Error loading user-theme: %s\n", e.what());

      // Load default theme again
      loadAll(pref.theme.selected.defaultValue());

      if (ui::get_theme())
        Console::showException(e);

      // We can continue, as we've already loaded the default theme
      // anyway. Here we restore the setting to its default value.
      pref.theme.selected(pref.theme.selected.defaultValue());
    }
  }
}

void SkinTheme::loadFontData()
{
  LOG("THEME: Loading fonts\n");

  std::string fontsFilename("fonts/fonts.xml");

  ResourceFinder rf;
  rf.includeDataDir(fontsFilename.c_str());
  if (!rf.findFirst())
    throw base::Exception("File %s not found", fontsFilename.c_str());

  XmlDocumentRef doc = open_xml(rf.filename());
  TiXmlHandle handle(doc.get());

  TiXmlElement* xmlFont = handle
    .FirstChild("fonts")
    .FirstChild("font").ToElement();
  while (xmlFont) {
    load_font(m_fonts, xmlFont, rf.filename());
    xmlFont = xmlFont->NextSiblingElement();
  }
}

void SkinTheme::loadAll(const std::string& themeId,
                        BackwardCompatibility* backward)
{
  LOG("THEME: Loading theme %s\n", themeId.c_str());

  if (m_fonts.empty())
    loadFontData();

  m_path = findThemePath(themeId);
  if (m_path.empty())
    throw base::Exception("Theme %s not found", themeId.c_str());

  loadSheet();
  loadXml(backward);
}

void SkinTheme::loadSheet()
{
  // Load the skin sheet
  std::string sheet_filename(base::join_path(m_path, "sheet.png"));
  os::SurfaceRef newSheet;
  try {
    newSheet = os::instance()->loadRgbaSurface(sheet_filename.c_str());
  }
  catch (...) {
    // Ignore the error, newSheet is nullptr and we will throw our own
    // exception.
  }
  if (!newSheet)
    throw base::Exception("Error loading %s file", sheet_filename.c_str());

  // TODO Change os::Surface::applyScale() to return a new surface,
  //      avoid loading two times the same file (even more, if there
  //      is no scale to apply, m_unscaledSheet must reference the
  //      same m_sheet).
  m_unscaledSheet = os::instance()->loadRgbaSurface(sheet_filename.c_str());

  // Replace the sprite sheet
  if (m_sheet)
    m_sheet.reset();
  m_sheet = newSheet;
  if (m_sheet)
    m_sheet->applyScale(guiscale());
  m_sheet->setImmutable();

  // Reset sprite sheet and font of all layer styles (to avoid
  // dangling pointers to os::Surface or os::Font).
  for (auto& it : m_styles) {
    for (auto& layer : it.second->layers()) {
      layer.setIcon(nullptr);
      layer.setSpriteSheet(nullptr);
    }
    it.second->setFont(nullptr);
  }
}

void SkinTheme::loadXml(BackwardCompatibility* backward)
{
  const int scale = guiscale();

  // Load the skin XML
  std::string xml_filename(base::join_path(m_path, "theme.xml"));

  XmlDocumentRef doc = open_xml(xml_filename);
  TiXmlHandle handle(doc.get());

  // Load Preferred scaling
  m_preferredScreenScaling = -1;
  m_preferredUIScaling = -1;
  {
    TiXmlElement* xmlTheme = handle
      .FirstChild("theme").ToElement();
    if (xmlTheme) {
      const char* screenScaling = xmlTheme->Attribute("screenscaling");
      const char* uiScaling = xmlTheme->Attribute("uiscaling");
      if (screenScaling)
        m_preferredScreenScaling = std::strtol(screenScaling, nullptr, 10);
      if (uiScaling)
        m_preferredUIScaling = std::strtol(uiScaling, nullptr, 10);
    }
  }

  // Load fonts
  {
    TiXmlElement* xmlFont = handle
      .FirstChild("theme")
      .FirstChild("fonts")
      .FirstChild("font").ToElement();
    while (xmlFont) {
      const char* idStr = xmlFont->Attribute("id");
      FontData* fontData = load_font(m_fonts, xmlFont, xml_filename);
      if (idStr && fontData) {
        std::string id(idStr);
        LOG(VERBOSE, "THEME: Loading theme font %s\n", idStr);

        int size = 10;
        const char* sizeStr = xmlFont->Attribute("size");
        if (sizeStr)
          size = std::strtol(sizeStr, nullptr, 10);

        const char* mnemonicsStr = xmlFont->Attribute("mnemonics");
        bool mnemonics = mnemonicsStr ? (std::string(mnemonicsStr) != "off") : true;

        os::FontRef font = fontData->getFont(size);
        m_themeFonts[idStr] = ThemeFont(font, mnemonics);

        // Store a unscaled version for using when ui scaling is not desired (i.e. in a Canvas widget with
        // autoScaling enabled).
        m_unscaledFonts[font.get()] = fontData->getFont(size, 1);

        if (id == "default")
          m_defaultFont = font;
        else if (id == "mini")
          m_miniFont = font;
      }

      xmlFont = xmlFont->NextSiblingElement();
    }
  }

  // No available font to run the program
  if (!m_defaultFont)
    throw base::Exception("There is no default font");
  if (!m_miniFont)
    m_miniFont = m_defaultFont;

  // Load dimension
  {
    TiXmlElement* xmlDim = handle
      .FirstChild("theme")
      .FirstChild("dimensions")
      .FirstChild("dim").ToElement();
    while (xmlDim) {
      std::string id = xmlDim->Attribute("id");
      uint32_t value = strtol(xmlDim->Attribute("value"), NULL, 10);

      LOG(VERBOSE, "THEME: Loading dimension %s\n", id.c_str());

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

      LOG(VERBOSE, "THEME: Loading color %s\n", id.c_str());

      m_colors_by_id[id] = color;
      xmlColor = xmlColor->NextSiblingElement();
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
      int x = scale*strtol(xmlPart->Attribute("x"), nullptr, 10);
      int y = scale*strtol(xmlPart->Attribute("y"), nullptr, 10);
      int w = (xmlPart->Attribute("w") ? scale*strtol(xmlPart->Attribute("w"), nullptr, 10): 0);
      int h = (xmlPart->Attribute("h") ? scale*strtol(xmlPart->Attribute("h"), nullptr, 10): 0);

      LOG(VERBOSE, "THEME: Loading part %s\n", part_id);

      SkinPartPtr part = m_parts_by_id[part_id];
      if (!part)
        part = m_parts_by_id[part_id] = SkinPartPtr(new SkinPart);

      SkinPartPtr unscaledPart = m_unscaledParts_by_id[part_id];
      if (!unscaledPart)
        unscaledPart = m_unscaledParts_by_id[part_id] = SkinPartPtr(new SkinPart);

      if (w > 0 && h > 0) {
        part->setSpriteBounds(gfx::Rect(x, y, w, h));
        part->setBitmap(0, sliceSheet(part->bitmapRef(0), gfx::Rect(x, y, w, h)));
        unscaledPart->setSpriteBounds(part->spriteBounds()/scale);
        unscaledPart->setBitmap(0, sliceUnscaledSheet(unscaledPart->bitmapRef(0), unscaledPart->spriteBounds()));
      }
      else if (xmlPart->Attribute("w1")) { // 3x3-1 part (NW, N, NE, E, SE, S, SW, W)
        int w1 = scale*strtol(xmlPart->Attribute("w1"), nullptr, 10);
        int w2 = scale*strtol(xmlPart->Attribute("w2"), nullptr, 10);
        int w3 = scale*strtol(xmlPart->Attribute("w3"), nullptr, 10);
        int h1 = scale*strtol(xmlPart->Attribute("h1"), nullptr, 10);
        int h2 = scale*strtol(xmlPart->Attribute("h2"), nullptr, 10);
        int h3 = scale*strtol(xmlPart->Attribute("h3"), nullptr, 10);

        part->setSpriteBounds(gfx::Rect(x, y, w1+w2+w3, h1+h2+h3));
        part->setSlicesBounds(gfx::Rect(w1, h1, w2, h2));

        part->setBitmap(0, sliceSheet(part->bitmapRef(0), gfx::Rect(x, y, w1, h1))); // NW
        part->setBitmap(1, sliceSheet(part->bitmapRef(1), gfx::Rect(x+w1, y, w2, h1))); // N
        part->setBitmap(2, sliceSheet(part->bitmapRef(2), gfx::Rect(x+w1+w2, y, w3, h1))); // NE
        part->setBitmap(3, sliceSheet(part->bitmapRef(3), gfx::Rect(x+w1+w2, y+h1, w3, h2))); // E
        part->setBitmap(4, sliceSheet(part->bitmapRef(4), gfx::Rect(x+w1+w2, y+h1+h2, w3, h3))); // SE
        part->setBitmap(5, sliceSheet(part->bitmapRef(5), gfx::Rect(x+w1, y+h1+h2, w2, h3))); // S
        part->setBitmap(6, sliceSheet(part->bitmapRef(6), gfx::Rect(x, y+h1+h2, w1, h3))); // SW
        part->setBitmap(7, sliceSheet(part->bitmapRef(7), gfx::Rect(x, y+h1, w1, h2))); // W

        unscaledPart->setSpriteBounds(part->spriteBounds()/scale);
        unscaledPart->setSlicesBounds(part->slicesBounds()/scale);

        unscaledPart->setBitmap(0, sliceUnscaledSheet(unscaledPart->bitmapRef(0), gfx::Rect(x, y, w1, h1)/scale));
        unscaledPart->setBitmap(1, sliceUnscaledSheet(unscaledPart->bitmapRef(1), gfx::Rect(x+w1, y, w2, h1)/scale));
        unscaledPart->setBitmap(2, sliceUnscaledSheet(unscaledPart->bitmapRef(2), gfx::Rect(x+w1+w2, y, w3, h1)/scale));
        unscaledPart->setBitmap(3, sliceUnscaledSheet(unscaledPart->bitmapRef(3), gfx::Rect(x+w1+w2, y+h1, w3, h2)/scale));
        unscaledPart->setBitmap(4, sliceUnscaledSheet(unscaledPart->bitmapRef(4), gfx::Rect(x+w1+w2, y+h1+h2, w3, h3)/scale));
        unscaledPart->setBitmap(5, sliceUnscaledSheet(unscaledPart->bitmapRef(5), gfx::Rect(x+w1, y+h1+h2, w2, h3)/scale));
        unscaledPart->setBitmap(6, sliceUnscaledSheet(unscaledPart->bitmapRef(6), gfx::Rect(x, y+h1+h2, w1, h3)/scale));
        unscaledPart->setBitmap(7, sliceUnscaledSheet(unscaledPart->bitmapRef(7), gfx::Rect(x, y+h1, w1, h2)/scale));
      }

      // Is it a mouse cursor?
      if (std::strncmp(part_id, "cursor_", 7) == 0) {
        std::string cursorName = std::string(part_id).substr(7);
        int focusx = scale*std::strtol(xmlPart->Attribute("focusx"), NULL, 10);
        int focusy = scale*std::strtol(xmlPart->Attribute("focusy"), NULL, 10);

        LOG(VERBOSE, "THEME: Loading cursor '%s'\n", cursorName.c_str());

        auto it = m_cursors.find(cursorName);
        if (it != m_cursors.end() && it->second != nullptr) {
          delete it->second;
          it->second = nullptr;
        }

        os::SurfaceRef slice = sliceSheet(nullptr, gfx::Rect(x, y, w, h));
        Cursor* cursor = new Cursor(slice, gfx::Point(focusx, focusy));
        m_cursors[cursorName] = cursor;

        for (int c=0; c<kCursorTypes; ++c) {
          if (cursorName == g_cursor_names[c]) {
            m_standardCursors[c] = cursor;
            break;
          }
        }
      }

      xmlPart = xmlPart->NextSiblingElement();
    }
  }

  // Load styles
  {
    TiXmlElement* xmlStyle = handle
      .FirstChild("theme")
      .FirstChild("styles")
      .FirstChild("style").ToElement();

    if (!xmlStyle)              // Without styles?
      throw base::Exception("There are no styles");

    if (backward) {
      backward->removeExistentStyles(xmlStyle);
      backward->copyMissingStyles(xmlStyle->Parent());
    }

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

      if (backward)
        backward->onStyle(xmlStyle);

      ui::Style* style = m_styles[style_id];
      if (!style) {
        m_styles[style_id] = style = new ui::Style(base);
      }
      else {
        *style = ui::Style(base);
      }
      style->setId(style_id);

      // Margin
      {
        const char* m = xmlStyle->Attribute("margin");
        const char* l = xmlStyle->Attribute("margin-left");
        const char* t = xmlStyle->Attribute("margin-top");
        const char* r = xmlStyle->Attribute("margin-right");
        const char* b = xmlStyle->Attribute("margin-bottom");
        gfx::Border margin = style->margin();
        if (m || l) margin.left(scale*std::strtol(l ? l: m, nullptr, 10));
        if (m || t) margin.top(scale*std::strtol(t ? t: m, nullptr, 10));
        if (m || r) margin.right(scale*std::strtol(r ? r: m, nullptr, 10));
        if (m || b) margin.bottom(scale*std::strtol(b ? b: m, nullptr, 10));
        style->setMargin(margin);
      }

      // Border
      {
        const char* m = xmlStyle->Attribute("border");
        const char* l = xmlStyle->Attribute("border-left");
        const char* t = xmlStyle->Attribute("border-top");
        const char* r = xmlStyle->Attribute("border-right");
        const char* b = xmlStyle->Attribute("border-bottom");
        gfx::Border border = style->border();
        if (m || l) border.left(scale*std::strtol(l ? l: m, nullptr, 10));
        if (m || t) border.top(scale*std::strtol(t ? t: m, nullptr, 10));
        if (m || r) border.right(scale*std::strtol(r ? r: m, nullptr, 10));
        if (m || b) border.bottom(scale*std::strtol(b ? b: m, nullptr, 10));
        style->setBorder(border);
      }

      // Padding
      {
        const char* m = xmlStyle->Attribute("padding");
        const char* l = xmlStyle->Attribute("padding-left");
        const char* t = xmlStyle->Attribute("padding-top");
        const char* r = xmlStyle->Attribute("padding-right");
        const char* b = xmlStyle->Attribute("padding-bottom");
        gfx::Border padding = style->padding();
        if (m || l) padding.left(scale*std::strtol(l ? l: m, nullptr, 10));
        if (m || t) padding.top(scale*std::strtol(t ? t: m, nullptr, 10));
        if (m || r) padding.right(scale*std::strtol(r ? r: m, nullptr, 10));
        if (m || b) padding.bottom(scale*std::strtol(b ? b: m, nullptr, 10));
        style->setPadding(padding);
      }

      // Size
      {
        const char* width     = xmlStyle->Attribute("width");
        const char* height    = xmlStyle->Attribute("height");
        const char* minwidth  = xmlStyle->Attribute("minwidth");
        const char* minheight = xmlStyle->Attribute("minheight");
        const char* maxwidth  = xmlStyle->Attribute("maxwidth");
        const char* maxheight = xmlStyle->Attribute("maxheight");
        gfx::Size minSize = style->minSize();
        gfx::Size maxSize = style->maxSize();
        if (width) {
          if (!minwidth) minwidth = width;
          if (!maxwidth) maxwidth = width;
        }
        if (height) {
          if (!minheight) minheight = height;
          if (!maxheight) maxheight = height;
        }
        if (minwidth) minSize.w = scale*std::strtol(minwidth, nullptr, 10);
        if (minheight) minSize.h = scale*std::strtol(minheight, nullptr, 10);
        if (maxwidth) maxSize.w = scale*std::strtol(maxwidth, nullptr, 10);
        if (maxheight) maxSize.h = scale*std::strtol(maxheight, nullptr, 10);
        style->setMinSize(minSize);
        style->setMaxSize(maxSize);
      }

      // Gap
      {
        const char* m = xmlStyle->Attribute("gap");
        const char* r = xmlStyle->Attribute("gap-rows");
        const char* c = xmlStyle->Attribute("gap-columns");
        gfx::Size gap = style->gap();
        if (m || c) gap.w = scale*std::strtol(c ? c: m, nullptr, 10);
        if (m || r) gap.h = scale*std::strtol(r ? r: m, nullptr, 10);
        style->setGap(gap);
      }

      // Font
      {
        const char* fontId = xmlStyle->Attribute("font");
        if (fontId) {
          auto themeFont = m_themeFonts[fontId];
          style->setFont(themeFont.font());
          style->setMnemonics(themeFont.mnemonics());
        }

        // Override mnemonics value if it is defined for this style.
        const char* mnemonicsStr = xmlStyle->Attribute("mnemonics");
        if (mnemonicsStr) {
          bool mnemonics = mnemonicsStr ? (std::string(mnemonicsStr) != "off") : true;
          style->setMnemonics(mnemonics);
        }
      }

      TiXmlElement* xmlLayer = xmlStyle->FirstChildElement();
      while (xmlLayer) {
        const std::string layerName = xmlLayer->Value();

        LOG(VERBOSE, "THEME: Layer %s for %s\n", layerName.c_str(), style_id);

        ui::Style::Layer layer;

        // Layer type
        if (layerName == "background") {
          layer.setType(ui::Style::Layer::Type::kBackground);
        }
        else if (layerName == "background-border") {
          layer.setType(ui::Style::Layer::Type::kBackgroundBorder);
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
          if (state.find("capture") != std::string::npos) flags |= ui::Style::Layer::kCapture;
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
          else if (std::strcmp(colorId, "none") == 0) {
            layer.setColor(gfx::ColorNone);
          }
          else {
            throw base::Exception("Color <%s color='%s' ...> was not found in '%s'\n",
                                  layerName.c_str(), colorId,
                                  xml_filename.c_str());
          }
        }

        // Offset
        const char* x = xmlLayer->Attribute("x");
        const char* y = xmlLayer->Attribute("y");
        if (x || y) {
          gfx::Point offset(0, 0);
          if (x) offset.x = std::strtol(x, nullptr, 10);
          if (y) offset.y = std::strtol(y, nullptr, 10);
          layer.setOffset(offset*scale);
        }

        // Sprite sheet
        const char* partId = xmlLayer->Attribute("part");
        if (partId) {
          auto it = m_parts_by_id.find(partId);
          if (it != m_parts_by_id.end()) {
            SkinPartPtr part = it->second;
            if (part) {
              if (layer.type() == ui::Style::Layer::Type::kIcon)
                layer.setIcon(AddRef(part->bitmap(0)));
              else {
                layer.setSpriteSheet(m_sheet);
                layer.setSpriteBounds(part->spriteBounds());
                layer.setSlicesBounds(part->slicesBounds());
              }
            }
          }
          else if (std::strcmp(partId, "none") == 0) {
            layer.setIcon(nullptr);
            layer.setSpriteSheet(nullptr);
            layer.setSpriteBounds(gfx::Rect(0, 0, 0, 0));
            layer.setSlicesBounds(gfx::Rect(0, 0, 0, 0));
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

static os::SurfaceRef sliceSheet(os::SurfaceRef sheet, os::SurfaceRef sur, const gfx::Rect& bounds)
{
  if (sur && (sur->width() != bounds.w ||
              sur->height() != bounds.h)) {
    sur = nullptr;
  }

  if (!bounds.isEmpty()) {
    if (!sur)
      sur = os::instance()->makeRgbaSurface(bounds.w, bounds.h);

    os::SurfaceLock lockSrc(sheet.get());
    os::SurfaceLock lockDst(sur.get());
    sheet->blitTo(sur.get(), bounds.x, bounds.y, 0, 0, bounds.w, bounds.h);

    // The new surface is immutable because we're going to re-use the
    // surface if we reload the theme.
    //
    // TODO Add sub-surfaces (SkBitmap::extractSubset())
    //sur->setImmutable();
  }
  else {
    ASSERT(!sur);
  }

  return sur;
}

os::SurfaceRef SkinTheme::sliceSheet(os::SurfaceRef sur, const gfx::Rect& bounds)
{
  return app::skin::sliceSheet(m_sheet, sur, bounds);
}

os::SurfaceRef SkinTheme::sliceUnscaledSheet(os::SurfaceRef sur, const gfx::Rect& bounds)
{
  return app::skin::sliceSheet(m_unscaledSheet, sur, bounds);
}

os::Font* SkinTheme::getWidgetFont(const Widget* widget) const
{
  auto skinPropery = std::static_pointer_cast<SkinProperty>(widget->getProperty(SkinProperty::Name));
  if (skinPropery && skinPropery->hasMiniFont())
    return getMiniFont();
  else
    return getDefaultFont();
}

Cursor* SkinTheme::getStandardCursor(CursorType type)
{
  if (type >= kFirstCursorType && type <= kLastCursorType)
    return m_standardCursors[type];
  else
    return nullptr;
}

void SkinTheme::initWidget(Widget* widget)
{
#define BORDER(n)                               \
  widget->setBorder(gfx::Border(n))

#define BORDER4(L,T,R,B)                                \
  widget->setBorder(gfx::Border((L), (T), (R), (B)))

  const int scale = guiscale();

  switch (widget->type()) {

    case kBoxWidget:
      widget->setStyle(styles.box());
      BORDER(0);
      widget->setChildSpacing(4 * scale);
      break;

    case kButtonWidget:
      widget->setStyle(styles.button());
      break;

    case kCheckWidget:
      widget->setStyle(styles.checkBox());
      break;

    case kRadioWidget:
      widget->setStyle(styles.radioButton());
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
      widget->setStyle(styles.grid());
      BORDER(0);
      widget->setChildSpacing(4 * scale);
      static_cast<ui::Grid *>(widget)->setGap(styles.grid()->gap());
      break;

    case kLabelWidget:
      widget->setStyle(styles.label());
      break;

    case kLinkLabelWidget:
      widget->setStyle(styles.link());
      break;

    case kListBoxWidget:
      BORDER(0);
      widget->setChildSpacing(0);
      break;

    case kListItemWidget:
      widget->setStyle(styles.listItem());
      break;

    case kComboBoxWidget: {
      ComboBox* combobox = static_cast<ComboBox*>(widget);
      Button* button = combobox->getButtonWidget();
      combobox->setChildSpacing(0);
      button->setStyle(styles.comboboxButton());
      break;
    }

    case kMenuWidget:
      widget->setStyle(styles.menu());
      break;

    case kMenuBarWidget:
      widget->setStyle(styles.menubar());
      break;

    case kMenuBoxWidget:
      widget->setStyle(styles.menubox());
      break;

    case kMenuItemWidget:
      BORDER(2 * scale);
      widget->setChildSpacing(18 * scale);
      break;

    case kSplitterWidget:
      widget->setChildSpacing(3 * scale);
      widget->setStyle(styles.splitter());
      break;

    case kSeparatorWidget:
      // Horizontal bar
      if (widget->align() & HORIZONTAL) {
        if (dynamic_cast<MenuSeparator*>(widget)) {
          widget->setStyle(styles.menuSeparator());
          BORDER(2 * scale);
        }
        else
          widget->setStyle(styles.horizontalSeparator());
      }
      // Vertical bar
      else {
        widget->setStyle(styles.verticalSeparator());
      }
      break;

    case kSliderWidget:
      widget->setStyle(styles.slider());
      break;

    case kTextBoxWidget:
      widget->setChildSpacing(0);
      widget->setStyle(styles.textboxText());
      break;

    case kViewWidget:
      widget->setChildSpacing(0);
      widget->setBgColor(colors.windowFace());
      widget->setStyle(styles.view());
      break;

    case kViewScrollbarWidget:
      widget->setStyle(styles.scrollbar());
      static_cast<ScrollBar*>(widget)->setThumbStyle(styles.scrollbarThumb());
      break;

    case kViewViewportWidget:
      BORDER(0);
      widget->setChildSpacing(0);
      break;

    case kManagerWidget:
      widget->setStyle(styles.desktop());
      break;

    case kWindowWidget:
      if (TipWindow* window = dynamic_cast<TipWindow*>(widget)) {
        window->setStyle(styles.tooltipWindow());
        window->setArrowStyle(styles.tooltipWindowArrow());
        window->textBox()->setStyle(styles.tooltipText());
      }
      else if (dynamic_cast<TransparentPopupWindow*>(widget)) {
        widget->setStyle(styles.transparentPopupWindow());
      }
      else if (dynamic_cast<PopupWindow*>(widget)) {
        widget->setStyle(styles.popupWindow());
      }
      else if (static_cast<Window*>(widget)->isDesktop()) {
        widget->setStyle(styles.desktop());
      }
      else {
        if (widget->hasText()) {
          widget->setStyle(styles.windowWithTitle());
        }
        else {
          widget->setStyle(styles.windowWithoutTitle());
        }
      }
      break;

    case kWindowTitleLabelWidget:
      widget->setStyle(styles.windowTitleLabel());
      break;

    case kWindowCloseButtonWidget:
      widget->setStyle(styles.windowCloseButton());
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
  if (widget->font()->type() == os::FontType::FreeType)
    return gfx::Size(2*guiscale(), widget->textHeight());
  else
    return gfx::Size(2*guiscale(), widget->textHeight()+2*guiscale());
}

void SkinTheme::paintEntry(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Entry* widget = static_cast<Entry*>(ev.getSource());
  gfx::Rect bounds = widget->clientBounds();

  // Outside borders
  g->fillRect(BGCOLOR, bounds);

  bool isMiniLook = false;
  auto skinPropery = std::static_pointer_cast<SkinProperty>(widget->getProperty(SkinProperty::Name));
  if (skinPropery)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  drawRect(g, bounds,
    (widget->hasFocus() ?
     (isMiniLook ? parts.sunkenMiniFocused().get(): parts.sunkenFocused().get()):
     (isMiniLook ? parts.sunkenMiniNormal().get() : parts.sunkenNormal().get())));

  drawEntryText(g, widget);
}

namespace {

class DrawEntryTextDelegate : public os::DrawTextDelegate {
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
    m_widget->getEntryThemeInfo(&m_index, &m_caret, &m_state, &m_range);
  }

  int index() const { return m_index; }
  bool caretDrawn() const { return m_caretDrawn; }
  const gfx::Rect& textBounds() const { return m_textBounds; }

  void preProcessChar(const int index,
                      const int codepoint,
                      gfx::Color& fg,
                      gfx::Color& bg,
                      const gfx::Rect& charBounds) override {
    auto theme = SkinTheme::get(m_widget);

    // Normal text
    auto& colors = theme->colors;
    bg = ColorNone;
    fg = colors.text();

    // Selected
    if ((m_index >= m_range.from) &&
        (m_index < m_range.to)) {
      if (m_widget->hasFocus())
        bg = colors.selected();
      else
        bg = colors.disabled();
      fg = colors.selectedText();
    }

    // Disabled
    if (!m_widget->isEnabled()) {
      bg = ColorNone;
      fg = colors.disabled();
    }

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
      auto theme = SkinTheme::get(m_widget);
      theme->drawEntryCaret(
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
  Entry::Range m_range;
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
  base::utf8_decode dec(textString);
  auto pos = dec.pos();
  for (int i=0; i<scroll && dec.next(); ++i)
    pos = dec.pos();

  // TODO use a string_view()
  g->drawText(std::string(pos, textString.end()),
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
        widget, sufBounds, widget->align(), 0);
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
    os::Surface* icon =
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
  drawText(g, nullptr, fg, ColorNone, widget, pos,
           widget->align(), widget->mnemonic());

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
        drawText(g, buf.c_str(), fg, ColorNone, widget, pos, widget->align(), 0);
        widget->setAlign(old_align);
      }
    }
  }
}

void SkinTheme::paintSlider(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Slider* widget = static_cast<Slider*>(ev.getSource());
  const Rect bounds = widget->clientBounds();
  int min, max, value;

  // Outside borders
  gfx::Color bgcolor = widget->bgColor();
  if (!is_transparent(bgcolor))
    g->fillRect(bgcolor, bounds);

  widget->getSliderThemeInfo(&min, &max, &value);

  Rect rc = bounds;
  rc.shrink(widget->border());
  int x;
  if (min != max)
    x = rc.x + rc.w * (value-min) / (max-min);
  else
    x = rc.x;

  rc = bounds;

  // The mini-look is used for sliders with tiny borders.
  bool isMiniLook = false;

  // The BG painter is used for sliders without a number-indicator and
  // customized background (e.g. RGB sliders)
  ISliderBgPainter* bgPainter = NULL;

  const auto skinPropery = std::static_pointer_cast<SkinProperty>(widget->getProperty(SkinProperty::Name));
  if (skinPropery)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  const auto skinSliderPropery = std::static_pointer_cast<SkinSliderProperty>(widget->getProperty(SkinSliderProperty::Name));
  if (skinSliderPropery)
    bgPainter = skinSliderPropery->getBgPainter();

  // Draw customized background
  if (bgPainter) {
    SkinPartPtr nw = parts.miniSliderEmpty();
    os::Surface* thumb =
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

    gfx::Rect textrc;
    int textAlign;
    calcTextInfo(widget, widget->style(), bounds, textrc, textAlign);

    {
      IntersectClip clip(g, Rect(rc.x, rc.y, x-rc.x+1, rc.h));
      if (clip) {
        drawText(g, nullptr,
                 colors.sliderFullText(), ColorNone,
                 widget, textrc, textAlign, widget->mnemonic());
      }
    }

    {
      IntersectClip clip(g, Rect(x+1, rc.y, rc.w-(x-rc.x+1), rc.h));
      if (clip) {
        drawText(g, nullptr,
                 colors.sliderEmptyText(),
                 ColorNone, widget, textrc, textAlign, widget->mnemonic());
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
  ui::Style* style = styles.combobox();

  paintWidget(g, widget, style, bounds);
  drawEntryText(g, widget);
}

void SkinTheme::paintTextBox(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());

  Theme::paintTextBoxWithStyle(g, widget);
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

  if (!is_transparent(c) ||
      widget->type() == kWindowWidget)
    return (widget->isTransparent() ? gfx::ColorNone: c);
  else if (decorative)
    return colors.selected();
  else
    return colors.face();
}

void SkinTheme::drawText(Graphics* g, const char* t,
                         const gfx::Color fgColor,
                         const gfx::Color bgColor,
                         const Widget* widget,
                         const Rect& rc,
                         const int textAlign,
                         const int mnemonic)
{
  if (t || widget->hasText()) {
    Rect textrc;

    g->setFont(AddRef(widget->font()));

    if (!t)
      t = widget->text().c_str();

    textrc.setSize(g->measureUIText(t));

    // Horizontally text alignment

    if (textAlign & RIGHT)
      textrc.x = rc.x + rc.w - textrc.w - 1;
    else if (textAlign & CENTER)
      textrc.x = rc.center().x - textrc.w/2;
    else
      textrc.x = rc.x;

    // Vertically text alignment

    if (textAlign & BOTTOM)
      textrc.y = rc.y + rc.h - textrc.h - 1;
    else if (textAlign & MIDDLE)
      textrc.y = rc.center().y - textrc.h/2;
    else
      textrc.y = rc.y;

    // Background
    if (!is_transparent(bgColor)) {
      if (!widget->isEnabled())
        g->fillRect(bgColor, Rect(textrc).inflate(guiscale(), guiscale()));
      else
        g->fillRect(bgColor, textrc);
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
         (gfx::geta(fgColor) > 0 ? fgColor :
                                   colors.text())),
        bgColor, textrc.origin(),
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

SkinPartPtr SkinTheme::getToolPart(const char* toolId) const
{
  return getPartById(std::string("tool_") + toolId);
}

os::Surface* SkinTheme::getToolIcon(const char* toolId) const
{
  SkinPartPtr part = getToolPart(toolId);
  if (part)
    return part->bitmap(0);
  else
    return nullptr;
}

void SkinTheme::drawRect(Graphics* g, const Rect& rc,
                         os::Surface* nw, os::Surface* n, os::Surface* ne,
                         os::Surface* e, os::Surface* se, os::Surface* s,
                         os::Surface* sw, os::Surface* w)
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
  Theme::drawSlices(g, m_sheet.get(), rc,
                    skinPart->spriteBounds(),
                    skinPart->slicesBounds(),
                    gfx::ColorNone,
                    drawCenter);
}

void SkinTheme::drawRectUsingUnscaledSheet(ui::Graphics* g, const gfx::Rect& rc,
                                           SkinPart* skinPart, const bool drawCenter)
{
  Theme::drawSlices(g, m_unscaledSheet.get(), rc,
                    skinPart->spriteBounds(),
                    skinPart->slicesBounds(),
                    gfx::ColorNone,
                    drawCenter);
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
  gfx::Color border = colors.text();
  border = gfx::rgba(gfx::getr(border),
                     gfx::getg(border),
                     gfx::getb(border), 64);
  g->drawRect(border, rc0);

  gfx::Rect rc = rc0;
  rc.shrink(1);

  int u = (int)((double)rc.w*progress);
  u = std::clamp(u, 0, rc.w);

  if (u > 0)
    g->fillRect(colors.selected(), gfx::Rect(rc.x, rc.y, u, rc.h));

  if (1+u < rc.w)
    g->fillRect(colors.background(), gfx::Rect(rc.x+u, rc.y, rc.w-u, rc.h));
}

std::string SkinTheme::findThemePath(const std::string& themeId) const
{
  // First we try to find the theme on an extensions
  std::string path = App::instance()->extensions().themePath(themeId);
  if (path.empty()) {
    // Then we try a theme in the old themes/ folder
    path = base::join_path(SkinTheme::kThemesFolderName, themeId);
    path = base::join_path(path, "theme.xml");

    ResourceFinder rf;
    rf.includeDataDir(path.c_str());
    if (!rf.findFirst())
      return std::string();

    path = base::get_file_path(rf.filename());
  }
  return base::normalize_path(path);
}

} // namespace skin
} // namespace app
