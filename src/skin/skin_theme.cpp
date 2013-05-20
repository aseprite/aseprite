/* ASEPRITE
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

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "base/bind.h"
#include "base/shared_ptr.h"
#include "gfx/border.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "loadpng.h"
#include "modules/gui.h"
#include "resource_finder.h"
#include "skin/button_icon_impl.h"
#include "skin/skin_property.h"
#include "skin/skin_slider_property.h"
#include "skin/skin_theme.h"
#include "she/system.h"
#include "ui/gui.h"
#include "ui/intern.h"
#include "xml_exception.h"

#include "tinyxml.h"

using namespace gfx;
using namespace ui;

#define CHARACTER_LENGTH(f, c)  ((f)->vtable->char_length((f), (c)))

#define BGCOLOR                 (getWidgetBgColor(widget))

static std::map<std::string, int> sheet_mapping;
static std::map<std::string, ThemeColor::Type> color_mapping;

const char* SkinTheme::kThemeCloseButtonId = "theme_close_button";

// Controls the "X" button in a window to close it.
class WindowCloseButton : public Button
{
public:
  WindowCloseButton() : Button("") {
    setup_bevels(this, 0, 0, 0, 0);
    setDecorative(true);
    setId(SkinTheme::kThemeCloseButtonId);
  }

protected:
  void onClick(Event& ev) OVERRIDE {
    Button::onClick(ev);
    closeWindow();
  }

  void onPaint(PaintEvent& ev) OVERRIDE {
    static_cast<SkinTheme*>(getTheme())->paintWindowButton(ev);
  }

  bool onProcessMessage(Message* msg) OVERRIDE {
    switch (msg->type) {

      case kSetCursorMessage:
        jmouse_set_cursor(kArrowCursor);
        return true;

      case kKeyDownMessage:
        if (msg->key.scancode == KEY_ESC) {
          setSelected(true);
          return true;
        }
        break;

      case kKeyUpMessage:
        if (msg->key.scancode == KEY_ESC) {
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
  "size_tl",                    // kSizeTLCursor
  "size_t",                     // kSizeTCursor
  "size_tr",                    // kSizeTRCursor
  "size_l",                     // kSizeLCursor
  "size_r",                     // kSizeRCursor
  "size_bl",                    // kSizeBLCursor
  "size_b",                     // kSizeBCursor
  "size_br",                    // kSizeBRCursor
  "rotate_tl",                  // kRotateTLCursor
  "rotate_t",                   // kRotateTCursor
  "rotate_tr",                  // kRotateTRCursor
  "rotate_l",                   // kRotateLCursor
  "rotate_r",                   // kRotateRCursor
  "rotate_bl",                  // kRotateBLCursor
  "rotate_b",                   // kRotateBCursor
  "rotate_br",                  // kRotateBRCursor
  "eyedropper"                  // kEyedropperCursor
};

SkinTheme::SkinTheme()
  : m_cursors(ui::kCursorTypes, NULL)
  , m_colors(ThemeColor::MaxColors)
{
  this->name = "Skin Theme";
  m_selected_skin = get_config_string("Skin", "Selected", "default");
  m_minifont = font;

  // Initialize all graphics in NULL (these bitmaps are loaded from the skin)
  m_sheet_bmp = NULL;
  for (int c=0; c<PARTS; ++c)
    m_part[c] = NULL;

  sheet_mapping["radio_normal"] = PART_RADIO_NORMAL;
  sheet_mapping["radio_selected"] = PART_RADIO_SELECTED;
  sheet_mapping["radio_disabled"] = PART_RADIO_DISABLED;
  sheet_mapping["check_normal"] = PART_CHECK_NORMAL;
  sheet_mapping["check_selected"] = PART_CHECK_SELECTED;
  sheet_mapping["check_disabled"] = PART_CHECK_DISABLED;
  sheet_mapping["check_focus"] = PART_CHECK_FOCUS_NW;
  sheet_mapping["radio_focus"] = PART_RADIO_FOCUS_NW;
  sheet_mapping["button_normal"] = PART_BUTTON_NORMAL_NW;
  sheet_mapping["button_hot"] = PART_BUTTON_HOT_NW;
  sheet_mapping["button_focused"] = PART_BUTTON_FOCUSED_NW;
  sheet_mapping["button_selected"] = PART_BUTTON_SELECTED_NW;
  sheet_mapping["sunken_normal"] = PART_SUNKEN_NORMAL_NW;
  sheet_mapping["sunken_focused"] = PART_SUNKEN_FOCUSED_NW;
  sheet_mapping["sunken2_normal"] = PART_SUNKEN2_NORMAL_NW;
  sheet_mapping["sunken2_focused"] = PART_SUNKEN2_FOCUSED_NW;
  sheet_mapping["sunken_mini_normal"] = PART_SUNKEN_MINI_NORMAL_NW;
  sheet_mapping["sunken_mini_focused"] = PART_SUNKEN_MINI_FOCUSED_NW;
  sheet_mapping["window"] = PART_WINDOW_NW;
  sheet_mapping["menu"] = PART_MENU_NW;
  sheet_mapping["window_close_button_normal"] = PART_WINDOW_CLOSE_BUTTON_NORMAL;
  sheet_mapping["window_close_button_hot"] = PART_WINDOW_CLOSE_BUTTON_HOT;
  sheet_mapping["window_close_button_selected"] = PART_WINDOW_CLOSE_BUTTON_SELECTED;
  sheet_mapping["window_play_button_normal"] = PART_WINDOW_PLAY_BUTTON_NORMAL;
  sheet_mapping["window_play_button_hot"] = PART_WINDOW_PLAY_BUTTON_HOT;
  sheet_mapping["window_play_button_selected"] = PART_WINDOW_PLAY_BUTTON_SELECTED;
  sheet_mapping["window_stop_button_normal"] = PART_WINDOW_STOP_BUTTON_NORMAL;
  sheet_mapping["window_stop_button_hot"] = PART_WINDOW_STOP_BUTTON_HOT;
  sheet_mapping["window_stop_button_selected"] = PART_WINDOW_STOP_BUTTON_SELECTED;
  sheet_mapping["slider_full"] = PART_SLIDER_FULL_NW;
  sheet_mapping["slider_empty"] = PART_SLIDER_EMPTY_NW;
  sheet_mapping["slider_full_focused"] = PART_SLIDER_FULL_FOCUSED_NW;
  sheet_mapping["slider_empty_focused"] = PART_SLIDER_EMPTY_FOCUSED_NW;
  sheet_mapping["mini_slider_full"] = PART_MINI_SLIDER_FULL_NW;
  sheet_mapping["mini_slider_empty"] = PART_MINI_SLIDER_EMPTY_NW;
  sheet_mapping["mini_slider_full_focused"] = PART_MINI_SLIDER_FULL_FOCUSED_NW;
  sheet_mapping["mini_slider_empty_focused"] = PART_MINI_SLIDER_EMPTY_FOCUSED_NW;
  sheet_mapping["mini_slider_thumb"] = PART_MINI_SLIDER_THUMB;
  sheet_mapping["mini_slider_thumb_focused"] = PART_MINI_SLIDER_THUMB_FOCUSED;
  sheet_mapping["separator_horz"] = PART_SEPARATOR_HORZ;
  sheet_mapping["separator_vert"] = PART_SEPARATOR_VERT;
  sheet_mapping["combobox_arrow_down"] = PART_COMBOBOX_ARROW_DOWN;
  sheet_mapping["combobox_arrow_down_selected"] = PART_COMBOBOX_ARROW_DOWN_SELECTED;
  sheet_mapping["combobox_arrow_down_disabled"] = PART_COMBOBOX_ARROW_DOWN_DISABLED;
  sheet_mapping["combobox_arrow_up"] = PART_COMBOBOX_ARROW_UP;
  sheet_mapping["combobox_arrow_up_selected"] = PART_COMBOBOX_ARROW_UP_SELECTED;
  sheet_mapping["combobox_arrow_up_disabled"] = PART_COMBOBOX_ARROW_UP_DISABLED;
  sheet_mapping["combobox_arrow_left"] = PART_COMBOBOX_ARROW_LEFT;
  sheet_mapping["combobox_arrow_left_selected"] = PART_COMBOBOX_ARROW_LEFT_SELECTED;
  sheet_mapping["combobox_arrow_left_disabled"] = PART_COMBOBOX_ARROW_LEFT_DISABLED;
  sheet_mapping["combobox_arrow_right"] = PART_COMBOBOX_ARROW_RIGHT;
  sheet_mapping["combobox_arrow_right_selected"] = PART_COMBOBOX_ARROW_RIGHT_SELECTED;
  sheet_mapping["combobox_arrow_right_disabled"] = PART_COMBOBOX_ARROW_RIGHT_DISABLED;
  sheet_mapping["toolbutton_normal"] = PART_TOOLBUTTON_NORMAL_NW;
  sheet_mapping["toolbutton_hot"] = PART_TOOLBUTTON_HOT_NW;
  sheet_mapping["toolbutton_last"] = PART_TOOLBUTTON_LAST_NW;
  sheet_mapping["toolbutton_pushed"] = PART_TOOLBUTTON_PUSHED_NW;
  sheet_mapping["tab_normal"] = PART_TAB_NORMAL_NW;
  sheet_mapping["tab_selected"] = PART_TAB_SELECTED_NW;
  sheet_mapping["tab_bottom_selected"] = PART_TAB_BOTTOM_SELECTED_NW;
  sheet_mapping["tab_bottom_normal"] = PART_TAB_BOTTOM_NORMAL;
  sheet_mapping["tab_filler"] = PART_TAB_FILLER;
  sheet_mapping["editor_normal"] = PART_EDITOR_NORMAL_NW;
  sheet_mapping["editor_selected"] = PART_EDITOR_SELECTED_NW;
  sheet_mapping["colorbar_0"] = PART_COLORBAR_0_NW;
  sheet_mapping["colorbar_1"] = PART_COLORBAR_1_NW;
  sheet_mapping["colorbar_2"] = PART_COLORBAR_2_NW;
  sheet_mapping["colorbar_3"] = PART_COLORBAR_3_NW;
  sheet_mapping["colorbar_border_fg"] = PART_COLORBAR_BORDER_FG_NW;
  sheet_mapping["colorbar_border_bg"] = PART_COLORBAR_BORDER_BG_NW;
  sheet_mapping["colorbar_border_hotfg"] = PART_COLORBAR_BORDER_HOTFG_NW;
  sheet_mapping["scrollbar_bg"] = PART_SCROLLBAR_BG_NW;
  sheet_mapping["scrollbar_thumb"] = PART_SCROLLBAR_THUMB_NW;
  sheet_mapping["tooltip"] = PART_TOOLTIP_NW;
  sheet_mapping["tooltip_arrow"] = PART_TOOLTIP_ARROW_NW;
  sheet_mapping["ani_first"] = PART_ANI_FIRST;
  sheet_mapping["ani_first_selected"] = PART_ANI_FIRST_SELECTED;
  sheet_mapping["ani_first_disabled"] = PART_ANI_FIRST_DISABLED;
  sheet_mapping["ani_previous"] = PART_ANI_PREVIOUS;
  sheet_mapping["ani_previous_selected"] = PART_ANI_PREVIOUS_SELECTED;
  sheet_mapping["ani_previous_disabled"] = PART_ANI_PREVIOUS_DISABLED;
  sheet_mapping["ani_play"] = PART_ANI_PLAY;
  sheet_mapping["ani_play_selected"] = PART_ANI_PLAY_SELECTED;
  sheet_mapping["ani_play_disabled"] = PART_ANI_PLAY_DISABLED;
  sheet_mapping["ani_next"] = PART_ANI_NEXT;
  sheet_mapping["ani_next_selected"] = PART_ANI_NEXT_SELECTED;
  sheet_mapping["ani_next_disabled"] = PART_ANI_NEXT_DISABLED;
  sheet_mapping["ani_last"] = PART_ANI_LAST;
  sheet_mapping["ani_last_selected"] = PART_ANI_LAST_SELECTED;
  sheet_mapping["ani_last_disabled"] = PART_ANI_LAST_DISABLED;
  sheet_mapping["target_one"] = PART_TARGET_ONE;
  sheet_mapping["target_one_selected"] = PART_TARGET_ONE_SELECTED;
  sheet_mapping["target_frames"] = PART_TARGET_FRAMES;
  sheet_mapping["target_frames_selected"] = PART_TARGET_FRAMES_SELECTED;
  sheet_mapping["target_layers"] = PART_TARGET_LAYERS;
  sheet_mapping["target_layers_selected"] = PART_TARGET_LAYERS_SELECTED;
  sheet_mapping["target_frames_layers"] = PART_TARGET_FRAMES_LAYERS;
  sheet_mapping["target_frames_layers_selected"] = PART_TARGET_FRAMES_LAYERS_SELECTED;
  sheet_mapping["brush_circle"] = PART_BRUSH_CIRCLE;
  sheet_mapping["brush_circle_selected"] = PART_BRUSH_CIRCLE_SELECTED;
  sheet_mapping["brush_square"] = PART_BRUSH_SQUARE;
  sheet_mapping["brush_square_selected"] = PART_BRUSH_SQUARE_SELECTED;
  sheet_mapping["brush_line"] = PART_BRUSH_LINE;
  sheet_mapping["brush_line_selected"] = PART_BRUSH_LINE_SELECTED;
  sheet_mapping["scale_arrow_1"] = PART_SCALE_ARROW_1;
  sheet_mapping["scale_arrow_2"] = PART_SCALE_ARROW_2;
  sheet_mapping["scale_arrow_3"] = PART_SCALE_ARROW_3;
  sheet_mapping["rotate_arrow_1"] = PART_ROTATE_ARROW_1;
  sheet_mapping["rotate_arrow_2"] = PART_ROTATE_ARROW_2;
  sheet_mapping["rotate_arrow_3"] = PART_ROTATE_ARROW_3;
  sheet_mapping["layer_visible"] = PART_LAYER_VISIBLE;
  sheet_mapping["layer_visible_selected"] = PART_LAYER_VISIBLE_SELECTED;
  sheet_mapping["layer_hidden"] = PART_LAYER_HIDDEN;
  sheet_mapping["layer_hidden_selected"] = PART_LAYER_HIDDEN_SELECTED;
  sheet_mapping["layer_editable"] = PART_LAYER_EDITABLE;
  sheet_mapping["layer_editable_selected"] = PART_LAYER_EDITABLE_SELECTED;
  sheet_mapping["layer_locked"] = PART_LAYER_LOCKED;
  sheet_mapping["layer_locked_selected"] = PART_LAYER_LOCKED_SELECTED;
  sheet_mapping["unpinned"] = PART_UNPINNED;
  sheet_mapping["pinned"] = PART_PINNED;
  sheet_mapping["drop_down_button_left_normal"] = PART_DROP_DOWN_BUTTON_LEFT_NORMAL_NW;
  sheet_mapping["drop_down_button_left_hot"] = PART_DROP_DOWN_BUTTON_LEFT_HOT_NW;
  sheet_mapping["drop_down_button_left_focused"] = PART_DROP_DOWN_BUTTON_LEFT_FOCUSED_NW;
  sheet_mapping["drop_down_button_left_selected"] = PART_DROP_DOWN_BUTTON_LEFT_SELECTED_NW;
  sheet_mapping["drop_down_button_right_normal"] = PART_DROP_DOWN_BUTTON_RIGHT_NORMAL_NW;
  sheet_mapping["drop_down_button_right_hot"] = PART_DROP_DOWN_BUTTON_RIGHT_HOT_NW;
  sheet_mapping["drop_down_button_right_focused"] = PART_DROP_DOWN_BUTTON_RIGHT_FOCUSED_NW;
  sheet_mapping["drop_down_button_right_selected"] = PART_DROP_DOWN_BUTTON_RIGHT_SELECTED_NW;
  sheet_mapping["transformation_handle"] = PART_TRANSFORMATION_HANDLE;
  sheet_mapping["pivot_handle"] = PART_PIVOT_HANDLE;

  color_mapping["text"] = ThemeColor::Text;
  color_mapping["disabled"] = ThemeColor::Disabled;
  color_mapping["face"] = ThemeColor::Face;
  color_mapping["hot_face"] = ThemeColor::HotFace;
  color_mapping["selected"] = ThemeColor::Selected;
  color_mapping["background"] = ThemeColor::Background;
  color_mapping["desktop"] = ThemeColor::Desktop;
  color_mapping["textbox_text"] = ThemeColor::TextBoxText;
  color_mapping["textbox_face"] = ThemeColor::TextBoxFace;
  color_mapping["entry_suffix"] = ThemeColor::EntrySuffix;
  color_mapping["link_text"] = ThemeColor::LinkText;
  color_mapping["button_normal_text"] = ThemeColor::ButtonNormalText;
  color_mapping["button_normal_face"] = ThemeColor::ButtonNormalFace;
  color_mapping["button_hot_text"] = ThemeColor::ButtonHotText;
  color_mapping["button_hot_face"] = ThemeColor::ButtonHotFace;
  color_mapping["button_selected_text"] = ThemeColor::ButtonSelectedText;
  color_mapping["button_selected_face"] = ThemeColor::ButtonSelectedFace;
  color_mapping["check_hot_face"] = ThemeColor::CheckHotFace;
  color_mapping["check_focus_face"] = ThemeColor::CheckFocusFace;
  color_mapping["radio_hot_face"] = ThemeColor::RadioHotFace;
  color_mapping["radio_focus_face"] = ThemeColor::RadioFocusFace;
  color_mapping["menuitem_normal_text"] = ThemeColor::MenuItemNormalText;
  color_mapping["menuitem_normal_face"] = ThemeColor::MenuItemNormalFace;
  color_mapping["menuitem_hot_text"] = ThemeColor::MenuItemHotText;
  color_mapping["menuitem_hot_face"] = ThemeColor::MenuItemHotFace;
  color_mapping["menuitem_highlight_text"] = ThemeColor::MenuItemHighlightText;
  color_mapping["menuitem_highlight_face"] = ThemeColor::MenuItemHighlightFace;
  color_mapping["window_face"] = ThemeColor::WindowFace;
  color_mapping["window_titlebar_text"] = ThemeColor::WindowTitlebarText;
  color_mapping["window_titlebar_face"] = ThemeColor::WindowTitlebarFace;
  color_mapping["editor_face"] = ThemeColor::EditorFace;
  color_mapping["editor_sprite_border"] = ThemeColor::EditorSpriteBorder;
  color_mapping["editor_sprite_bottom_border"] = ThemeColor::EditorSpriteBottomBorder;
  color_mapping["listitem_normal_text"] = ThemeColor::ListItemNormalText;
  color_mapping["listitem_normal_face"] = ThemeColor::ListItemNormalFace;
  color_mapping["listitem_selected_text"] = ThemeColor::ListItemSelectedText;
  color_mapping["listitem_selected_face"] = ThemeColor::ListItemSelectedFace;
  color_mapping["slider_empty_text"] = ThemeColor::SliderEmptyText;
  color_mapping["slider_empty_face"] = ThemeColor::SliderEmptyFace;
  color_mapping["slider_full_text"] = ThemeColor::SliderFullText;
  color_mapping["slider_full_face"] = ThemeColor::SliderFullFace;
  color_mapping["tab_normal_text"] = ThemeColor::TabNormalText;
  color_mapping["tab_normal_face"] = ThemeColor::TabNormalFace;
  color_mapping["tab_selected_text"] = ThemeColor::TabSelectedText;
  color_mapping["tab_selected_face"] = ThemeColor::TabSelectedFace;
  color_mapping["splitter_normal_face"] = ThemeColor::SplitterNormalFace;
  color_mapping["scrollbar_bg_face"] = ThemeColor::ScrollBarBgFace;
  color_mapping["scrollbar_thumb_face"] = ThemeColor::ScrollBarThumbFace;
  color_mapping["popup_window_border"] = ThemeColor::PopupWindowBorder;
  color_mapping["tooltip_text"] = ThemeColor::TooltipText;
  color_mapping["tooltip_face"] = ThemeColor::TooltipFace;
  color_mapping["filelist_even_row_text"] = ThemeColor::FileListEvenRowText;
  color_mapping["filelist_even_row_face"] = ThemeColor::FileListEvenRowFace;
  color_mapping["filelist_odd_row_text"] = ThemeColor::FileListOddRowText;
  color_mapping["filelist_odd_row_face"] = ThemeColor::FileListOddRowFace;
  color_mapping["filelist_selected_row_text"] = ThemeColor::FileListSelectedRowText;
  color_mapping["filelist_selected_row_face"] = ThemeColor::FileListSelectedRowFace;
  color_mapping["filelist_disabled_row_text"] = ThemeColor::FileListDisabledRowText;
  color_mapping["workspace"] = ThemeColor::Workspace;

  reload_skin();
}

SkinTheme::~SkinTheme()
{
  // Delete all cursors.
  for (size_t c=0; c<m_cursors.size(); ++c)
    delete m_cursors[c];

  for (int c=0; c<PARTS; ++c)
    destroy_bitmap(m_part[c]);

  for (std::map<std::string, BITMAP*>::iterator
         it = m_toolicon.begin(); it != m_toolicon.end(); ++it) {
    destroy_bitmap(it->second);
  }

  destroy_bitmap(m_sheet_bmp);
  sheet_mapping.clear();

  // Destroy the minifont
  if (m_minifont && m_minifont != font)
    destroy_font(m_minifont);
}

// Call ji_regen_theme after this
void SkinTheme::reload_skin()
{
  if (m_sheet_bmp) {
    destroy_bitmap(m_sheet_bmp);
    m_sheet_bmp = NULL;
  }

  // Load the skin sheet
  std::string sheet_filename("skins/" + m_selected_skin + "/sheet.png");
  {
    ResourceFinder rf;
    rf.findInDataDir(sheet_filename.c_str());

    while (const char* path = rf.next()) {
      if (exists(path)) {
        int old_color_conv = _color_conv;
        set_color_conversion(COLORCONV_NONE);

        PALETTE pal;
        m_sheet_bmp = load_png(path, pal);

        set_color_conversion(old_color_conv);
        break;
      }
    }
  }
  if (!m_sheet_bmp)
    throw base::Exception("Error loading %s file", sheet_filename.c_str());
}

void SkinTheme::reload_fonts()
{
  if (default_font && default_font != font)
    destroy_font(default_font);

  if (m_minifont && m_minifont != font)
    destroy_font(m_minifont);

  default_font = loadFont("UserFont", "skins/" + m_selected_skin + "/font.png");
  m_minifont = loadFont("UserMiniFont", "skins/" + m_selected_skin + "/minifont.png");
}

gfx::Size SkinTheme::get_part_size(int part_i) const
{
  BITMAP* bmp = get_part(part_i);
  return gfx::Size(bmp->w, bmp->h);
}

void SkinTheme::onRegenerate()
{
  scrollbar_size = 12 * jguiscale();

  // Load the skin XML
  std::string xml_filename = "skins/" + m_selected_skin + "/skin.xml";
  ResourceFinder rf;
  rf.findInDataDir(xml_filename.c_str());

  while (const char* path = rf.next()) {
    if (!exists(path))
      continue;

    TiXmlDocument doc;
    if (!doc.LoadFile(path))
      throw XmlException(&doc);

    TiXmlHandle handle(&doc);

    // Load colors
    {
      TiXmlElement* xmlColor = handle
        .FirstChild("skin")
        .FirstChild("colors")
        .FirstChild("color").ToElement();
      while (xmlColor) {
        std::string id = xmlColor->Attribute("id");
        uint32_t value = strtol(xmlColor->Attribute("value")+1, NULL, 16);

        std::map<std::string, ThemeColor::Type>::iterator it =
          color_mapping.find(id);
        if (it != color_mapping.end()) {
          m_colors[it->second] = ui::rgba((value & 0xff0000) >> 16,
                                          (value & 0xff00) >> 8,
                                          (value & 0xff));
        }

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

        for (c=0; c<kCursorTypes; ++c) {
          if (id != cursor_names[c])
            continue;

          delete m_cursors[c];
          m_cursors[c] = NULL;

          BITMAP* bmp = cropPartFromSheet(NULL, x, y, w, h);
          she::Surface* surface =
            she::Instance()->createSurfaceFromNativeHandle(reinterpret_cast<void*>(bmp));

          m_cursors[c] = new Cursor(surface, gfx::Point(focusx*jguiscale(),
                                                        focusy*jguiscale()));
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

        // Crop the tool-icon from the sheet
        m_toolicon[tool_id] = cropPartFromSheet(m_toolicon[tool_id], x, y, w, h);

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
        std::map<std::string, int>::iterator it = sheet_mapping.find(part_id);
        if (it == sheet_mapping.end()) {
          throw base::Exception("Unknown part specified in '%s':\n"
                                "<part id='%s' ... />\n", xml_filename.c_str(), part_id);
        }

        int c = it->second;

        if (w > 0 && h > 0) {
          // Crop the part from the sheet
          m_part[c] = cropPartFromSheet(m_part[c], x, y, w, h);
        }
        else if (xmlPart->Attribute("w1")) { // 3x3-1 part (NW, N, NE, E, SE, S, SW, W)
          int w1 = strtol(xmlPart->Attribute("w1"), NULL, 10);
          int w2 = strtol(xmlPart->Attribute("w2"), NULL, 10);
          int w3 = strtol(xmlPart->Attribute("w3"), NULL, 10);
          int h1 = strtol(xmlPart->Attribute("h1"), NULL, 10);
          int h2 = strtol(xmlPart->Attribute("h2"), NULL, 10);
          int h3 = strtol(xmlPart->Attribute("h3"), NULL, 10);

          m_part[c  ] = cropPartFromSheet(m_part[c  ], x, y, w1, h1); // NW
          m_part[c+1] = cropPartFromSheet(m_part[c+1], x+w1, y, w2, h1); // N
          m_part[c+2] = cropPartFromSheet(m_part[c+2], x+w1+w2, y, w3, h1); // NE
          m_part[c+3] = cropPartFromSheet(m_part[c+3], x+w1+w2, y+h1, w3, h2); // E
          m_part[c+4] = cropPartFromSheet(m_part[c+4], x+w1+w2, y+h1+h2, w3, h3); // SE
          m_part[c+5] = cropPartFromSheet(m_part[c+5], x+w1, y+h1+h2, w2, h3); // S
          m_part[c+6] = cropPartFromSheet(m_part[c+6], x, y+h1+h2, w1, h3); // SW
          m_part[c+7] = cropPartFromSheet(m_part[c+7], x, y+h1, w1, h2); // W
        }

        xmlPart = xmlPart->NextSiblingElement();
      }
    }

    break;
  }
}

BITMAP* SkinTheme::cropPartFromSheet(BITMAP* bmp, int x, int y, int w, int h)
{
  int colordepth = 32;

  if (bmp &&
      (bmp->w != w ||
       bmp->h != h ||
       bitmap_color_depth(bmp) != colordepth)) {
    destroy_bitmap(bmp);
    bmp = NULL;
  }

  if (!bmp)
    bmp = create_bitmap_ex(colordepth, w, h);

  blit(m_sheet_bmp, bmp, x, y, 0, 0, w, h);
  return ji_apply_guiscale(bmp);
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
#define BORDER(n)                       \
  widget->border_width.l = (n);         \
  widget->border_width.t = (n);         \
  widget->border_width.r = (n);         \
  widget->border_width.b = (n);

#define BORDER4(L,T,R,B)                \
  widget->border_width.l = (L);         \
  widget->border_width.t = (T);         \
  widget->border_width.r = (R);         \
  widget->border_width.b = (B);

  int scale = jguiscale();

  switch (widget->type) {

    case kBoxWidget:
      BORDER(0);
      widget->child_spacing = 4 * scale;
      break;

    case kButtonWidget:
      BORDER4(m_part[PART_BUTTON_NORMAL_W]->w,
              m_part[PART_BUTTON_NORMAL_N]->h,
              m_part[PART_BUTTON_NORMAL_E]->w,
              m_part[PART_BUTTON_NORMAL_S]->h);
      widget->child_spacing = 0;
      break;

    case kCheckWidget:
      BORDER(2 * scale);
      widget->child_spacing = 4 * scale;

      static_cast<ButtonBase*>(widget)->setIconInterface
        (new ButtonIconImpl(static_cast<SkinTheme*>(widget->getTheme()),
                            PART_CHECK_NORMAL,
                            PART_CHECK_SELECTED,
                            PART_CHECK_DISABLED,
                            JI_LEFT | JI_MIDDLE));
      break;

    case kEntryWidget:
      BORDER4(m_part[PART_SUNKEN_NORMAL_W]->w,
              m_part[PART_SUNKEN_NORMAL_N]->h,
              m_part[PART_SUNKEN_NORMAL_E]->w,
              m_part[PART_SUNKEN_NORMAL_S]->h);
      break;

    case kGridWidget:
      BORDER(0);
      widget->child_spacing = 4 * scale;
      break;

    case kLabelWidget:
      BORDER(1 * scale);
      static_cast<Label*>(widget)->setTextColor(getColor(ThemeColor::Text));
      break;

    case kListBoxWidget:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case kListItemWidget:
      BORDER(1 * scale);
      break;

    case kComboBoxWidget:
      {
        ComboBox* combobox = dynamic_cast<ComboBox*>(widget);
        ASSERT(combobox != NULL);

        Button* button = combobox->getButtonWidget();

        button->border_width.l = 0;
        button->border_width.t = 0;
        button->border_width.r = 0;
        button->border_width.b = 0;
        button->child_spacing = 0;
        button->min_w = 15 * jguiscale();
        button->min_h = 16 * jguiscale();

        static_cast<ButtonBase*>(button)->setIconInterface
          (new ButtonIconImpl(static_cast<SkinTheme*>(button->getTheme()),
                              PART_COMBOBOX_ARROW_DOWN,
                              PART_COMBOBOX_ARROW_DOWN_SELECTED,
                              PART_COMBOBOX_ARROW_DOWN_DISABLED,
                              JI_CENTER | JI_MIDDLE));
      }
      break;

    case kMenuWidget:
    case kMenuBarWidget:
    case kMenuBoxWidget:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case kMenuItemWidget:
      BORDER(2 * scale);
      widget->child_spacing = 18 * scale;
      break;

    case kSplitterWidget:
      BORDER(0);
      widget->child_spacing = 3 * scale;
      break;

    case kRadioWidget:
      BORDER(2 * scale);
      widget->child_spacing = 4 * scale;

      static_cast<ButtonBase*>(widget)->setIconInterface
        (new ButtonIconImpl(static_cast<SkinTheme*>(widget->getTheme()),
                            PART_RADIO_NORMAL,
                            PART_RADIO_SELECTED,
                            PART_RADIO_DISABLED,
                            JI_LEFT | JI_MIDDLE));
      break;

    case kSeparatorWidget:
      /* frame */
      if ((widget->getAlign() & JI_HORIZONTAL) &&
          (widget->getAlign() & JI_VERTICAL)) {
        BORDER(4 * scale);
      }
      /* horizontal bar */
      else if (widget->getAlign() & JI_HORIZONTAL) {
        BORDER4(2 * scale, 4 * scale, 2 * scale, 0);
      }
      /* vertical bar */
      else {
        BORDER4(4 * scale, 2 * scale, 0, 2 * scale);
      }

      if (widget->hasText()) {
        if (widget->getAlign() & JI_TOP)
          widget->border_width.t = jwidget_get_text_height(widget);
        else if (widget->getAlign() & JI_BOTTOM)
          widget->border_width.b = jwidget_get_text_height(widget);
      }
      break;

    case kSliderWidget:
      BORDER4(m_part[PART_SLIDER_EMPTY_W]->w-1*scale,
              m_part[PART_SLIDER_EMPTY_N]->h,
              m_part[PART_SLIDER_EMPTY_E]->w-1*scale,
              m_part[PART_SLIDER_EMPTY_S]->h-1*scale);
      widget->child_spacing = jwidget_get_text_height(widget);
      widget->setAlign(JI_CENTER | JI_MIDDLE);
      break;

    case kTextBoxWidget:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case kViewWidget:
      BORDER4(m_part[PART_SUNKEN_NORMAL_W]->w-1*scale,
              m_part[PART_SUNKEN_NORMAL_N]->h,
              m_part[PART_SUNKEN_NORMAL_E]->w-1*scale,
              m_part[PART_SUNKEN_NORMAL_S]->h-1*scale);
      widget->child_spacing = 0;
      break;

    case kViewScrollbarWidget:
      BORDER(1 * scale);
      widget->child_spacing = 0;
      break;

    case kViewViewportWidget:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case kWindowWidget:
      if (!static_cast<Window*>(widget)->isDesktop()) {
        if (widget->hasText()) {
          BORDER4(6 * scale, (4+6) * scale, 6 * scale, 6 * scale);
          widget->border_width.t += jwidget_get_text_height(widget);

          if (!(widget->flags & JI_INITIALIZED)) {
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
      widget->child_spacing = 4 * scale;
      widget->setBgColor(getColor(ThemeColor::WindowFace));
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
    gfx::Rect rect(0, 0, 0, 0);

    rect.w = m_part[PART_WINDOW_CLOSE_BUTTON_NORMAL]->w;
    rect.h = m_part[PART_WINDOW_CLOSE_BUTTON_NORMAL]->h;

    rect.offset(window->rc->x2 - 3*jguiscale() - rect.w,
                window->rc->y1 + 3*jguiscale());

    widget->setBounds(rect);
  }
}

void SkinTheme::paintDesktop(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  g->fillRect(getColor(ThemeColor::Disabled), g->getClipBounds());
}

void SkinTheme::paintBox(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintButton(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  IButtonIcon* iconInterface = widget->getIconInterface();
  struct jrect box, text, icon;
  ui::Color fg, bg;
  int part_nw;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
                            iconInterface ? iconInterface->getIconAlign(): 0,
                            iconInterface ? iconInterface->getWidth() : 0,
                            iconInterface ? iconInterface->getHeight() : 0);

  // Tool buttons are smaller
  LookType look = NormalLook;
  SharedPtr<SkinProperty> skinPropery = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinPropery != NULL)
    look = skinPropery->getLook();

  // Selected
  if (widget->isSelected()) {
    fg = getColor(ThemeColor::ButtonSelectedText);
    bg = getColor(ThemeColor::ButtonSelectedFace);
    part_nw = (look == MiniLook ? PART_TOOLBUTTON_NORMAL_NW:
               look == LeftButtonLook ? PART_DROP_DOWN_BUTTON_LEFT_SELECTED_NW:
               look == RightButtonLook ? PART_DROP_DOWN_BUTTON_RIGHT_SELECTED_NW:
                                         PART_BUTTON_SELECTED_NW);
  }
  // With mouse
  else if (widget->isEnabled() && widget->hasMouseOver()) {
    fg = getColor(ThemeColor::ButtonHotText);
    bg = getColor(ThemeColor::ButtonHotFace);
    part_nw = (look == MiniLook ? PART_TOOLBUTTON_HOT_NW:
               look == LeftButtonLook ? PART_DROP_DOWN_BUTTON_LEFT_HOT_NW:
               look == RightButtonLook ? PART_DROP_DOWN_BUTTON_RIGHT_HOT_NW:
                                         PART_BUTTON_HOT_NW);
  }
  // Without mouse
  else {
    fg = getColor(ThemeColor::ButtonNormalText);
    bg = getColor(ThemeColor::ButtonNormalFace);

    if (widget->hasFocus())
      part_nw = (look == MiniLook ? PART_TOOLBUTTON_HOT_NW:
                 look == LeftButtonLook ? PART_DROP_DOWN_BUTTON_LEFT_FOCUSED_NW:
                 look == RightButtonLook ? PART_DROP_DOWN_BUTTON_RIGHT_FOCUSED_NW:
                                           PART_BUTTON_FOCUSED_NW);
    else
      part_nw = (look == MiniLook ? PART_TOOLBUTTON_NORMAL_NW:
                 look == LeftButtonLook ? PART_DROP_DOWN_BUTTON_LEFT_NORMAL_NW:
                 look == RightButtonLook ? PART_DROP_DOWN_BUTTON_RIGHT_NORMAL_NW:
                                           PART_BUTTON_NORMAL_NW);
  }

  // external background
  g->fillRect(BGCOLOR, g->getClipBounds());

  // draw borders
  draw_bounds_nw(g, widget->getClientBounds(), part_nw, bg);

  // text
  drawTextString(g, NULL, fg, bg, false, widget,
                 widget->getClientChildrenBounds(), get_button_selected_offset());

  // Paint the icon
  if (iconInterface) {
    if (widget->isSelected())
      jrect_displace(&icon,
                     get_button_selected_offset(),
                     get_button_selected_offset());

    paintIcon(widget, ev.getGraphics(), iconInterface,
              icon.x1-widget->rc->x1,
              icon.y1-widget->rc->y1);
  }
}

void SkinTheme::paintCheckBox(PaintEvent& ev)
{
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  IButtonIcon* iconInterface = widget->getIconInterface();
  struct jrect box, text, icon;
  ui::Color bg;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
                            iconInterface ? iconInterface->getIconAlign(): 0,
                            iconInterface ? iconInterface->getWidth() : 0,
                            iconInterface ? iconInterface->getHeight() : 0);

  // Check box look
  LookType look = NormalLook;
  SharedPtr<SkinProperty> skinPropery = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinPropery != NULL)
    look = skinPropery->getLook();

  // Background
  jdraw_rectfill(widget->rc, bg = BGCOLOR);

  // Mouse
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      jdraw_rectfill(widget->rc, bg = getColor(ThemeColor::CheckHotFace));
    else if (widget->hasFocus())
      jdraw_rectfill(widget->rc, bg = getColor(ThemeColor::CheckFocusFace));
  }

  // Text
  drawTextStringDeprecated(NULL, ColorNone, bg, false, widget, &text, 0);

  // Paint the icon
  if (iconInterface)
    paintIcon(widget, ev.getGraphics(), iconInterface, icon.x1-widget->rc->x1, icon.y1-widget->rc->y1);

  // draw focus
  if (look != WithoutBordersLook && widget->hasFocus()) {
    draw_bounds_nw(ji_screen,
                   widget->rc->x1,
                   widget->rc->y1,
                   widget->rc->x2-1,
                   widget->rc->y2-1, PART_CHECK_FOCUS_NW, ui::ColorNone);
  }
}

void SkinTheme::paintGrid(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintEntry(PaintEvent& ev)
{
  Entry* widget = static_cast<Entry*>(ev.getSource());
  bool password = widget->isPassword();
  int scroll, caret, state, selbeg, selend;
  std::string textString = widget->getText() + widget->getSuffix();
  int suffixIndex = widget->getTextSize();
  const char* text = textString.c_str();
  int c, ch, x, y, w;
  int x1, y1, x2, y2;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  // Outside borders
  jdraw_rectfill(widget->rc, BGCOLOR);

  // Main pos
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  bool isMiniLook = false;
  SharedPtr<SkinProperty> skinPropery = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinPropery != NULL)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  ui::Color bg = getColor(ThemeColor::Background);
  draw_bounds_nw(ji_screen,
                 x1, y1, x2, y2,
                 (widget->hasFocus() ?
                  (isMiniLook ? PART_SUNKEN_MINI_FOCUSED_NW: PART_SUNKEN_FOCUSED_NW):
                  (isMiniLook ? PART_SUNKEN_MINI_NORMAL_NW : PART_SUNKEN_NORMAL_NW)),
                 bg);

  // Draw the text
  x = widget->rc->x1 + widget->border_width.l;
  y = (widget->rc->y1+widget->rc->y2)/2 - jwidget_get_text_height(widget)/2;

  for (c=scroll; ugetat(text, c); c++) {
    ch = password ? '*': ugetat(text, c);

    // Normal text
    bg = ColorNone;
    ui::Color fg = getColor(ThemeColor::Text);

    // Selected
    if ((c >= selbeg) && (c <= selend)) {
      if (widget->hasFocus())
        bg = getColor(ThemeColor::Selected);
      else
        bg = getColor(ThemeColor::Disabled);
      fg = getColor(ThemeColor::Background);
    }

    // Disabled
    if (!widget->isEnabled()) {
      bg = ColorNone;
      fg = getColor(ThemeColor::Disabled);
    }

    // Suffix
    if (c >= suffixIndex) {
      if (widget->hasFocus())
        break;

      bg = ColorNone;
      fg = getColor(ThemeColor::EntrySuffix);
    }

    w = CHARACTER_LENGTH(widget->getFont(), ch);
    if (x+w > widget->rc->x2-3)
      return;

    caret_x = x;
    ji_font_set_aa_mode(widget->getFont(),
                        to_system(is_transparent(bg) ? getColor(ThemeColor::Background):
                                                       bg));
    widget->getFont()->vtable->render_char(widget->getFont(), ch,
                                           to_system(fg),
                                           to_system(bg), ji_screen, x, y);
    x += w;

    // Caret
    if ((c == caret) && (state) && (widget->hasFocus()))
      draw_entry_caret(widget, caret_x, y);
  }

  // Draw the caret if it is next of the last character
  if ((c == caret) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled())) {
    draw_entry_caret(widget, x, y);
  }
}

void SkinTheme::paintLabel(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Label* widget = static_cast<Label*>(ev.getSource());
  struct jrect text;
  ui::Color bg = BGCOLOR;
  ui::Color fg = widget->getTextColor();
  Rect rc = widget->getClientBounds();

  g->fillRect(bg, rc);
  rc.shrink(widget->getBorder());

  jwidget_get_texticon_info(widget, NULL, &text, NULL, 0, 0, 0);

  g->drawString(widget->getText(), fg, bg, false,
                // TODO "text" coordinates are absolute and we are drawing on client area
                Point(text.x1, text.y1) - Point(widget->rc->x1, widget->rc->y1));
}

void SkinTheme::paintLinkLabel(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  ui::Color bg = BGCOLOR;

  jdraw_rectfill(widget->rc, bg);
  drawTextStringDeprecated(NULL, getColor(ThemeColor::LinkText), bg, false, widget, widget->rc, 0);

  if (widget->hasMouseOver()) {
    int w = jwidget_get_text_length(widget);

    hline(ji_screen,
          widget->rc->x1, widget->rc->y2-1, widget->rc->x1+w-1,
          to_system(getColor(ThemeColor::LinkText)));
  }
}

void SkinTheme::paintListBox(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  g->fillRect(getColor(ThemeColor::Background), g->getClipBounds());
}

void SkinTheme::paintListItem(ui::PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  ui::Color fg, bg;
  int x, y;

  if (!widget->isEnabled()) {
    bg = getColor(ThemeColor::Face);
    fg = getColor(ThemeColor::Disabled);
  }
  else if (widget->isSelected()) {
    fg = getColor(ThemeColor::ListItemSelectedText);
    bg = getColor(ThemeColor::ListItemSelectedFace);
  }
  else {
    fg = getColor(ThemeColor::ListItemNormalText);
    bg = getColor(ThemeColor::ListItemNormalFace);
  }

  x = widget->rc->x1+widget->border_width.l;
  y = widget->rc->y1+widget->border_width.t;

  if (widget->hasText()) {
    // Text
    jdraw_text(ji_screen, widget->getFont(), widget->getText(), x, y,
               fg, bg, true, jguiscale());

    // Background
    jrectexclude
      (ji_screen,
       widget->rc->x1, widget->rc->y1,
       widget->rc->x2-1, widget->rc->y2-1,
       x, y,
       x+jwidget_get_text_length(widget)-1,
       y+jwidget_get_text_height(widget)-1, bg);
  }
  // Background
  else {
    jdraw_rectfill(widget->rc, bg);
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
  MenuItem* widget = static_cast<MenuItem*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  ui::Color fg, bg;
  int x1, y1, x2, y2;
  int c, bar;

  // TODO ASSERT?
  if (!widget->getParent()->getParent())
    return;

  bar = (widget->getParent()->getParent()->type == kMenuBarWidget);

  // Colors
  if (!widget->isEnabled()) {
    fg = ColorNone;
    bg = getColor(ThemeColor::MenuItemNormalFace);
  }
  else {
    if (widget->isHighlighted()) {
      fg = getColor(ThemeColor::MenuItemHighlightText);
      bg = getColor(ThemeColor::MenuItemHighlightFace);
    }
    else if (widget->hasMouseOver()) {
      fg = getColor(ThemeColor::MenuItemHotText);
      bg = getColor(ThemeColor::MenuItemHotFace);
    }
    else {
      fg = getColor(ThemeColor::MenuItemNormalText);
      bg = getColor(ThemeColor::MenuItemNormalFace);
    }
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* background */
  rectfill(ji_screen, x1, y1, x2, y2, to_system(bg));

  /* draw an indicator for selected items */
  if (widget->isSelected()) {
    BITMAP* icon = m_part[widget->isEnabled() ? PART_CHECK_SELECTED:
                                                PART_CHECK_DISABLED];

    int x = widget->rc->x1+4-icon->w/2;
    int y = (widget->rc->y1+widget->rc->y2)/2-icon->h/2;

    set_alpha_blender();
    draw_trans_sprite(ji_screen, icon, x, y);
  }

  /* text */
  if (bar)
    widget->setAlign(JI_CENTER | JI_MIDDLE);
  else
    widget->setAlign(JI_LEFT | JI_MIDDLE);

  Rect pos = widget->getClientBounds();
  if (!bar)
    pos.offset(widget->child_spacing/2, 0);
  drawTextString(g, NULL, fg, bg, false, widget, pos, 0);

  // For menu-box
  if (!bar) {
    // Draw the arrown (to indicate which this menu has a sub-menu)
    if (widget->getSubmenu()) {
      int scale = jguiscale();

      // Enabled
      if (widget->isEnabled()) {
        for (c=0; c<3*scale; c++)
          vline(ji_screen,
                widget->rc->x2-3*scale-c,
                (widget->rc->y1+widget->rc->y2)/2-c,
                (widget->rc->y1+widget->rc->y2)/2+c, to_system(fg));
      }
      // Disabled
      else {
        for (c=0; c<3*scale; c++)
          vline(ji_screen,
                widget->rc->x2-3*scale-c+1,
                (widget->rc->y1+widget->rc->y2)/2-c+1,
                (widget->rc->y1+widget->rc->y2)/2+c+1,
                to_system(getColor(ThemeColor::Background)));
        for (c=0; c<3*scale; c++)
          vline(ji_screen,
                widget->rc->x2-3*scale-c,
                (widget->rc->y1+widget->rc->y2)/2-c,
                (widget->rc->y1+widget->rc->y2)/2+c,
                to_system(getColor(ThemeColor::Disabled)));
      }
    }
    // Draw the keyboard shortcut
    else if (widget->getAccel()) {
      int old_align = widget->getAlign();

      pos = widget->getClientBounds();
      pos.w -= widget->child_spacing/4;

      std::string buf = widget->getAccel()->toString();

      widget->setAlign(JI_RIGHT | JI_MIDDLE);
      drawTextString(g, buf.c_str(), fg, bg, false, widget, pos, 0);
      widget->setAlign(old_align);
    }
  }
}

void SkinTheme::paintSplitter(PaintEvent& ev)
{
  Splitter* splitter = static_cast<Splitter*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  g->fillRect(getColor(ThemeColor::SplitterNormalFace), g->getClipBounds());
}

void SkinTheme::paintRadioButton(PaintEvent& ev)
{
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  IButtonIcon* iconInterface = widget->getIconInterface();
  struct jrect box, text, icon;
  ui::Color bg = BGCOLOR;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
                            iconInterface ? iconInterface->getIconAlign(): 0,
                            iconInterface ? iconInterface->getWidth() : 0,
                            iconInterface ? iconInterface->getHeight() : 0);

  // Background
  g->fillRect(bg, g->getClipBounds());

  // Mouse
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      jdraw_rectfill(widget->rc, bg = getColor(ThemeColor::RadioHotFace));
    else if (widget->hasFocus())
      jdraw_rectfill(widget->rc, bg = getColor(ThemeColor::RadioFocusFace));
  }

  // Text
  drawTextStringDeprecated(NULL, ColorNone, bg, false, widget, &text, 0);

  // Paint the icon
  if (iconInterface)
    paintIcon(widget, g, iconInterface, icon.x1-widget->rc->x1, icon.y1-widget->rc->y1);

  // draw focus
  if (widget->hasFocus()) {
    draw_bounds_nw(ji_screen,
                   widget->rc->x1,
                   widget->rc->y1,
                   widget->rc->x2-1,
                   widget->rc->y2-1, PART_RADIO_FOCUS_NW, ui::ColorNone);
  }
}

void SkinTheme::paintSeparator(ui::PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  int x1, y1, x2, y2;

  // position
  x1 = widget->rc->x1 + widget->border_width.l/2;
  y1 = widget->rc->y1 + widget->border_width.t/2;
  x2 = widget->rc->x2 - 1 - widget->border_width.r/2;
  y2 = widget->rc->y2 - 1 - widget->border_width.b/2;

  // background
  jdraw_rectfill(widget->rc, BGCOLOR);

  if (widget->getAlign() & JI_HORIZONTAL) {
    draw_part_as_hline(ji_screen,
                       widget->rc->x1,
                       widget->rc->y1,
                       widget->rc->x2-1,
                       widget->rc->y2-1, PART_SEPARATOR_HORZ);
  }

  if (widget->getAlign() & JI_VERTICAL) {
    draw_part_as_vline(ji_screen,
                       widget->rc->x1,
                       widget->rc->y1,
                       widget->rc->x2-1,
                       widget->rc->y2-1, PART_SEPARATOR_VERT);
  }

  // text
  if (widget->hasText()) {
    int h = jwidget_get_text_height(widget);
    Rect r(Point(x1+h/2, y1-h/2),
           Point(x2+1-h, y2+1+h));

    drawTextString(g, NULL, getColor(ThemeColor::Selected), BGCOLOR, false, widget, r, 0);
  }
}

static bool my_add_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
  int u1 = MAX(x1, bitmap->cl);
  int v1 = MAX(y1, bitmap->ct);
  int u2 = MIN(x2, bitmap->cr-1);
  int v2 = MIN(y2, bitmap->cb-1);

  if (u1 > u2 || v1 > v2)
    return false;
  else
    set_clip_rect(bitmap, u1, v1, u2, v2);

  return true;
}

void SkinTheme::paintSlider(PaintEvent& ev)
{
  Slider* widget = static_cast<Slider*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  int min, max, value;
  char buf[256];

  // Outside borders
  ui::Color bgcolor = widget->getBgColor();
  if (!is_transparent(bgcolor))
    jdraw_rectfill(widget->rc, bgcolor);

  widget->getSliderThemeInfo(&min, &max, &value);

  Rect rc(widget->getClientBounds().shrink(widget->getBorder()));
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

  SharedPtr<SkinProperty> skinPropery = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinPropery != NULL)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  if (SkinSliderProperty* sliderProperty = dynamic_cast<SkinSliderProperty*>(skinPropery.get()))
    bgPainter = sliderProperty->getBgPainter();

  // Draw customized background
  if (bgPainter) {
    int nw = PART_MINI_SLIDER_EMPTY_NW;
    BITMAP* thumb = widget->hasFocus() ? m_part[PART_MINI_SLIDER_THUMB_FOCUSED]:
                                         m_part[PART_MINI_SLIDER_THUMB];

    // Draw background
    g->fillRect(BGCOLOR, rc);

    // Draw thumb
    g->drawAlphaBitmap(thumb, x-thumb->w/2, rc.y);

    // Draw borders
    rc.shrink(Border(3 * jguiscale(),
                          thumb->h,
                          3 * jguiscale(),
                          1 * jguiscale()));

    draw_bounds_nw(g, rc, nw, ui::ColorNone);

    // Draw background (using the customized ISliderBgPainter implementation)
    rc.shrink(Border(1, 1, 1, 2) * jguiscale());
    if (!rc.isEmpty())
      bgPainter->paint(widget, g, rc);
  }
  else {
    // Draw borders
    int full_part_nw;
    int empty_part_nw;

    if (isMiniLook) {
      full_part_nw = widget->hasMouseOver() ? PART_MINI_SLIDER_FULL_FOCUSED_NW:
                                              PART_MINI_SLIDER_FULL_NW;
      empty_part_nw = widget->hasMouseOver() ? PART_MINI_SLIDER_EMPTY_FOCUSED_NW:
                                               PART_MINI_SLIDER_EMPTY_NW;
    }
    else {
      full_part_nw = widget->hasFocus() ? PART_SLIDER_FULL_FOCUSED_NW:
                                          PART_SLIDER_FULL_NW;
      empty_part_nw = widget->hasFocus() ? PART_SLIDER_EMPTY_FOCUSED_NW:
                                           PART_SLIDER_EMPTY_NW;
    }

    if (value == min)
      draw_bounds_nw(g, rc, empty_part_nw, getColor(ThemeColor::SliderEmptyFace));
    else if (value == max)
      draw_bounds_nw(g, rc, full_part_nw, getColor(ThemeColor::SliderFullFace));
    else
      draw_bounds_nw2(g, rc, x,
                      full_part_nw, empty_part_nw,
                      getColor(ThemeColor::SliderFullFace),
                      getColor(ThemeColor::SliderEmptyFace));

    // Draw text
    std::string old_text = widget->getText();

    usprintf(buf, "%d", value);

    widget->setTextQuiet(buf);

    if (IntersectClip clip = IntersectClip(g, Rect(rc.x, rc.y, x-rc.x, rc.h))) {
      drawTextString(g, NULL,
                     getColor(ThemeColor::SliderFullText),
                     getColor(ThemeColor::SliderFullFace), false, widget, rc, 0);
    }

    if (IntersectClip clip = IntersectClip(g, Rect(x+1, rc.y, rc.w-(x-rc.x+1), rc.h))) {
      drawTextString(g, NULL,
                     getColor(ThemeColor::SliderEmptyText),
                     getColor(ThemeColor::SliderEmptyFace), false, widget, rc, 0);
    }

    widget->setTextQuiet(old_text.c_str());
  }
}

void SkinTheme::paintComboBoxEntry(ui::PaintEvent& ev)
{
  Entry* widget = static_cast<Entry*>(ev.getSource());
  bool password = widget->isPassword();
  int scroll, caret, state, selbeg, selend;
  const char *text = widget->getText();
  int c, ch, x, y, w;
  int x1, y1, x2, y2;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  // Outside borders
  jdraw_rectfill(widget->rc, BGCOLOR);

  // Main pos
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  ui::Color fg, bg = getColor(ThemeColor::Background);

  draw_bounds_nw(ji_screen,
                 x1, y1, x2, y2,
                 widget->hasFocus() ? PART_SUNKEN2_FOCUSED_NW:
                                      PART_SUNKEN2_NORMAL_NW, bg);

  /* draw the text */
  x = widget->rc->x1 + widget->border_width.l;
  y = (widget->rc->y1+widget->rc->y2)/2 - jwidget_get_text_height(widget)/2;

  for (c=scroll; ugetat(text, c); c++) {
    ch = password ? '*': ugetat(text, c);

    /* normal text */
    bg = ColorNone;
    fg = getColor(ThemeColor::Text);

    /* selected */
    if ((c >= selbeg) && (c <= selend)) {
      if (widget->hasFocus())
        bg = getColor(ThemeColor::Selected);
      else
        bg = getColor(ThemeColor::Disabled);
      fg = getColor(ThemeColor::Background);
    }

    // Disabled
    if (!widget->isEnabled()) {
      bg = ColorNone;
      fg = getColor(ThemeColor::Disabled);
    }

    w = CHARACTER_LENGTH(widget->getFont(), ch);
    if (x+w > widget->rc->x2-3)
      return;

    caret_x = x;
    ji_font_set_aa_mode(widget->getFont(),
                        to_system(is_transparent(bg) ? getColor(ThemeColor::Background):
                                                       bg));
    widget->getFont()->vtable->render_char(widget->getFont(), ch,
                                           to_system(fg),
                                           to_system(bg), ji_screen, x, y);
    x += w;

    // Caret
    if ((c == caret) && (state) && (widget->hasFocus()))
      draw_entry_caret(widget, caret_x, y);
  }

  // Draw the caret if it is next of the last character
  if ((c == caret) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled())) {
    draw_entry_caret(widget, x, y);
  }
}

void SkinTheme::paintComboBoxButton(PaintEvent& ev)
{
  Button* widget = static_cast<Button*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  IButtonIcon* iconInterface = widget->getIconInterface();
  int part_nw;
  ui::Color bg;

  if (widget->isSelected()) {
    bg = getColor(ThemeColor::ButtonSelectedFace);
    part_nw = PART_TOOLBUTTON_PUSHED_NW;
  }
  // With mouse
  else if (widget->isEnabled() && widget->hasMouseOver()) {
    bg = getColor(ThemeColor::ButtonHotFace);
    part_nw = PART_TOOLBUTTON_HOT_NW;
  }
  // Without mouse
  else {
    bg = getColor(ThemeColor::ButtonNormalFace);
    part_nw = PART_TOOLBUTTON_LAST_NW;
  }

  Rect rc = widget->getClientBounds();

  // external background
  g->fillRect(BGCOLOR, rc);

  // draw borders
  draw_bounds_nw(g, rc, part_nw, bg);

  // Paint the icon
  if (iconInterface) {
    // Icon
    int x = rc.x + rc.w/2 - iconInterface->getWidth()/2;
    int y = rc.y + rc.h/2 - iconInterface->getHeight()/2;

    paintIcon(widget, ev.getGraphics(), iconInterface, x, y);
  }
}

void SkinTheme::paintTextBox(ui::PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  drawTextBox(ji_screen, widget, NULL, NULL,
              getColor(ThemeColor::TextBoxFace),
              getColor(ThemeColor::TextBoxText));
}

void SkinTheme::paintView(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  View* widget = static_cast<View*>(ev.getSource());

  // Outside borders
  jdraw_rectfill(widget->rc, BGCOLOR);

  draw_bounds_nw(g, widget->getClientBounds(),
                 widget->hasFocus() ? PART_SUNKEN_FOCUSED_NW:
                                      PART_SUNKEN_NORMAL_NW,
                 getColor(ThemeColor::Background));
}

void SkinTheme::paintViewScrollbar(PaintEvent& ev)
{
  ScrollBar* widget = static_cast<ScrollBar*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  int pos, len;

  widget->getScrollBarThemeInfo(&pos, &len);

  gfx::Rect rc = widget->getClientBounds();

  draw_bounds_nw(g, rc,
                 PART_SCROLLBAR_BG_NW,
                 getColor(ThemeColor::ScrollBarBgFace));

  // Horizontal bar
  if (widget->getAlign() & JI_HORIZONTAL) {
    rc.x += pos;
    rc.w = len;
  }
  // Vertical bar
  else {
    rc.y += pos;
    rc.h = len;
  }

  draw_bounds_nw(g, rc,
                 PART_SCROLLBAR_THUMB_NW,
                 getColor(ThemeColor::ScrollBarThumbFace));
}

void SkinTheme::paintViewViewport(PaintEvent& ev)
{
  Viewport* widget = static_cast<Viewport*>(ev.getSource());
  Graphics* g = ev.getGraphics();

  g->fillRect(BGCOLOR, widget->getClientBounds());
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
      draw_bounds_nw(g, pos, PART_WINDOW_NW,
                     getColor(ThemeColor::WindowFace));

      pos.h = cpos.y - pos.y;

      // titlebar
      g->setFont(window->getFont());
      g->drawString(window->getText(), ThemeColor::Background, ColorNone,
                    false,
                    gfx::Point(cpos.x,
                               pos.y + pos.h/2 - text_height(window->getFont())/2));
    }
    // menubox
    else {
      draw_bounds_nw(g, pos, PART_MENU_NW,
                     getColor(ThemeColor::WindowFace));
    }
  }
  // desktop
  else {
    g->fillRect(ThemeColor::Desktop, pos);
  }
}

void SkinTheme::paintPopupWindow(PaintEvent& ev)
{
  Window* window = static_cast<Window*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  gfx::Rect pos = window->getClientBounds();

  g->drawRect(getColor(ThemeColor::PopupWindowBorder), pos);
  pos.shrink(1);

  g->fillRect(window->getBgColor(), pos);
  pos.shrink(window->getBorder());

  g->drawString(window->getText(),
                getColor(ThemeColor::Text),
                window->getBgColor(), pos,
                window->getAlign());
}

void SkinTheme::paintWindowButton(ui::PaintEvent& ev)
{
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  Rect rc = widget->getClientBounds();
  int part;

  if (widget->isSelected())
    part = PART_WINDOW_CLOSE_BUTTON_SELECTED;
  else if (widget->hasMouseOver())
    part = PART_WINDOW_CLOSE_BUTTON_HOT;
  else
    part = PART_WINDOW_CLOSE_BUTTON_NORMAL;

  g->drawAlphaBitmap(m_part[part], rc.x, rc.y);
}

void SkinTheme::paintTooltip(PaintEvent& ev)
{
  ui::TipWindow* widget = static_cast<ui::TipWindow*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  Rect rc = widget->getClientBounds();
  ui::Color fg = getColor(ThemeColor::TooltipText);
  ui::Color bg = getColor(ThemeColor::TooltipFace);

  int nw = PART_TOOLTIP_NW;
  int n  = PART_TOOLTIP_N;
  int ne = PART_TOOLTIP_NE;
  int e  = PART_TOOLTIP_E;
  int se = PART_TOOLTIP_SE;
  int s  = PART_TOOLTIP_S;
  int sw = PART_TOOLTIP_SW;
  int w  = PART_TOOLTIP_W;

  switch (widget->getArrowAlign()) {
    case JI_TOP | JI_LEFT:     nw = PART_TOOLTIP_ARROW_NW; break;
    case JI_TOP | JI_RIGHT:    ne = PART_TOOLTIP_ARROW_NE; break;
    case JI_BOTTOM | JI_LEFT:  sw = PART_TOOLTIP_ARROW_SW; break;
    case JI_BOTTOM | JI_RIGHT: se = PART_TOOLTIP_ARROW_SE; break;
  }

  draw_bounds_template(g, rc, nw, n, ne, e, se, s, sw, w);

  // Draw arrow in sides
  BITMAP* arrow = NULL;
  switch (widget->getArrowAlign()) {
    case JI_TOP:
      arrow = m_part[PART_TOOLTIP_ARROW_N];
      g->drawAlphaBitmap(arrow, rc.x+rc.w/2-arrow->w/2, rc.y);
      break;
    case JI_BOTTOM:
      arrow = m_part[PART_TOOLTIP_ARROW_S];
      g->drawAlphaBitmap(arrow, rc.x+rc.w/2-arrow->w/2, rc.y+rc.h-arrow->h);
      break;
    case JI_LEFT:
      arrow = m_part[PART_TOOLTIP_ARROW_W];
      g->drawAlphaBitmap(arrow, rc.x, rc.y+rc.h/2-arrow->h/2);
      break;
    case JI_RIGHT:
      arrow = m_part[PART_TOOLTIP_ARROW_E];
      g->drawAlphaBitmap(arrow, rc.x+rc.w-arrow->w, rc.y+rc.h/2-arrow->h/2);
      break;
  }


  // Fill background
  g->fillRect(bg, Rect(rc).shrink(Border(m_part[w]->w,
                                         m_part[n]->h,
                                         m_part[e]->w,
                                         m_part[s]->h)));

  rc.shrink(widget->getBorder());

  g->drawString(widget->getText(), fg, bg, rc, widget->getAlign());
}

ui::Color SkinTheme::getWidgetBgColor(Widget* widget)
{
  ui::Color c = widget->getBgColor();
  bool decorative = widget->isDecorative();

  return (!is_transparent(c) ? c: (decorative ? getColor(ThemeColor::Selected):
                                                getColor(ThemeColor::Face)));
}

void SkinTheme::drawTextStringDeprecated(const char *t, ui::Color fg_color, ui::Color bg_color,
                                         bool fill_bg, Widget* widget, const JRect rect,
                                         int selected_offset)
{
  if (t || widget->hasText()) {
    int x, y, w, h;

    if (!t) {
      t = widget->getText();
      w = jwidget_get_text_length(widget);
      h = jwidget_get_text_height(widget);
    }
    else {
      w = ji_font_text_len(widget->getFont(), t);
      h = text_height(widget->getFont());
    }

    /* horizontally text alignment */

    if (widget->getAlign() & JI_RIGHT)
      x = rect->x2 - w;
    else if (widget->getAlign() & JI_CENTER)
      x = (rect->x1+rect->x2)/2 - w/2;
    else
      x = rect->x1;

    /* vertically text alignment */

    if (widget->getAlign() & JI_BOTTOM)
      y = rect->y2 - h;
    else if (widget->getAlign() & JI_MIDDLE)
      y = (rect->y1+rect->y2)/2 - h/2;
    else
      y = rect->y1;

    if (widget->isSelected()) {
      x += selected_offset;
      y += selected_offset;
    }

    /* background */
    if (ui::geta(bg_color) > 0) {
      if (!widget->isEnabled())
        rectfill(ji_screen, x, y, x+w-1+jguiscale(), y+h-1+jguiscale(), to_system(bg_color));
      else
        rectfill(ji_screen, x, y, x+w-1, y+h-1, to_system(bg_color));
    }

    /* text */
    if (!widget->isEnabled()) {
      // TODO avoid this
      if (fill_bg)              // Only to draw the background
        jdraw_text(ji_screen, widget->getFont(), t, x, y, ColorNone, bg_color, fill_bg, jguiscale());

      // Draw white part
      jdraw_text(ji_screen, widget->getFont(), t, x+jguiscale(), y+jguiscale(),
                 getColor(ThemeColor::Background), bg_color, fill_bg, jguiscale());

      if (fill_bg)
        fill_bg = false;
    }

    jdraw_text(ji_screen, widget->getFont(), t, x, y,
               (!widget->isEnabled() ? getColor(ThemeColor::Disabled):
                (is_transparent(fg_color) ? getColor(ThemeColor::Text): fg_color)),
               bg_color, fill_bg, jguiscale());
  }
}

void SkinTheme::drawTextString(Graphics* g, const char *t, ui::Color fg_color, ui::Color bg_color,
                               bool fill_bg, Widget* widget, const Rect& rc,
                               int selected_offset)
{
  if (t || widget->hasText()) {
    Rect textrc;

    g->setFont(widget->getFont());

    if (!t)
      t = widget->getText();

    textrc.setSize(g->measureString(t));

    // Horizontally text alignment

    if (widget->getAlign() & JI_RIGHT)
      textrc.x = rc.x + rc.w - textrc.w - 1;
    else if (widget->getAlign() & JI_CENTER)
      textrc.x = rc.getCenter().x - textrc.w/2;
    else
      textrc.x = rc.x;

    // Vertically text alignment

    if (widget->getAlign() & JI_BOTTOM)
      textrc.y = rc.y + rc.h - textrc.h - 1;
    else if (widget->getAlign() & JI_MIDDLE)
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
        g->fillRect(bg_color, Rect(textrc).inflate(jguiscale(), jguiscale()));
      else
        g->fillRect(bg_color, textrc);
    }

    // Text
    if (!widget->isEnabled()) {
      // TODO avoid this
      if (fill_bg)              // Only to draw the background
        g->drawString(t, ColorNone, bg_color, fill_bg, textrc.getOrigin());

      // Draw white part
      g->drawString(t, getColor(ThemeColor::Background), bg_color, fill_bg,
                    textrc.getOrigin() + Point(jguiscale(), jguiscale()));

      if (fill_bg)
        fill_bg = false;
    }

    g->drawString(t,
                  (!widget->isEnabled() ?
                   getColor(ThemeColor::Disabled):
                   (ui::geta(fg_color) > 0 ? fg_color :
                                             getColor(ThemeColor::Text))),
                  bg_color, fill_bg, textrc.getOrigin());
  }
}

void SkinTheme::draw_entry_caret(Entry* widget, int x, int y)
{
  int h = jwidget_get_text_height(widget);

  vline(ji_screen, x,   y-1, y+h, to_system(getColor(ThemeColor::Text)));
  vline(ji_screen, x+1, y-1, y+h, to_system(getColor(ThemeColor::Text)));
}

BITMAP* SkinTheme::get_toolicon(const char* tool_id) const
{
  std::map<std::string, BITMAP*>::const_iterator it = m_toolicon.find(tool_id);
  if (it != m_toolicon.end())
    return it->second;
  else
    return NULL;
}

void SkinTheme::draw_bounds_template(BITMAP* bmp, int x1, int y1, int x2, int y2,
                                     int nw, int n, int ne, int e, int se, int s, int sw, int w)
{
  int x, y;
  int cx1, cy1, cx2, cy2;
  get_clip_rect(bmp, &cx1, &cy1, &cx2, &cy2);

  /* top */

  draw_trans_sprite(bmp, m_part[nw], x1, y1);

  if (my_add_clip_rect(bmp,
                       x1+m_part[nw]->w, y1,
                       x2-m_part[ne]->w, y2)) {
    for (x = x1+m_part[nw]->w;
         x <= x2-m_part[ne]->w;
         x += m_part[n]->w) {
      draw_trans_sprite(bmp, m_part[n], x, y1);
    }
  }
  set_clip_rect(bmp, cx1, cy1, cx2, cy2);

  draw_trans_sprite(bmp, m_part[ne], x2-m_part[ne]->w+1, y1);

  /* bottom */

  draw_trans_sprite(bmp, m_part[sw], x1, y2-m_part[sw]->h+1);

  if (my_add_clip_rect(bmp,
                       x1+m_part[sw]->w, y1,
                       x2-m_part[se]->w, y2)) {
    for (x = x1+m_part[sw]->w;
         x <= x2-m_part[se]->w;
         x += m_part[s]->w) {
      draw_trans_sprite(bmp, m_part[s], x, y2-m_part[s]->h+1);
    }
  }
  set_clip_rect(bmp, cx1, cy1, cx2, cy2);

  draw_trans_sprite(bmp, m_part[se], x2-m_part[se]->w+1, y2-m_part[se]->h+1);

  if (my_add_clip_rect(bmp,
                       x1, y1+m_part[nw]->h,
                       x2, y2-m_part[sw]->h)) {
    /* left */
    for (y = y1+m_part[nw]->h;
         y <= y2-m_part[sw]->h;
         y += m_part[w]->h) {
      draw_trans_sprite(bmp, m_part[w], x1, y);
    }

    /* right */
    for (y = y1+m_part[ne]->h;
         y <= y2-m_part[se]->h;
         y += m_part[e]->h) {
      draw_trans_sprite(bmp, m_part[e], x2-m_part[e]->w+1, y);
    }
  }
  set_clip_rect(bmp, cx1, cy1, cx2, cy2);
}

void SkinTheme::draw_bounds_template(Graphics* g, const Rect& rc,
                                     int nw, int n, int ne, int e, int se, int s, int sw, int w)
{
  int x, y;

  // Top

  g->drawAlphaBitmap(m_part[nw], rc.x, rc.y);

  if (IntersectClip clip = IntersectClip(g, Rect(rc.x+m_part[nw]->w, rc.y,
                                                      rc.w-m_part[nw]->w-m_part[ne]->w, rc.h))) {
    for (x = rc.x+m_part[nw]->w;
         x < rc.x+rc.w-m_part[ne]->w;
         x += m_part[n]->w) {
      g->drawAlphaBitmap(m_part[n], x, rc.y);
    }
  }

  g->drawAlphaBitmap(m_part[ne], rc.x+rc.w-m_part[ne]->w, rc.y);

  // Bottom

  g->drawAlphaBitmap(m_part[sw], rc.x, rc.y+rc.h-m_part[sw]->h);

  if (IntersectClip clip = IntersectClip(g, Rect(rc.x+m_part[sw]->w, rc.y,
                                                      rc.w-m_part[sw]->w-m_part[se]->w, rc.h))) {
    for (x = rc.x+m_part[sw]->w;
         x < rc.x+rc.w-m_part[se]->w;
         x += m_part[s]->w) {
      g->drawAlphaBitmap(m_part[s], x, rc.y+rc.h-m_part[s]->h);
    }
  }

  g->drawAlphaBitmap(m_part[se], rc.x+rc.w-m_part[se]->w, rc.y+rc.h-m_part[se]->h);

  if (IntersectClip clip = IntersectClip(g, Rect(rc.x, rc.y+m_part[nw]->h,
                                                      rc.w, rc.h-m_part[nw]->h-m_part[sw]->h))) {
    // Left
    for (y = rc.y+m_part[nw]->h;
         y < rc.y+rc.h-m_part[sw]->h;
         y += m_part[w]->h) {
      g->drawAlphaBitmap(m_part[w], rc.x, y);
    }

    // Right
    for (y = rc.y+m_part[ne]->h;
         y < rc.y+rc.h-m_part[se]->h;
         y += m_part[e]->h) {
      g->drawAlphaBitmap(m_part[e], rc.x+rc.w-m_part[e]->w, y);
    }
  }
}

void SkinTheme::draw_bounds_array(BITMAP* bmp, int x1, int y1, int x2, int y2, int parts[8])
{
  int nw = parts[0];
  int n  = parts[1];
  int ne = parts[2];
  int e  = parts[3];
  int se = parts[4];
  int s  = parts[5];
  int sw = parts[6];
  int w  = parts[7];

  set_alpha_blender();
  draw_bounds_template(bmp, x1, y1, x2, y2,
                       nw, n, ne, e,
                       se, s, sw, w);
}

void SkinTheme::draw_bounds_nw(BITMAP* bmp, int x1, int y1, int x2, int y2, int nw, ui::Color bg)
{
  set_alpha_blender();
  draw_bounds_template(bmp, x1, y1, x2, y2,
                       nw+0, nw+1, nw+2, nw+3,
                       nw+4, nw+5, nw+6, nw+7);

  // Center
  if (!is_transparent(bg)) {
    x1 += m_part[nw+7]->w;
    y1 += m_part[nw+1]->h;
    x2 -= m_part[nw+3]->w;
    y2 -= m_part[nw+5]->h;

    rectfill(bmp, x1, y1, x2, y2, to_system(bg));
  }
}

void SkinTheme::draw_bounds_nw(Graphics* g, const Rect& rc, int nw, ui::Color bg)
{
  draw_bounds_template(g, rc,
                       nw+0, nw+1, nw+2, nw+3,
                       nw+4, nw+5, nw+6, nw+7);

  // Center
  if (!is_transparent(bg)) {
    g->fillRect(bg, Rect(rc).shrink(Border(m_part[nw+7]->w,
                                           m_part[nw+1]->h,
                                           m_part[nw+3]->w,
                                           m_part[nw+5]->h)));
  }
}

void SkinTheme::draw_bounds_nw2(Graphics* g, const Rect& rc, int x_mid, int nw1, int nw2, ui::Color bg1, ui::Color bg2)
{
  Rect rc2(rc.x, rc.y, x_mid-rc.x+1, rc.h);

  if (IntersectClip clip = IntersectClip(g, rc2))
    draw_bounds_nw(g, rc, nw1, bg1);

  rc2.x += rc2.w;
  rc2.w = rc.w - rc2.w;

  if (IntersectClip clip = IntersectClip(g, rc2))
    draw_bounds_nw(g, rc, nw2, bg2);
}

void SkinTheme::draw_part_as_hline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part)
{
  int x;

  set_alpha_blender();

  for (x = x1;
       x <= x2-m_part[part]->w;
       x += m_part[part]->w) {
    draw_trans_sprite(bmp, m_part[part], x, y1);
  }

  if (x <= x2) {
    int cx1, cy1, cx2, cy2;
    get_clip_rect(bmp, &cx1, &cy1, &cx2, &cy2);

    if (my_add_clip_rect(bmp, x, y1, x2, y1+m_part[part]->h-1))
      draw_trans_sprite(bmp, m_part[part], x, y1);

    set_clip_rect(bmp, cx1, cy1, cx2, cy2);
  }
}

void SkinTheme::draw_part_as_vline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part)
{
  int y;

  set_alpha_blender();

  for (y = y1;
       y <= y2-m_part[part]->h;
       y += m_part[part]->h) {
    draw_trans_sprite(bmp, m_part[part], x1, y);
  }

  if (y <= y2) {
    int cx1, cy1, cx2, cy2;
    get_clip_rect(bmp, &cx1, &cy1, &cx2, &cy2);

    if (my_add_clip_rect(bmp, x1, y, x1+m_part[part]->w-1, y2))
      draw_trans_sprite(bmp, m_part[part], x1, y);

    set_clip_rect(bmp, cx1, cy1, cx2, cy2);
  }
}

void SkinTheme::draw_part_as_hline(ui::Graphics* g, const gfx::Rect& rc, int part)
{
  int x;

  set_alpha_blender();

  for (x = rc.x;
       x < rc.x2()-m_part[part]->w;
       x += m_part[part]->w) {
    g->drawAlphaBitmap(m_part[part], x, rc.y);
  }

  if (x < rc.x2()) {
    Rect rc2(x, rc.y, rc.w-(x-rc.x), m_part[part]->h);
    if (IntersectClip clip = IntersectClip(g, rc2))
      g->drawAlphaBitmap(m_part[part], x, rc.y);
  }
}

void SkinTheme::draw_part_as_vline(ui::Graphics* g, const gfx::Rect& rc, int part)
{
  int y;

  for (y = rc.y;
       y < rc.y2()-m_part[part]->h;
       y += m_part[part]->h) {
    g->drawAlphaBitmap(m_part[part], rc.x, y);
  }

  if (y < rc.y2()) {
    Rect rc2(rc.x, y, m_part[part]->w, rc.h-(y-rc.y));
    if (IntersectClip clip = IntersectClip(g, rc2))
      g->drawAlphaBitmap(m_part[part], rc.x, y);
  }
}

void SkinTheme::drawProgressBar(BITMAP* bmp, int x1, int y1, int x2, int y2, float progress)
{
  int w = x2 - x1 + 1;
  int u = (int)((float)(w-2)*progress);
  u = MID(0, u, w-2);

  rect(bmp, x1, y1, x2, y2, ui::to_system(getColor(ThemeColor::Text)));

  if (u > 0)
    rectfill(bmp, x1+1, y1+1, x1+u, y2-1,
             ui::to_system(getColor(ThemeColor::Selected)));

  if (1+u < w-2)
    rectfill(bmp, x1+u+1, y1+1, x2-1, y2-1,
             ui::to_system(getColor(ThemeColor::Background)));
}

void SkinTheme::paintIcon(Widget* widget, Graphics* g, IButtonIcon* iconInterface, int x, int y)
{
  BITMAP* icon_bmp = NULL;

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
    g->drawAlphaBitmap(icon_bmp, x, y);
}

FONT* SkinTheme::loadFont(const char* userFont, const std::string& path)
{
  // Directories
  ResourceFinder rf;

  const char* user_font = get_config_string("Options", userFont, "");
  if (user_font && *user_font)
    rf.addPath(user_font);

  rf.findInDataDir(path.c_str());

  // Try to load the font
  while (const char* path = rf.next()) {
    FONT* font = ji_font_load(path);
    if (font) {
      if (ji_font_is_scalable(font))
        ji_font_set_size(font, 8*jguiscale());
      return font;
    }
  }

  return font;                  // Use Allegro font by default
}
