// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include "she/font.h"
#include "she/scoped_surface_lock.h"
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

const char* SkinTheme::kThemeCloseButtonId = "theme_close_button";

// Controls the "X" button in a window to close it.
class WindowCloseButton : public Button {
public:
  WindowCloseButton() : Button("") {
    setup_bevels(this, 0, 0, 0, 0);
    setDecorative(true);
    setId(SkinTheme::kThemeCloseButtonId);
  }

protected:
  void onClick(Event& ev) override {
    Button::onClick(ev);
    closeWindow();
  }

  void onPaint(PaintEvent& ev) override {
    static_cast<SkinTheme*>(getTheme())->paintWindowButton(ev);
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kSetCursorMessage:
        ui::set_mouse_cursor(kArrowCursor);
        return true;

      case kKeyDownMessage:
        if (getRoot()->isForeground() &&
            static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
          setSelected(true);
          return true;
        }
        break;

      case kKeyUpMessage:
        if (getRoot()->isForeground() &&
            static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
          if (isSelected()) {
            setSelected(false);
            closeWindow();
            return true;
          }
        }
        break;
    }

    return Button::onProcessMessage(msg);
  }
};

static const char* cursor_names[kCursorTypes] = {
  "null",                       // kNoCursor
  "normal",                     // kArrowCursor
  "normal_add",                 // kArrowPlusCursor
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

// static
SkinTheme* SkinTheme::instance()
{
  return static_cast<SkinTheme*>(ui::Manager::getDefault()->getTheme());
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
  if (pref.theme.selected.defaultValue() != pref.theme.selected())
    loadAll(pref.theme.selected());
}

void SkinTheme::loadAll(const std::string& skinId)
{
  loadSheet(skinId);
  loadFonts(skinId);
  loadXml(skinId);
}

void SkinTheme::loadSheet(const std::string& skinId)
{
  TRACE("SkinTheme::loadSheet(%s)\n", skinId.c_str());

  if (m_sheet) {
    m_sheet->dispose();
    m_sheet = NULL;
  }

  // Load the skin sheet
  std::string sheet_filename("skins/" + skinId + "/sheet.png");

  ResourceFinder rf;
  rf.includeDataDir(sheet_filename.c_str());
  if (!rf.findFirst())
    throw base::Exception("File %s not found", sheet_filename.c_str());

  try {
    m_sheet = she::instance()->loadRgbaSurface(rf.filename().c_str());
  }
  catch (...) {
    throw base::Exception("Error loading %s file", sheet_filename.c_str());
  }
}

void SkinTheme::loadFonts(const std::string& skinId)
{
  TRACE("SkinTheme::loadFonts(%s)\n", skinId.c_str());

  if (m_defaultFont) m_defaultFont->dispose();
  if (m_miniFont) m_miniFont->dispose();

  Preferences& pref = Preferences::instance();

  m_defaultFont = loadFont(pref.theme.font(), "skins/" + skinId + "/font.png");
  m_miniFont = loadFont(pref.theme.miniFont(), "skins/" + skinId + "/minifont.png");
}

void SkinTheme::loadXml(const std::string& skinId)
{
  TRACE("SkinTheme::loadXml(%s)\n", skinId.c_str());

  // Load the skin XML
  std::string xml_filename = "skins/" + skinId + "/skin.xml";
  ResourceFinder rf;
  rf.includeDataDir(xml_filename.c_str());
  if (!rf.findFirst())
    return;

  XmlDocumentRef doc = open_xml(rf.filename());
  TiXmlHandle handle(doc.get());

  // Load dimension
  {
    TiXmlElement* xmlDim = handle
      .FirstChild("skin")
      .FirstChild("dimensions")
      .FirstChild("dim").ToElement();
    while (xmlDim) {
      std::string id = xmlDim->Attribute("id");
      uint32_t value = strtol(xmlDim->Attribute("value"), NULL, 10);

      LOG("Loading dimension '%s'...\n", id.c_str());

      m_dimensions_by_id[id] = value;
      xmlDim = xmlDim->NextSiblingElement();
    }
  }

  // Load colors
  {
    TiXmlElement* xmlColor = handle
      .FirstChild("skin")
      .FirstChild("colors")
      .FirstChild("color").ToElement();
    while (xmlColor) {
      std::string id = xmlColor->Attribute("id");
      uint32_t value = strtol(xmlColor->Attribute("value")+1, NULL, 16);
      gfx::Color color = gfx::rgba(
        (value & 0xff0000) >> 16,
        (value & 0xff00) >> 8,
        (value & 0xff));

      LOG("Loading color '%s'...\n", id.c_str());

      m_colors_by_id[id] = color;
      xmlColor = xmlColor->NextSiblingElement();
    }
  }

  // Load cursors
  {
    TiXmlElement* xmlCursor = handle
      .FirstChild("skin")
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

      LOG("Loading cursor '%s'...\n", id.c_str());

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
      .FirstChild("skin")
      .FirstChild("tools")
      .FirstChild("tool").ToElement();
    while (xmlIcon) {
      // Get the tool-icon rectangle
      const char* tool_id = xmlIcon->Attribute("id");
      int x = strtol(xmlIcon->Attribute("x"), NULL, 10);
      int y = strtol(xmlIcon->Attribute("y"), NULL, 10);
      int w = strtol(xmlIcon->Attribute("w"), NULL, 10);
      int h = strtol(xmlIcon->Attribute("h"), NULL, 10);

      LOG("Loading tool icon '%s'...\n", tool_id);

      // Crop the tool-icon from the sheet
      m_toolicon[tool_id] = sliceSheet(
        m_toolicon[tool_id], gfx::Rect(x, y, w, h));

      xmlIcon = xmlIcon->NextSiblingElement();
    }
  }

  // Load parts
  {
    TiXmlElement* xmlPart = handle
      .FirstChild("skin")
      .FirstChild("parts")
      .FirstChild("part").ToElement();
    while (xmlPart) {
      // Get the tool-icon rectangle
      const char* part_id = xmlPart->Attribute("id");
      int x = strtol(xmlPart->Attribute("x"), NULL, 10);
      int y = strtol(xmlPart->Attribute("y"), NULL, 10);
      int w = xmlPart->Attribute("w") ? strtol(xmlPart->Attribute("w"), NULL, 10): 0;
      int h = xmlPart->Attribute("h") ? strtol(xmlPart->Attribute("h"), NULL, 10): 0;

      LOG("Loading part '%s'...\n", part_id);

      SkinPartPtr part = m_parts_by_id[part_id];
      if (!part)
        part = m_parts_by_id[part_id] = SkinPartPtr(new SkinPart);

      if (w > 0 && h > 0) {
        part->setBitmap(0,
          sliceSheet(part->getBitmap(0), gfx::Rect(x, y, w, h)));
      }
      else if (xmlPart->Attribute("w1")) { // 3x3-1 part (NW, N, NE, E, SE, S, SW, W)
        int w1 = strtol(xmlPart->Attribute("w1"), NULL, 10);
        int w2 = strtol(xmlPart->Attribute("w2"), NULL, 10);
        int w3 = strtol(xmlPart->Attribute("w3"), NULL, 10);
        int h1 = strtol(xmlPart->Attribute("h1"), NULL, 10);
        int h2 = strtol(xmlPart->Attribute("h2"), NULL, 10);
        int h3 = strtol(xmlPart->Attribute("h3"), NULL, 10);

        part->setBitmap(0, sliceSheet(part->getBitmap(0), gfx::Rect(x, y, w1, h1))); // NW
        part->setBitmap(1, sliceSheet(part->getBitmap(1), gfx::Rect(x+w1, y, w2, h1))); // N
        part->setBitmap(2, sliceSheet(part->getBitmap(2), gfx::Rect(x+w1+w2, y, w3, h1))); // NE
        part->setBitmap(3, sliceSheet(part->getBitmap(3), gfx::Rect(x+w1+w2, y+h1, w3, h2))); // E
        part->setBitmap(4, sliceSheet(part->getBitmap(4), gfx::Rect(x+w1+w2, y+h1+h2, w3, h3))); // SE
        part->setBitmap(5, sliceSheet(part->getBitmap(5), gfx::Rect(x+w1, y+h1+h2, w2, h3))); // S
        part->setBitmap(6, sliceSheet(part->getBitmap(6), gfx::Rect(x, y+h1+h2, w1, h3))); // SW
        part->setBitmap(7, sliceSheet(part->getBitmap(7), gfx::Rect(x, y+h1, w1, h2))); // W
      }

      xmlPart = xmlPart->NextSiblingElement();
    }
  }

  // Load styles
  {
    TiXmlElement* xmlStyle = handle
      .FirstChild("skin")
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

        LOG("- Rule '%s' for '%s'\n", ruleName.c_str(), style_id);

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

          if (color_id) (*style)[StyleSheet::backgroundColorRule()] = css::Value(color_id);
          if (part_id) (*style)[StyleSheet::backgroundPartRule()] = css::Value(part_id);
          if (repeat_id) (*style)[StyleSheet::backgroundRepeatRule()] = css::Value(repeat_id);
        }
        else if (ruleName == "icon") {
          if (align) (*style)[StyleSheet::iconAlignRule()] = css::Value(align);
          if (part_id) (*style)[StyleSheet::iconPartRule()] = css::Value(part_id);

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

  SkinFile<SkinTheme>::updateInternals();
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
    she::ScopedSurfaceLock src(m_sheet);
    she::ScopedSurfaceLock dst(sur);
    src->blitTo(dst, bounds.x, bounds.y, 0, 0, bounds.w, bounds.h);
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
      BORDER4(
        parts.buttonNormal()->getBitmapW()->width(),
        parts.buttonNormal()->getBitmapN()->height(),
        parts.buttonNormal()->getBitmapE()->width(),
        parts.buttonNormal()->getBitmapS()->height());
      widget->setChildSpacing(0);
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
        parts.sunkenNormal()->getBitmapW()->width(),
        parts.sunkenNormal()->getBitmapN()->height(),
        parts.sunkenNormal()->getBitmapE()->width(),
        parts.sunkenNormal()->getBitmapS()->height());
      break;

    case kGridWidget:
      BORDER(0);
      widget->setChildSpacing(4 * scale);
      break;

    case kLabelWidget:
      BORDER(1 * scale);
      break;

    case kListBoxWidget:
      BORDER(0);
      widget->setChildSpacing(0);
      break;

    case kListItemWidget:
      BORDER(1 * scale);
      break;

    case kComboBoxWidget:
      {
        ComboBox* combobox = dynamic_cast<ComboBox*>(widget);
        ASSERT(combobox != NULL);

        Button* button = combobox->getButtonWidget();

        button->setBorder(gfx::Border(0));
        button->setChildSpacing(0);
        button->setMinSize(gfx::Size(15 * guiscale(),
                                     16 * guiscale()));

        static_cast<ButtonBase*>(button)->setIconInterface
          (new ButtonIconImpl(parts.comboboxArrowDown(),
                              parts.comboboxArrowDownSelected(),
                              parts.comboboxArrowDownDisabled(),
                              CENTER | MIDDLE));
      }
      break;

    case kMenuWidget:
    case kMenuBarWidget:
    case kMenuBoxWidget:
      BORDER(0);
      widget->setChildSpacing(0);
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
      if ((widget->getAlign() & HORIZONTAL) &&
          (widget->getAlign() & VERTICAL)) {
        BORDER(4 * scale);
      }
      // Horizontal bar
      else if (widget->getAlign() & HORIZONTAL) {
        BORDER4(2 * scale, 4 * scale, 2 * scale, 0);
      }
      // Vertical bar
      else {
        BORDER4(4 * scale, 2 * scale, 1 * scale, 2 * scale);
      }

      if (widget->hasText()) {
        gfx::Border border = widget->border();

        if (widget->getAlign() & TOP)
          border.top(widget->getTextHeight());
        else if (widget->getAlign() & BOTTOM)
          border.bottom(widget->getTextHeight());

        widget->setBorder(border);
      }
      break;

    case kSliderWidget:
      BORDER4(
        parts.sliderEmpty()->getBitmapW()->width()-1*scale,
        parts.sliderEmpty()->getBitmapN()->height(),
        parts.sliderEmpty()->getBitmapE()->width()-1*scale,
        parts.sliderEmpty()->getBitmapS()->height()-1*scale);
      widget->setChildSpacing(widget->getTextHeight());
      widget->setAlign(CENTER | MIDDLE);
      break;

    case kTextBoxWidget:
      BORDER(0);
      widget->setChildSpacing(0);
      break;

    case kViewWidget:
      BORDER4(
        parts.sunkenNormal()->getBitmapW()->width()-1*scale,
        parts.sunkenNormal()->getBitmapN()->height(),
        parts.sunkenNormal()->getBitmapE()->width()-1*scale,
        parts.sunkenNormal()->getBitmapS()->height()-1*scale);
      widget->setChildSpacing(0);
      widget->setBgColor(colors.windowFace());
      break;

    case kViewScrollbarWidget:
      BORDER(1 * scale);
      widget->setChildSpacing(0);
      break;

    case kViewViewportWidget:
      BORDER(0);
      widget->setChildSpacing(0);
      break;

    case kWindowWidget:
      if (!static_cast<Window*>(widget)->isDesktop()) {
        if (widget->hasText()) {
          BORDER4(6 * scale,
                  (4+6) * scale + widget->getTextHeight(),
                  6 * scale,
                  6 * scale);

          if (!widget->hasFlags(INITIALIZED)) {
            Button* button = new WindowCloseButton();
            widget->addChild(button);
          }
        }
        else {
          BORDER(3 * scale);
        }
      }
      else {
        BORDER(0);
      }
      widget->setChildSpacing(4 * scale); // TODO this hard-coded 4 should be configurable in skin.xml
      widget->setBgColor(colors.windowFace());
      break;

    default:
      break;
  }
}

void SkinTheme::getWindowMask(Widget* widget, Region& region)
{
  region = widget->getBounds();
}

void SkinTheme::setDecorativeWidgetBounds(Widget* widget)
{
  if (widget->getId() == kThemeCloseButtonId) {
    Widget* window = widget->getParent();
    gfx::Rect rect(parts.windowCloseButtonNormal()->getSize());

    rect.offset(window->getBounds().x2() - 3*guiscale() - rect.w,
                window->getBounds().y + 3*guiscale());

    widget->setBounds(rect);
  }
}

int SkinTheme::getScrollbarSize()
{
  return dimensions.scrollbarSize();
}

void SkinTheme::paintDesktop(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();

  g->fillRect(colors.disabled(), g->getClipBounds());
}

void SkinTheme::paintBox(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  if (!is_transparent(BGCOLOR))
    g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintButton(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  IButtonIcon* iconInterface = widget->getIconInterface();
  gfx::Rect box, text, icon;
  gfx::Color fg, bg;
  SkinPartPtr part_nw;

  widget->getTextIconInfo(&box, &text, &icon,
    iconInterface ? iconInterface->getIconAlign(): 0,
    iconInterface ? iconInterface->getSize().w: 0,
    iconInterface ? iconInterface->getSize().h: 0);

  // Tool buttons are smaller
  LookType look = NormalLook;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery)
    look = skinPropery->getLook();

  // Selected
  if (widget->isSelected()) {
    fg = colors.buttonSelectedText();
    bg = colors.buttonSelectedFace();
    part_nw = (look == MiniLook ? parts.toolbuttonNormal():
               look == LeftButtonLook ? parts.dropDownButtonLeftSelected():
               look == RightButtonLook ? parts.dropDownButtonRightSelected():
                                         parts.buttonSelected());
  }
  // With mouse
  else if (widget->isEnabled() && widget->hasMouseOver()) {
    fg = colors.buttonHotText();
    bg = colors.buttonHotFace();
    part_nw = (look == MiniLook ? parts.toolbuttonHot():
               look == LeftButtonLook ? parts.dropDownButtonLeftHot():
               look == RightButtonLook ? parts.dropDownButtonRightHot():
                                         parts.buttonHot());
  }
  // Without mouse
  else {
    fg = colors.buttonNormalText();
    bg = colors.buttonNormalFace();

    if (widget->hasFocus())
      part_nw = (look == MiniLook ? parts.toolbuttonHot():
                 look == LeftButtonLook ? parts.dropDownButtonLeftFocused():
                 look == RightButtonLook ? parts.dropDownButtonRightFocused():
                                           parts.buttonFocused());
    else
      part_nw = (look == MiniLook ? parts.toolbuttonNormal():
                 look == LeftButtonLook ? parts.dropDownButtonLeftNormal():
                 look == RightButtonLook ? parts.dropDownButtonRightNormal():
                                           parts.buttonNormal());
  }

  // external background
  g->fillRect(BGCOLOR, g->getClipBounds());

  // draw borders
  if (part_nw)
    drawRect(g, widget->getClientBounds(), part_nw.get(), bg);

  // text
  drawTextString(g, NULL, fg, ColorNone, widget,
                 widget->getClientChildrenBounds(), get_button_selected_offset());

  // Paint the icon
  if (iconInterface) {
    if (widget->isSelected())
      icon.offset(get_button_selected_offset(),
                  get_button_selected_offset());

    paintIcon(widget, ev.getGraphics(), iconInterface, icon.x, icon.y);
  }
}

void SkinTheme::paintCheckBox(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  IButtonIcon* iconInterface = widget->getIconInterface();
  gfx::Rect box, text, icon;
  gfx::Color bg;

  widget->getTextIconInfo(&box, &text, &icon,
    iconInterface ? iconInterface->getIconAlign(): 0,
    iconInterface ? iconInterface->getSize().w: 0,
    iconInterface ? iconInterface->getSize().h: 0);

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
  drawTextString(g, NULL, ColorNone, ColorNone, widget, text, 0);

  // Paint the icon
  if (iconInterface)
    paintIcon(widget, g, iconInterface, icon.x, icon.y);

  // draw focus
  if (look != WithoutBordersLook && widget->hasFocus())
    drawRect(g, bounds, parts.checkFocus().get(), gfx::ColorNone);
}

void SkinTheme::paintGrid(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  if (!is_transparent(BGCOLOR))
    g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintEntry(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Entry* widget = static_cast<Entry*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  bool password = widget->isPassword();
  int scroll, caret, state, selbeg, selend;
  const std::string& textString = widget->getText();
  int c, ch, x, y, w;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  // Outside borders
  g->fillRect(BGCOLOR, bounds);

  bool isMiniLook = false;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  gfx::Color bg = colors.background();
  drawRect(g, bounds,
    (widget->hasFocus() ?
     (isMiniLook ? parts.sunkenMiniFocused().get(): parts.sunkenFocused().get()):
     (isMiniLook ? parts.sunkenMiniNormal().get() : parts.sunkenNormal().get())),
    bg);

  // Draw the text
  x = bounds.x + widget->border().left();
  y = bounds.y + bounds.h/2 - widget->getTextHeight()/2;

  base::utf8_const_iterator utf8_it = base::utf8_const_iterator(textString.begin());
  int textlen = base::utf8_length(textString);
  if (scroll < textlen)
    utf8_it += scroll;

  for (c=scroll; c<textlen; ++c, ++utf8_it) {
    ch = password ? '*': *utf8_it;

    // Normal text
    bg = ColorNone;
    gfx::Color fg = colors.text();

    // Selected
    if ((c >= selbeg) && (c <= selend)) {
      if (widget->hasFocus())
        bg = colors.selected();
      else
        bg = colors.disabled();
      fg = colors.background();
    }

    // Disabled
    if (!widget->isEnabled()) {
      bg = ColorNone;
      fg = colors.disabled();
    }

    w = g->measureChar(ch).w;
    if (x+w > bounds.x2()-3)
      return;

    caret_x = x;
    g->drawChar(ch, fg, bg, x, y);
    x += w;

    // Caret
    if ((c == caret) && (state) && (widget->hasFocus()))
      drawEntryCaret(g, widget, caret_x, y);
  }

  // Draw suffix if there is enough space
  if (!widget->getSuffix().empty()) {
    Rect sufBounds(x, y,
                   bounds.x2()-3*guiscale()-x,
                   widget->getTextHeight());
    IntersectClip clip(g, sufBounds);
    if (clip) {
      drawTextString(
        g, widget->getSuffix().c_str(),
        colors.entrySuffix(), ColorNone,
        widget, sufBounds, 0);
    }
  }

  // Draw the caret if it is next of the last character
  if ((c == caret) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled())) {
    drawEntryCaret(g, widget, x, y);
  }
}

void SkinTheme::paintLabel(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Label* widget = static_cast<Label*>(ev.getSource());
  Style* style = styles.label();
  gfx::Color bg = BGCOLOR;
  Rect text, rc = widget->getClientBounds();

  SkinStylePropertyPtr styleProp = widget->getProperty(SkinStyleProperty::Name);
  if (styleProp)
    style = styleProp->getStyle();

  if (!is_transparent(bg))
    g->fillRect(bg, rc);

  rc.shrink(widget->border());

  widget->getTextIconInfo(NULL, &text);
  style->paint(g, text, widget->getText().c_str(), Style::State());
}

void SkinTheme::paintLinkLabel(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Style* style = styles.link();
  gfx::Rect bounds = widget->getClientBounds();
  gfx::Color bg = BGCOLOR;

  SkinStylePropertyPtr styleProp = widget->getProperty(SkinStyleProperty::Name);
  if (styleProp)
    style = styleProp->getStyle();

  Style::State state;
  if (widget->hasMouseOver()) state += Style::hover();
  if (widget->isSelected()) state += Style::clicked();

  g->fillRect(bg, bounds);
  style->paint(g, bounds, widget->getText().c_str(), state);
}

void SkinTheme::paintListBox(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();

  g->fillRect(colors.background(), g->getClipBounds());
}

void SkinTheme::paintListItem(ui::PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  Graphics* g = ev.getGraphics();
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
    drawTextString(g, NULL, fg, bg, widget, bounds, 0);
  }
}

void SkinTheme::paintMenu(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintMenuItem(ui::PaintEvent& ev)
{
  int scale = guiscale();
  Graphics* g = ev.getGraphics();
  MenuItem* widget = static_cast<MenuItem*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  gfx::Color fg, bg;
  int c, bar;

  // TODO ASSERT?
  if (!widget->getParent()->getParent())
    return;

  bar = (widget->getParent()->getParent()->type() == kMenuBarWidget);

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
       parts.checkSelected()->getBitmap(0):
       parts.checkDisabled()->getBitmap(0));

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
  drawTextString(g, NULL, fg, ColorNone, widget, pos, 0);

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
      if (appMenuItem->getKey() && !appMenuItem->getKey()->accels().empty()) {
        int old_align = appMenuItem->getAlign();

        pos = bounds;
        pos.w -= widget->childSpacing()/4;

        std::string buf = appMenuItem->getKey()->accels().front().toString();

        widget->setAlign(RIGHT | MIDDLE);
        drawTextString(g, buf.c_str(), fg, ColorNone, widget, pos, 0);
        widget->setAlign(old_align);
      }
    }
  }
}

void SkinTheme::paintSplitter(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();

  g->fillRect(colors.splitterNormalFace(), g->getClipBounds());
}

void SkinTheme::paintRadioButton(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  IButtonIcon* iconInterface = widget->getIconInterface();
  gfx::Color bg = BGCOLOR;

  gfx::Rect box, text, icon;
  widget->getTextIconInfo(&box, &text, &icon,
    iconInterface ? iconInterface->getIconAlign(): 0,
    iconInterface ? iconInterface->getSize().w: 0,
    iconInterface ? iconInterface->getSize().h: 0);

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
  drawTextString(g, NULL, ColorNone, ColorNone, widget, text, 0);

  // Icon
  if (iconInterface)
    paintIcon(widget, g, iconInterface, icon.x, icon.y);

  // Focus
  if (widget->hasFocus())
    drawRect(g, bounds, parts.radioFocus().get(), gfx::ColorNone);
}

void SkinTheme::paintSeparator(ui::PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();

  // background
  g->fillRect(BGCOLOR, bounds);

  if (widget->getAlign() & HORIZONTAL)
    drawHline(g, bounds, parts.separatorHorz().get());

  if (widget->getAlign() & VERTICAL)
    drawVline(g, bounds, parts.separatorVert().get());

  // text
  if (widget->hasText()) {
    int h = widget->getTextHeight();
    Rect r(
      Point(
        bounds.x + widget->border().left()/2 + h/2,
        bounds.y + widget->border().top()/2 - h/2),
      Point(
        bounds.x2() - widget->border().right()/2 - h,
        bounds.y2() - widget->border().bottom()/2 + h));

    drawTextString(g, NULL,
      colors.separatorLabel(), BGCOLOR,
      widget, r, 0);
  }
}

void SkinTheme::paintSlider(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Slider* widget = static_cast<Slider*>(ev.getSource());
  Rect bounds = widget->getClientBounds();
  int min, max, value;

  // Outside borders
  gfx::Color bgcolor = widget->getBgColor();
  if (!is_transparent(bgcolor))
    g->fillRect(bgcolor, bounds);

  widget->getSliderThemeInfo(&min, &max, &value);

  Rect rc(Rect(bounds).shrink(widget->border()));
  int x;
  if (min != max)
    x = rc.x + rc.w * (value-min) / (max-min);
  else
    x = rc.x;

  rc = widget->getClientBounds();

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
      (widget->hasFocus() ? parts.miniSliderThumbFocused()->getBitmap(0):
                            parts.miniSliderThumb()->getBitmap(0));

    // Draw background
    g->fillRect(BGCOLOR, rc);

    // Draw thumb
    g->drawRgbaSurface(thumb, x-thumb->width()/2, rc.y);

    // Draw borders
    rc.shrink(Border(
        3 * guiscale(),
        thumb->height(),
        3 * guiscale(),
        1 * guiscale()));

    drawRect(g, rc, nw.get(), gfx::ColorNone);

    // Draw background (using the customized ISliderBgPainter implementation)
    rc.shrink(Border(1, 1, 1, 2) * guiscale());
    if (!rc.isEmpty())
      bgPainter->paint(widget, g, rc);
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
      drawRect(g, rc, empty_part.get(), colors.sliderEmptyFace());
    else if (value == max)
      drawRect(g, rc, full_part.get(), colors.sliderFullFace());
    else
      drawRect2(g, rc, x,
                full_part.get(), empty_part.get(),
                colors.sliderFullFace(),
                colors.sliderEmptyFace());

    // Draw text
    std::string old_text = widget->getText();
    widget->setTextQuiet(widget->convertValueToText(value));

    {
      IntersectClip clip(g, Rect(rc.x, rc.y, x-rc.x, rc.h));
      if (clip) {
        drawTextString(g, NULL,
          colors.sliderFullText(), ColorNone,
          widget, rc, 0);
      }
    }

    {
      IntersectClip clip(g, Rect(x+1, rc.y, rc.w-(x-rc.x+1), rc.h));
      if (clip) {
        drawTextString(g, NULL,
          colors.sliderEmptyText(),
          ColorNone, widget, rc, 0);
      }
    }

    widget->setTextQuiet(old_text.c_str());
  }
}

void SkinTheme::paintComboBoxEntry(ui::PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Entry* widget = static_cast<Entry*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  bool password = widget->isPassword();
  int scroll, caret, state, selbeg, selend;
  const std::string& textString = widget->getText();
  int c, ch, x, y, w;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  // Outside borders
  g->fillRect(BGCOLOR, bounds);

  gfx::Color fg, bg = colors.background();

  drawRect(g, bounds,
           (widget->hasFocus() ?
            parts.sunken2Focused().get():
            parts.sunken2Normal().get()), bg);

  // Draw the text
  x = bounds.x + widget->border().left();
  y = bounds.y + bounds.h/2 - widget->getTextHeight()/2;

  base::utf8_const_iterator utf8_it = base::utf8_const_iterator(textString.begin());
  int textlen = base::utf8_length(textString);

  if (scroll < textlen)
    utf8_it += scroll;

  for (c=scroll; c<textlen; ++c, ++utf8_it) {
    ch = password ? '*': *utf8_it;

    // Normal text
    bg = ColorNone;
    fg = colors.text();

    // Selected
    if ((c >= selbeg) && (c <= selend)) {
      if (widget->hasFocus())
        bg = colors.selected();
      else
        bg = colors.disabled();
      fg = colors.background();
    }

    // Disabled
    if (!widget->isEnabled()) {
      bg = ColorNone;
      fg = colors.disabled();
    }

    w = g->measureChar(ch).w;
    if (x+w > bounds.x2()-3)
      return;

    caret_x = x;
    g->drawChar(ch, fg, bg, x, y);
    x += w;

    // Caret
    if ((c == caret) && (state) && (widget->hasFocus()))
      drawEntryCaret(g, widget, caret_x, y);
  }

  // Draw the caret if it is next of the last character
  if ((c == caret) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled())) {
    drawEntryCaret(g, widget, x, y);
  }
}

void SkinTheme::paintComboBoxButton(PaintEvent& ev)
{
  Button* widget = static_cast<Button*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  IButtonIcon* iconInterface = widget->getIconInterface();
  SkinPartPtr part_nw;
  gfx::Color bg;

  if (widget->isSelected()) {
    bg = colors.buttonSelectedFace();
    part_nw = parts.toolbuttonPushed();
  }
  // With mouse
  else if (widget->isEnabled() && widget->hasMouseOver()) {
    bg = colors.buttonHotFace();
    part_nw = parts.toolbuttonHot();
  }
  // Without mouse
  else {
    bg = colors.buttonNormalFace();
    part_nw = parts.toolbuttonLast();
  }

  Rect rc = widget->getClientBounds();

  // external background
  g->fillRect(BGCOLOR, rc);

  // draw borders
  drawRect(g, rc, part_nw.get(), bg);

  // Paint the icon
  if (iconInterface) {
    // Icon
    int x = rc.x + rc.w/2 - iconInterface->getSize().w/2;
    int y = rc.y + rc.h/2 - iconInterface->getSize().h/2;

    paintIcon(widget, ev.getGraphics(), iconInterface, x, y);
  }
}

void SkinTheme::paintTextBox(ui::PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());

  drawTextBox(g, widget, NULL, NULL,
    colors.textboxFace(),
    colors.textboxText());
}

void SkinTheme::paintView(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  View* widget = static_cast<View*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  gfx::Color bg = BGCOLOR;
  Style* style = styles.view();

  SkinStylePropertyPtr styleProp = widget->getProperty(SkinStyleProperty::Name);
  if (styleProp)
    style = styleProp->getStyle();

  Style::State state;
  if (widget->hasMouseOver()) state += Style::hover();

  if (!is_transparent(bg))
    g->fillRect(bg, bounds);

  style->paint(g, bounds, nullptr, state);
}

void SkinTheme::paintViewScrollbar(PaintEvent& ev)
{
  ScrollBar* widget = static_cast<ScrollBar*>(ev.getSource());
  Graphics* g = ev.getGraphics();
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

  gfx::Rect rc = widget->getClientBounds();
  bgStyle->paint(g, rc, NULL, state);

  // Horizontal bar
  if (widget->getAlign() & HORIZONTAL) {
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
  Graphics* g = ev.getGraphics();
  gfx::Color bg = BGCOLOR;

  if (!is_transparent(bg))
    g->fillRect(bg, widget->getClientBounds());
}

void SkinTheme::paintWindow(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Window* window = static_cast<Window*>(ev.getSource());
  Rect pos = window->getClientBounds();
  Rect cpos = window->getClientChildrenBounds();

  if (!window->isDesktop()) {
    // window frame
    if (window->hasText()) {
      styles.window()->paint(g, pos, NULL, Style::State());
      styles.windowTitle()->paint(g,
        gfx::Rect(cpos.x, pos.y+5*guiscale(), cpos.w, // TODO this hard-coded 5 should be configurable in skin.xml
          window->getTextHeight()),
        window->getText().c_str(), Style::State());
    }
    // menubox
    else {
      styles.menubox()->paint(g, pos, NULL, Style::State());
    }
  }
  // desktop
  else {
    styles.desktop()->paint(g, pos, NULL, Style::State());
  }
}

void SkinTheme::paintPopupWindow(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Window* window = static_cast<Window*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  gfx::Rect pos = window->getClientBounds();

  if (!is_transparent(BGCOLOR))
    styles.menubox()->paint(g, pos, NULL, Style::State());

  pos.shrink(window->border());

  g->drawAlignedUIString(window->getText(),
    colors.text(),
    window->getBgColor(), pos,
    window->getAlign());
}

void SkinTheme::paintWindowButton(ui::PaintEvent& ev)
{
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  Rect rc = widget->getClientBounds();
  SkinPartPtr part;

  if (widget->isSelected())
    part = parts.windowCloseButtonSelected();
  else if (widget->hasMouseOver())
    part = parts.windowCloseButtonHot();
  else
    part = parts.windowCloseButtonNormal();

  g->drawRgbaSurface(part->getBitmap(0), rc.x, rc.y);
}

void SkinTheme::paintTooltip(PaintEvent& ev)
{
  ui::TipWindow* widget = static_cast<ui::TipWindow*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  Rect absRc = widget->getBounds();
  Rect rc = widget->getClientBounds();
  gfx::Color fg = colors.tooltipText();
  gfx::Color bg = colors.tooltipFace();
  SkinPartPtr tooltipPart = parts.tooltip();

  she::Surface* nw = tooltipPart->getBitmapNW();
  she::Surface* n  = tooltipPart->getBitmapN();
  she::Surface* ne = tooltipPart->getBitmapNE();
  she::Surface* e  = tooltipPart->getBitmapE();
  she::Surface* se = tooltipPart->getBitmapSE();
  she::Surface* s  = tooltipPart->getBitmapS();
  she::Surface* sw = tooltipPart->getBitmapSW();
  she::Surface* w  = tooltipPart->getBitmapW();

  switch (widget->getArrowAlign()) {
    case TOP | LEFT:     nw = parts.tooltipArrow()->getBitmapNW(); break;
    case TOP | RIGHT:    ne = parts.tooltipArrow()->getBitmapNE(); break;
    case BOTTOM | LEFT:  sw = parts.tooltipArrow()->getBitmapSW(); break;
    case BOTTOM | RIGHT: se = parts.tooltipArrow()->getBitmapSE(); break;
  }

  drawRect(g, rc, nw, n, ne, e, se, s, sw, w);

  // Draw arrow in sides
  she::Surface* arrow = NULL;
  gfx::Rect target(widget->target());
  target = target.createIntersection(gfx::Rect(0, 0, ui::display_w(), ui::display_h()));
  target.offset(-absRc.getOrigin());

  switch (widget->getArrowAlign()) {
    case TOP:
      arrow = parts.tooltipArrow()->getBitmapN();
      g->drawRgbaSurface(arrow,
                         target.x+target.w/2-arrow->width()/2,
                         rc.y);
      break;
    case BOTTOM:
      arrow = parts.tooltipArrow()->getBitmapS();
      g->drawRgbaSurface(arrow,
                         target.x+target.w/2-arrow->width()/2,
                         rc.y+rc.h-arrow->height());
      break;
    case LEFT:
      arrow = parts.tooltipArrow()->getBitmapW();
      g->drawRgbaSurface(arrow,
                         rc.x,
                         target.y+target.h/2-arrow->height()/2);
      break;
    case RIGHT:
      arrow = parts.tooltipArrow()->getBitmapE();
      g->drawRgbaSurface(arrow,
                         rc.x+rc.w-arrow->width(),
                         target.y+target.h/2-arrow->height()/2);
      break;
  }

  // Fill background
  g->fillRect(
    bg, Rect(rc).shrink(
      Border(
        w->width(),
        n->height(),
        e->width(),
        s->height())));

  rc.shrink(widget->border());

  g->drawAlignedUIString(widget->getText(), fg, bg, rc, widget->getAlign());
}

gfx::Color SkinTheme::getWidgetBgColor(Widget* widget)
{
  gfx::Color c = widget->getBgColor();
  bool decorative = widget->isDecorative();

  if (!is_transparent(c) || widget->type() == kWindowWidget)
    return c;
  else if (decorative)
    return colors.selected();
  else
    return colors.face();
}

void SkinTheme::drawTextString(Graphics* g, const char *t, gfx::Color fg_color, gfx::Color bg_color,
                               Widget* widget, const Rect& rc,
                               int selected_offset)
{
  if (t || widget->hasText()) {
    Rect textrc;

    g->setFont(widget->getFont());

    if (!t)
      t = widget->getText().c_str();

    textrc.setSize(g->measureUIString(t));

    // Horizontally text alignment

    if (widget->getAlign() & RIGHT)
      textrc.x = rc.x + rc.w - textrc.w - 1;
    else if (widget->getAlign() & CENTER)
      textrc.x = rc.getCenter().x - textrc.w/2;
    else
      textrc.x = rc.x;

    // Vertically text alignment

    if (widget->getAlign() & BOTTOM)
      textrc.y = rc.y + rc.h - textrc.h - 1;
    else if (widget->getAlign() & MIDDLE)
      textrc.y = rc.getCenter().y - textrc.h/2;
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
      // Rect(widget->getClientBounds()).shrink(widget->border()));
      widget->getClientBounds()).inflate(0, 1*guiscale());

    IntersectClip clip(g, textWrap);
    if (clip) {
      if (!widget->isEnabled()) {
        // Draw white part
        g->drawUIString(t,
          colors.background(),
          gfx::ColorNone,
          textrc.getOrigin() + Point(guiscale(), guiscale()));
      }

      g->drawUIString(t,
        (!widget->isEnabled() ?
          colors.disabled():
          (gfx::geta(fg_color) > 0 ? fg_color :
            colors.text())),
        bg_color, textrc.getOrigin());
    }
  }
}

void SkinTheme::drawEntryCaret(ui::Graphics* g, Entry* widget, int x, int y)
{
  gfx::Color color = colors.text();
  int h = widget->getTextHeight();

  for (int u=x; u<x+2*guiscale(); ++u)
    g->drawVLine(color, u, y-1, h+2);
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

void SkinTheme::drawRect(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart, gfx::Color bg)
{
  drawRect(g, rc,
    skinPart->getBitmap(0),
    skinPart->getBitmap(1),
    skinPart->getBitmap(2),
    skinPart->getBitmap(3),
    skinPart->getBitmap(4),
    skinPart->getBitmap(5),
    skinPart->getBitmap(6),
    skinPart->getBitmap(7));

  // Center
  if (!is_transparent(bg)) {
    gfx::Rect inside = rc;
    inside.shrink(Border(
        skinPart->getBitmap(7)->width(),
        skinPart->getBitmap(1)->height(),
        skinPart->getBitmap(3)->width(),
        skinPart->getBitmap(5)->height()));

    IntersectClip clip(g, inside);
    if (clip)
      g->fillRect(bg, inside);
  }
}

void SkinTheme::drawRect2(Graphics* g, const Rect& rc, int x_mid,
                          SkinPart* nw1, SkinPart* nw2,
                          gfx::Color bg1, gfx::Color bg2)
{
  Rect rc2(rc.x, rc.y, x_mid-rc.x+1, rc.h);
  {
    IntersectClip clip(g, rc2);
    if (clip)
      drawRect(g, rc, nw1, bg1);
  }

  rc2.x += rc2.w;
  rc2.w = rc.w - rc2.w;

  IntersectClip clip(g, rc2);
  if (clip)
    drawRect(g, rc, nw2, bg2);
}

void SkinTheme::drawHline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* part)
{
  int x;

  for (x = rc.x;
       x < rc.x2()-part->getSize().w;
       x += part->getSize().w) {
    g->drawRgbaSurface(part->getBitmap(0), x, rc.y);
  }

  if (x < rc.x2()) {
    Rect rc2(x, rc.y, rc.w-(x-rc.x), part->getSize().h);
    IntersectClip clip(g, rc2);
    if (clip)
      g->drawRgbaSurface(part->getBitmap(0), x, rc.y);
  }
}

void SkinTheme::drawVline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* part)
{
  int y;

  for (y = rc.y;
       y < rc.y2()-part->getSize().h;
       y += part->getSize().h) {
    g->drawRgbaSurface(part->getBitmap(0), rc.x, y);
  }

  if (y < rc.y2()) {
    Rect rc2(rc.x, y, part->getSize().w, rc.h-(y-rc.y));
    IntersectClip clip(g, rc2);
    if (clip)
      g->drawRgbaSurface(part->getBitmap(0), rc.x, y);
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
      icon_bmp = iconInterface->getSelectedIcon();
    else
      icon_bmp = iconInterface->getNormalIcon();
  }
  // disabled
  else {
    icon_bmp = iconInterface->getDisabledIcon();
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
      she::Font* f = she::instance()->loadBitmapFont(rf.filename().c_str(), guiscale());
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

} // namespace skin
} // namespace app
