/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "app/modules/gui.h"
#include "app/resource_finder.h"
#include "app/ui/skin/button_icon_impl.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_slider_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui/skin/style_sheet.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/shared_ptr.h"
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

#include <allegro.h>

#define BGCOLOR                 (getWidgetBgColor(widget))

namespace app {
namespace skin {

using namespace gfx;
using namespace ui;

static std::map<std::string, int> sheet_mapping;
static std::map<std::string, ThemeColor::Type> color_mapping;

const char* SkinTheme::kThemeCloseButtonId = "theme_close_button";

const char* kWindowFaceColorId = "window_face";
const char* kSeparatorLabelColorId = "separator_label";

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
        jmouse_set_cursor(kArrowCursor);
        return true;

      case kKeyDownMessage:
        if (static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
          setSelected(true);
          return true;
        }
        break;

      case kKeyUpMessage:
        if (static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
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

SkinTheme::SkinTheme()
  : m_cursors(ui::kCursorTypes, NULL)
  , m_colors(ThemeColor::MaxColors)
{
  this->name = "Skin Theme";
  m_selected_skin = get_config_string("Skin", "Selected", "default");
  m_minifont = she::instance()->defaultFont();

  // Initialize all graphics in NULL (these bitmaps are loaded from the skin)
  m_sheet = NULL;
  m_part.resize(PARTS, NULL);

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
  sheet_mapping["newfolder"] = PART_NEWFOLDER;
  sheet_mapping["newfolder_selected"] = PART_NEWFOLDER_SELECTED;
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
  sheet_mapping["selection_replace"] = PART_SELECTION_REPLACE;
  sheet_mapping["selection_replace_selected"] = PART_SELECTION_REPLACE_SELECTED;
  sheet_mapping["selection_add"] = PART_SELECTION_ADD;
  sheet_mapping["selection_add_selected"] = PART_SELECTION_ADD_SELECTED;
  sheet_mapping["selection_subtract"] = PART_SELECTION_SUBTRACT;
  sheet_mapping["selection_subtract_selected"] = PART_SELECTION_SUBTRACT_SELECTED;
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
  sheet_mapping["drop_pixels_ok"] = PART_DROP_PIXELS_OK;
  sheet_mapping["drop_pixels_ok_selected"] = PART_DROP_PIXELS_OK_SELECTED;
  sheet_mapping["drop_pixels_cancel"] = PART_DROP_PIXELS_CANCEL;
  sheet_mapping["drop_pixels_cancel_selected"] = PART_DROP_PIXELS_CANCEL_SELECTED;
  sheet_mapping["freehand_algo_default"] = PART_FREEHAND_ALGO_DEFAULT;
  sheet_mapping["freehand_algo_default_selected"] = PART_FREEHAND_ALGO_DEFAULT_SELECTED;
  sheet_mapping["freehand_algo_pixel_perfect"] = PART_FREEHAND_ALGO_PIXEL_PERFECT;
  sheet_mapping["freehand_algo_pixel_perfect_selected"] = PART_FREEHAND_ALGO_PIXEL_PERFECT_SELECTED;
  sheet_mapping["freehand_algo_dots"] = PART_FREEHAND_ALGO_DOTS;
  sheet_mapping["freehand_algo_dots_selected"] = PART_FREEHAND_ALGO_DOTS_SELECTED;

  color_mapping["text"] = ThemeColor::Text;
  color_mapping["disabled"] = ThemeColor::Disabled;
  color_mapping["face"] = ThemeColor::Face;
  color_mapping["hot_face"] = ThemeColor::HotFace;
  color_mapping["selected"] = ThemeColor::Selected;
  color_mapping["background"] = ThemeColor::Background;
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

  for (std::map<std::string, she::Surface*>::iterator
         it = m_toolicon.begin(); it != m_toolicon.end(); ++it) {
    it->second->dispose();
  }

  if (m_sheet)
    m_sheet->dispose();

  m_part.clear();
  m_parts_by_id.clear();
  sheet_mapping.clear();
  color_mapping.clear();

  // Destroy the minifont
  if (m_minifont)
    m_minifont->dispose();
}

// Call Theme::regenerate() after this.
void SkinTheme::reload_skin()
{
  if (m_sheet) {
    m_sheet->dispose();
    m_sheet = NULL;
  }

  // Load the skin sheet
  std::string sheet_filename("skins/" + m_selected_skin + "/sheet.png");

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

void SkinTheme::reload_fonts()
{
  if (default_font)
    default_font->dispose();

  if (m_minifont)
    m_minifont->dispose();

  default_font = loadFont("UserFont", "skins/" + m_selected_skin + "/font.png");
  m_minifont = loadFont("UserMiniFont", "skins/" + m_selected_skin + "/minifont.png");
}

gfx::Size SkinTheme::get_part_size(int part_i) const
{
  she::Surface* bmp = get_part(part_i);
  ASSERT(bmp);
  return gfx::Size(bmp->width(), bmp->height());
}

void SkinTheme::onRegenerate()
{
  scrollbar_size = 12 * jguiscale();

  m_part.clear();
  m_part.resize(PARTS, NULL);

  // Load the skin XML
  std::string xml_filename = "skins/" + m_selected_skin + "/skin.xml";
  ResourceFinder rf;
  rf.includeDataDir(xml_filename.c_str());
  if (!rf.findFirst())
    return;

  XmlDocumentRef doc = open_xml(rf.filename());
  TiXmlHandle handle(doc);

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

      PRINTF("Loading color '%s'...\n", id.c_str());

      m_colors_by_id[id] = color;

      std::map<std::string, ThemeColor::Type>::iterator it = color_mapping.find(id);
      if (it != color_mapping.end()) {
        m_colors[it->second] = color;
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

      PRINTF("Loading cursor '%s'...\n", id.c_str());

      for (c=0; c<kCursorTypes; ++c) {
        if (id != cursor_names[c])
          continue;

        delete m_cursors[c];
        m_cursors[c] = NULL;

        she::Surface* slice = sliceSheet(NULL, gfx::Rect(x, y, w, h));

        m_cursors[c] = new Cursor(slice,
          gfx::Point(focusx*jguiscale(), focusy*jguiscale()));
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

      PRINTF("Loading tool icon '%s'...\n", tool_id);

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

      PRINTF("Loading part '%s'...\n", part_id);

      SkinPartPtr part = m_parts_by_id[part_id];
      if (part == NULL)
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

      // Prepare the m_part vector (which is used for backward
      // compatibility for widgets that doesn't use SkinStyle).
      std::map<std::string, int>::iterator it = sheet_mapping.find(part_id);
      if (it != sheet_mapping.end()) {
        int c = it->second;
        for (size_t i=0; i<part->size(); ++i)
          m_part[c+i] = part->getBitmap(i);
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

        PRINTF("- Rule '%s' for '%s'\n", ruleName.c_str(), style_id);

        // TODO This code design to read styles could be improved.

        const char* part_id = xmlRule->Attribute("part");
        const char* color_id = xmlRule->Attribute("color");

        // Style align
        int align = 0;
        const char* halign = xmlRule->Attribute("align");
        const char* valign = xmlRule->Attribute("valign");
        if (halign) {
          if (strcmp(halign, "left") == 0) align |= JI_LEFT;
          else if (strcmp(halign, "right") == 0) align |= JI_RIGHT;
          else if (strcmp(halign, "center") == 0) align |= JI_CENTER;
        }
        if (valign) {
          if (strcmp(valign, "top") == 0) align |= JI_TOP;
          else if (strcmp(valign, "bottom") == 0) align |= JI_BOTTOM;
          else if (strcmp(valign, "middle") == 0) align |= JI_MIDDLE;
        }

        if (ruleName == "background") {
          if (color_id) (*style)[StyleSheet::backgroundColorRule()] = css::Value(color_id);
          if (part_id) (*style)[StyleSheet::backgroundPartRule()] = css::Value(part_id);
        }
        else if (ruleName == "icon") {
          if (align) (*style)[StyleSheet::iconAlignRule()] = css::Value(align);
          if (part_id) (*style)[StyleSheet::iconPartRule()] = css::Value(part_id);
        }
        else if (ruleName == "text") {
          if (color_id) (*style)[StyleSheet::textColorRule()] = css::Value(color_id);
          if (align) (*style)[StyleSheet::textAlignRule()] = css::Value(align);

          const char* padding_left = xmlRule->Attribute("padding-left");
          const char* padding_top = xmlRule->Attribute("padding-top");
          const char* padding_right = xmlRule->Attribute("padding-right");
          const char* padding_bottom = xmlRule->Attribute("padding-bottom");

          if (padding_left) (*style)[StyleSheet::paddingLeftRule()] = css::Value(strtol(padding_left, NULL, 10));
          if (padding_top) (*style)[StyleSheet::paddingTopRule()] = css::Value(strtol(padding_top, NULL, 10));
          if (padding_right) (*style)[StyleSheet::paddingRightRule()] = css::Value(strtol(padding_right, NULL, 10));
          if (padding_bottom) (*style)[StyleSheet::paddingBottomRule()] = css::Value(strtol(padding_bottom, NULL, 10));
        }

        xmlRule = xmlRule->NextSiblingElement();
      }

      xmlStyle = xmlStyle->NextSiblingElement();
    }
  }
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

  sur->applyScale(jguiscale());
  return sur;
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
      BORDER4(
        m_part[PART_BUTTON_NORMAL_W]->width(),
        m_part[PART_BUTTON_NORMAL_N]->height(),
        m_part[PART_BUTTON_NORMAL_E]->width(),
        m_part[PART_BUTTON_NORMAL_S]->height());
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
      BORDER4(
        m_part[PART_SUNKEN_NORMAL_W]->width(),
        m_part[PART_SUNKEN_NORMAL_N]->height(),
        m_part[PART_SUNKEN_NORMAL_E]->width(),
        m_part[PART_SUNKEN_NORMAL_S]->height());
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
          widget->border_width.t = widget->getTextHeight();
        else if (widget->getAlign() & JI_BOTTOM)
          widget->border_width.b = widget->getTextHeight();
      }
      break;

    case kSliderWidget:
      BORDER4(
        m_part[PART_SLIDER_EMPTY_W]->width()-1*scale,
        m_part[PART_SLIDER_EMPTY_N]->height(),
        m_part[PART_SLIDER_EMPTY_E]->width()-1*scale,
        m_part[PART_SLIDER_EMPTY_S]->height()-1*scale);
      widget->child_spacing = widget->getTextHeight();
      widget->setAlign(JI_CENTER | JI_MIDDLE);
      break;

    case kTextBoxWidget:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case kViewWidget:
      BORDER4(
        m_part[PART_SUNKEN_NORMAL_W]->width()-1*scale,
        m_part[PART_SUNKEN_NORMAL_N]->height(),
        m_part[PART_SUNKEN_NORMAL_E]->width()-1*scale,
        m_part[PART_SUNKEN_NORMAL_S]->height()-1*scale);
      widget->child_spacing = 0;
      widget->setBgColor(getColorById(kWindowFaceColorId));
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
          widget->border_width.t += widget->getTextHeight();

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
      widget->child_spacing = 4 * scale; // TODO this hard-coded 4 should be configurable in skin.xml
      widget->setBgColor(getColorById(kWindowFaceColorId));
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

    rect.w = m_part[PART_WINDOW_CLOSE_BUTTON_NORMAL]->width();
    rect.h = m_part[PART_WINDOW_CLOSE_BUTTON_NORMAL]->height();

    rect.offset(window->getBounds().x2() - 3*jguiscale() - rect.w,
                window->getBounds().y + 3*jguiscale());

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
  int part_nw;

  widget->getTextIconInfo(&box, &text, &icon,
    iconInterface ? iconInterface->getIconAlign(): 0,
    iconInterface ? iconInterface->getWidth() : 0,
    iconInterface ? iconInterface->getHeight() : 0);

  // Tool buttons are smaller
  LookType look = NormalLook;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
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
    iconInterface ? iconInterface->getWidth() : 0,
    iconInterface ? iconInterface->getHeight() : 0);

  // Check box look
  LookType look = NormalLook;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery != NULL)
    look = skinPropery->getLook();

  // Background
  g->fillRect(bg = BGCOLOR, bounds);

  // Mouse
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      g->fillRect(bg = getColor(ThemeColor::CheckHotFace), bounds);
    else if (widget->hasFocus())
      g->fillRect(bg = getColor(ThemeColor::CheckFocusFace), bounds);
  }

  // Text
  drawTextString(g, NULL, ColorNone, ColorNone, widget, text, 0);

  // Paint the icon
  if (iconInterface)
    paintIcon(widget, g, iconInterface, icon.x, icon.y);

  // draw focus
  if (look != WithoutBordersLook && widget->hasFocus())
    draw_bounds_nw(g, bounds, PART_CHECK_FOCUS_NW, gfx::ColorNone);
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
  std::string textString = widget->getText() + widget->getSuffix();
  int suffixIndex = widget->getTextLength();
  const char* text = textString.c_str();
  int c, ch, x, y, w;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  // Outside borders
  g->fillRect(BGCOLOR, bounds);

  bool isMiniLook = false;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery != NULL)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  gfx::Color bg = getColor(ThemeColor::Background);
  draw_bounds_nw(g, bounds,
    (widget->hasFocus() ?
      (isMiniLook ? PART_SUNKEN_MINI_FOCUSED_NW: PART_SUNKEN_FOCUSED_NW):
      (isMiniLook ? PART_SUNKEN_MINI_NORMAL_NW : PART_SUNKEN_NORMAL_NW)),
    bg);

  // Draw the text
  x = bounds.x + widget->border_width.l;
  y = bounds.y + bounds.h/2 - widget->getTextHeight()/2;

  for (c=scroll; ugetat(text, c); c++) {
    ch = password ? '*': ugetat(text, c);

    // Normal text
    bg = ColorNone;
    gfx::Color fg = getColor(ThemeColor::Text);

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

void SkinTheme::paintLabel(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Label* widget = static_cast<Label*>(ev.getSource());
  gfx::Color bg = BGCOLOR;
  gfx::Color fg = widget->getTextColor();
  Rect text, rc = widget->getClientBounds();

  if (!is_transparent(bg))
    g->fillRect(bg, rc);

  rc.shrink(widget->getBorder());

  widget->getTextIconInfo(NULL, &text);
  g->drawUIString(widget->getText(), fg, ColorNone, text.getOrigin());
}

void SkinTheme::paintLinkLabel(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  gfx::Color fg = getColor(ThemeColor::LinkText);
  gfx::Color bg = BGCOLOR;

  g->fillRect(bg, bounds);
  drawTextString(g, NULL, fg, ColorNone, widget, bounds, 0);

  // Underline style
  if (widget->hasMouseOver()) {
    int w = widget->getTextWidth();
    for (int v=0; v<jguiscale(); ++v)
      g->drawHLine(fg, bounds.x, bounds.y2()-1-v, w);
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
  gfx::Rect bounds = widget->getClientBounds();
  Graphics* g = ev.getGraphics();
  gfx::Color fg, bg;

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

  g->fillRect(bg, bounds);

  if (widget->hasText()) {
    bounds.shrink(widget->getBorder());
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
  int scale = jguiscale();
  Graphics* g = ev.getGraphics();
  MenuItem* widget = static_cast<MenuItem*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  gfx::Color fg, bg;
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
    else if (widget->hasMouse()) {
      fg = getColor(ThemeColor::MenuItemHotText);
      bg = getColor(ThemeColor::MenuItemHotFace);
    }
    else {
      fg = getColor(ThemeColor::MenuItemNormalText);
      bg = getColor(ThemeColor::MenuItemNormalFace);
    }
  }

  // Background
  g->fillRect(bg, bounds);

  // Draw an indicator for selected items
  if (widget->isSelected()) {
    she::Surface* icon =
      m_part[widget->isEnabled() ?
        PART_CHECK_SELECTED:
        PART_CHECK_DISABLED];

    int x = bounds.x+4*scale-icon->width()/2;
    int y = bounds.y+bounds.h/2-icon->height()/2;
    g->drawRgbaSurface(icon, x, y);
  }

  // Text
  if (bar)
    widget->setAlign(JI_CENTER | JI_MIDDLE);
  else
    widget->setAlign(JI_LEFT | JI_MIDDLE);

  Rect pos = bounds;
  if (!bar)
    pos.offset(widget->child_spacing/2, 0);
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
          g->drawVLine(getColor(ThemeColor::Background),
            bounds.x2()-3*scale-c+1,
            bounds.y+bounds.h/2-c+1, 2*c+1);

        for (c=0; c<3*scale; c++)
          g->drawVLine(getColor(ThemeColor::Disabled),
            bounds.x2()-3*scale-c,
            bounds.y+bounds.h/2-c, 2*c+1);
      }
    }
    // Draw the keyboard shortcut
    else if (widget->getAccel()) {
      int old_align = widget->getAlign();

      pos = bounds;
      pos.w -= widget->child_spacing/4;

      std::string buf = widget->getAccel()->toString();

      widget->setAlign(JI_RIGHT | JI_MIDDLE);
      drawTextString(g, buf.c_str(), fg, ColorNone, widget, pos, 0);
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
  Graphics* g = ev.getGraphics();
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  IButtonIcon* iconInterface = widget->getIconInterface();
  gfx::Color bg = BGCOLOR;

  gfx::Rect box, text, icon;
  widget->getTextIconInfo(&box, &text, &icon,
    iconInterface ? iconInterface->getIconAlign(): 0,
    iconInterface ? iconInterface->getWidth() : 0,
    iconInterface ? iconInterface->getHeight() : 0);

  // Background
  g->fillRect(bg, g->getClipBounds());

  // Mouse
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      g->fillRect(bg = getColor(ThemeColor::RadioHotFace), bounds);
    else if (widget->hasFocus())
      g->fillRect(bg = getColor(ThemeColor::RadioFocusFace), bounds);
  }

  // Text
  drawTextString(g, NULL, ColorNone, ColorNone, widget, text, 0);

  // Icon
  if (iconInterface)
    paintIcon(widget, g, iconInterface, icon.x, icon.y);

  // Focus
  if (widget->hasFocus())
    draw_bounds_nw(g, bounds, PART_RADIO_FOCUS_NW, gfx::ColorNone);
}

void SkinTheme::paintSeparator(ui::PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();

  // background
  g->fillRect(BGCOLOR, bounds);

  if (widget->getAlign() & JI_HORIZONTAL)
    draw_part_as_hline(g, bounds, PART_SEPARATOR_HORZ);

  if (widget->getAlign() & JI_VERTICAL)
    draw_part_as_vline(g, bounds, PART_SEPARATOR_VERT);

  // text
  if (widget->hasText()) {
    int h = widget->getTextHeight();
    Rect r(
      Point(
        bounds.x + widget->border_width.l/2 + h/2,
        bounds.y + widget->border_width.t/2 - h/2),
      Point(
        bounds.x2() - widget->border_width.r/2 - h,
        bounds.y2() - widget->border_width.b/2 + h));

    drawTextString(g, NULL,
      getColorById(kSeparatorLabelColorId), BGCOLOR,
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

  Rect rc(Rect(bounds).shrink(widget->getBorder()));
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
  if (skinPropery != NULL)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  SkinSliderPropertyPtr skinSliderPropery = widget->getProperty(SkinSliderProperty::Name);
  if (skinSliderPropery != NULL)
    bgPainter = skinSliderPropery->getBgPainter();

  // Draw customized background
  if (bgPainter) {
    int nw = PART_MINI_SLIDER_EMPTY_NW;
    she::Surface* thumb =
      widget->hasFocus() ? m_part[PART_MINI_SLIDER_THUMB_FOCUSED]:
                           m_part[PART_MINI_SLIDER_THUMB];

    // Draw background
    g->fillRect(BGCOLOR, rc);

    // Draw thumb
    g->drawRgbaSurface(thumb, x-thumb->width()/2, rc.y);

    // Draw borders
    rc.shrink(Border(
        3 * jguiscale(),
        thumb->height(),
        3 * jguiscale(),
        1 * jguiscale()));

    draw_bounds_nw(g, rc, nw, gfx::ColorNone);

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

    {
      char buf[128];
      sprintf(buf, "%d", value);
      widget->setTextQuiet(buf);
    }

    {
      IntersectClip clip(g, Rect(rc.x, rc.y, x-rc.x, rc.h));
      if (clip) {
        drawTextString(g, NULL,
          getColor(ThemeColor::SliderFullText), ColorNone,
          widget, rc, 0);
      }
    }

    {
      IntersectClip clip(g, Rect(x+1, rc.y, rc.w-(x-rc.x+1), rc.h));
      if (clip) {
        drawTextString(g, NULL,
          getColor(ThemeColor::SliderEmptyText),
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
  const char *text = widget->getText().c_str();
  int c, ch, x, y, w;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  // Outside borders
  g->fillRect(BGCOLOR, bounds);

  gfx::Color fg, bg = getColor(ThemeColor::Background);

  draw_bounds_nw(g, bounds,
    widget->hasFocus() ?
      PART_SUNKEN2_FOCUSED_NW:
      PART_SUNKEN2_NORMAL_NW, bg);

  // Draw the text
  x = bounds.x + widget->border_width.l;
  y = bounds.y + bounds.h/2 - widget->getTextHeight()/2;

  for (c=scroll; ugetat(text, c); c++) {
    ch = password ? '*': ugetat(text, c);

    // Normal text
    bg = ColorNone;
    fg = getColor(ThemeColor::Text);

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
  int part_nw;
  gfx::Color bg;

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
  Graphics* g = ev.getGraphics();
  Widget* widget = static_cast<Widget*>(ev.getSource());

  drawTextBox(g, widget, NULL, NULL,
    getColor(ThemeColor::TextBoxFace),
    getColor(ThemeColor::TextBoxText));
}

void SkinTheme::paintView(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  View* widget = static_cast<View*>(ev.getSource());
  gfx::Rect bounds = widget->getClientBounds();
  gfx::Color bg = BGCOLOR;

  if (!is_transparent(bg))
    g->fillRect(bg, bounds);

  draw_bounds_nw(g, bounds,
    (widget->hasFocus() ?
      PART_SUNKEN_FOCUSED_NW:
      PART_SUNKEN_NORMAL_NW),
    BGCOLOR);
}

void SkinTheme::paintViewScrollbar(PaintEvent& ev)
{
  ScrollBar* widget = static_cast<ScrollBar*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  int pos, len;

  bool isMiniLook = false;
  SkinPropertyPtr skinPropery = widget->getProperty(SkinProperty::Name);
  if (skinPropery != NULL)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  skin::Style* bgStyle = get_style(isMiniLook ? "mini_scrollbar": "scrollbar");
  skin::Style* thumbStyle = get_style(isMiniLook ? "mini_scrollbar_thumb": "scrollbar_thumb");

  widget->getScrollBarThemeInfo(&pos, &len);

  gfx::Rect rc = widget->getClientBounds();

  bgStyle->paint(g, rc, NULL, Style::State());

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

  thumbStyle->paint(g, rc, NULL, Style::State());
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
      get_style("window")->paint(g, pos, NULL, Style::State());
      get_style("window_title")->paint(g,
        gfx::Rect(cpos.x, pos.y+5*jguiscale(), cpos.w, // TODO this hard-coded 5 should be configurable in skin.xml
          window->getTextHeight()),
        window->getText().c_str(), Style::State());
    }
    // menubox
    else {
      get_style("menubox")->paint(g, pos, NULL, Style::State());
    }
  }
  // desktop
  else {
    get_style("desktop")->paint(g, pos, NULL, Style::State());
  }
}

void SkinTheme::paintPopupWindow(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Window* window = static_cast<Window*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  gfx::Rect pos = window->getClientBounds();

  if (!is_transparent(BGCOLOR))
    get_style("menubox")->paint(g, pos, NULL, Style::State());

  pos.shrink(window->getBorder());

  g->drawAlignedUIString(window->getText(),
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

  g->drawRgbaSurface(m_part[part], rc.x, rc.y);
}

void SkinTheme::paintTooltip(PaintEvent& ev)
{
  ui::TipWindow* widget = static_cast<ui::TipWindow*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  Rect rc = widget->getClientBounds();
  gfx::Color fg = getColor(ThemeColor::TooltipText);
  gfx::Color bg = getColor(ThemeColor::TooltipFace);

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
  she::Surface* arrow = NULL;
  switch (widget->getArrowAlign()) {
    case JI_TOP:
      arrow = m_part[PART_TOOLTIP_ARROW_N];
      g->drawRgbaSurface(arrow, rc.x+rc.w/2-arrow->width()/2, rc.y);
      break;
    case JI_BOTTOM:
      arrow = m_part[PART_TOOLTIP_ARROW_S];
      g->drawRgbaSurface(arrow,
        rc.x+rc.w/2-arrow->width()/2,
        rc.y+rc.h-arrow->height());
      break;
    case JI_LEFT:
      arrow = m_part[PART_TOOLTIP_ARROW_W];
      g->drawRgbaSurface(arrow, rc.x, rc.y+rc.h/2-arrow->height()/2);
      break;
    case JI_RIGHT:
      arrow = m_part[PART_TOOLTIP_ARROW_E];
      g->drawRgbaSurface(arrow,
        rc.x+rc.w-arrow->width(),
        rc.y+rc.h/2-arrow->height()/2);
      break;
  }

  // Fill background
  g->fillRect(bg, Rect(rc).shrink(
      Border(
        m_part[w]->width(),
        m_part[n]->height(),
        m_part[e]->width(),
        m_part[s]->height())));

  rc.shrink(widget->getBorder());

  g->drawAlignedUIString(widget->getText(), fg, bg, rc, widget->getAlign());
}

gfx::Color SkinTheme::getWidgetBgColor(Widget* widget)
{
  gfx::Color c = widget->getBgColor();
  bool decorative = widget->isDecorative();

  if (!is_transparent(c) || widget->getType() == kWindowWidget)
    return c;
  else if (decorative)
    return getColor(ThemeColor::Selected);
  else
    return getColor(ThemeColor::Face);
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
    Rect textWrap = textrc.createIntersect(
      // TODO add ui::Widget::getPadding() property
      // Rect(widget->getClientBounds()).shrink(widget->getBorder()));
      widget->getClientBounds()).inflate(0, 1*jguiscale());

    IntersectClip clip(g, textWrap);
    if (clip) {
      if (!widget->isEnabled()) {
        // Draw white part
        g->drawUIString(t,
          getColor(ThemeColor::Background),
          gfx::ColorNone,
          textrc.getOrigin() + Point(jguiscale(), jguiscale()));
      }

      g->drawUIString(t,
        (!widget->isEnabled() ?
          getColor(ThemeColor::Disabled):
          (gfx::geta(fg_color) > 0 ? fg_color :
            getColor(ThemeColor::Text))),
        bg_color, textrc.getOrigin());
    }
  }
}

void SkinTheme::drawEntryCaret(ui::Graphics* g, Entry* widget, int x, int y)
{
  gfx::Color color = getColor(ThemeColor::Text);
  int h = widget->getTextHeight();

  for (int u=x; u<x+2*jguiscale(); ++u)
    g->drawVLine(color, u, y-1, h+2);
}

she::Surface* SkinTheme::get_toolicon(const char* tool_id) const
{
  std::map<std::string, she::Surface*>::const_iterator it = m_toolicon.find(tool_id);
  if (it != m_toolicon.end())
    return it->second;
  else
    return NULL;
}

void SkinTheme::draw_bounds_template(Graphics* g, const Rect& rc,
                                     int nw, int n, int ne, int e, int se, int s, int sw, int w)
{
  draw_bounds_template(g, rc,
    m_part[nw],
    m_part[n],
    m_part[ne],
    m_part[e],
    m_part[se],
    m_part[s],
    m_part[sw],
    m_part[w]);
}

void SkinTheme::draw_bounds_template(ui::Graphics* g, const gfx::Rect& rc, const SkinPartPtr& skinPart)
{
  draw_bounds_template(g, rc,
    skinPart->getBitmap(0),
    skinPart->getBitmap(1),
    skinPart->getBitmap(2),
    skinPart->getBitmap(3),
    skinPart->getBitmap(4),
    skinPart->getBitmap(5),
    skinPart->getBitmap(6),
    skinPart->getBitmap(7));
}

void SkinTheme::draw_bounds_template(Graphics* g, const Rect& rc,
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

void SkinTheme::draw_bounds_array(ui::Graphics* g, const gfx::Rect& rc, int parts[8])
{
  int nw = parts[0];
  int n  = parts[1];
  int ne = parts[2];
  int e  = parts[3];
  int se = parts[4];
  int s  = parts[5];
  int sw = parts[6];
  int w  = parts[7];

  draw_bounds_template(g, rc,
    nw, n, ne, e,
    se, s, sw, w);
}

void SkinTheme::draw_bounds_nw(Graphics* g, const Rect& rc, int nw, gfx::Color bg)
{
  draw_bounds_template(g, rc,
                       nw+0, nw+1, nw+2, nw+3,
                       nw+4, nw+5, nw+6, nw+7);

  // Center
  if (!is_transparent(bg)) {
    g->fillRect(bg, Rect(rc).shrink(
        Border(
          m_part[nw+7]->width(),
          m_part[nw+1]->height(),
          m_part[nw+3]->width(),
          m_part[nw+5]->height())));
  }
}

void SkinTheme::draw_bounds_nw(ui::Graphics* g, const gfx::Rect& rc, const SkinPartPtr skinPart, gfx::Color bg)
{
  draw_bounds_template(g, rc, skinPart);

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

void SkinTheme::draw_bounds_nw2(Graphics* g, const Rect& rc, int x_mid, int nw1, int nw2, gfx::Color bg1, gfx::Color bg2)
{
  Rect rc2(rc.x, rc.y, x_mid-rc.x+1, rc.h);
  {
    IntersectClip clip(g, rc2);
    if (clip)
      draw_bounds_nw(g, rc, nw1, bg1);
  }

  rc2.x += rc2.w;
  rc2.w = rc.w - rc2.w;

  IntersectClip clip(g, rc2);
  if (clip)
    draw_bounds_nw(g, rc, nw2, bg2);
}

void SkinTheme::draw_part_as_hline(ui::Graphics* g, const gfx::Rect& rc, int part)
{
  int x;

  for (x = rc.x;
       x < rc.x2()-m_part[part]->width();
       x += m_part[part]->width()) {
    g->drawRgbaSurface(m_part[part], x, rc.y);
  }

  if (x < rc.x2()) {
    Rect rc2(x, rc.y, rc.w-(x-rc.x), m_part[part]->height());
    IntersectClip clip(g, rc2);
    if (clip)
      g->drawRgbaSurface(m_part[part], x, rc.y);
  }
}

void SkinTheme::draw_part_as_vline(ui::Graphics* g, const gfx::Rect& rc, int part)
{
  int y;

  for (y = rc.y;
       y < rc.y2()-m_part[part]->height();
       y += m_part[part]->height()) {
    g->drawRgbaSurface(m_part[part], rc.x, y);
  }

  if (y < rc.y2()) {
    Rect rc2(rc.x, y, m_part[part]->width(), rc.h-(y-rc.y));
    IntersectClip clip(g, rc2);
    if (clip)
      g->drawRgbaSurface(m_part[part], rc.x, y);
  }
}

void SkinTheme::paintProgressBar(ui::Graphics* g, const gfx::Rect& rc0, float progress)
{
  g->drawRect(getColor(ThemeColor::Text), rc0);

  gfx::Rect rc = rc0;
  rc.shrink(1);

  int u = (int)((float)rc.w*progress);
  u = MID(0, u, rc.w);

  if (u > 0)
    g->fillRect(getColor(ThemeColor::Selected), gfx::Rect(rc.x, rc.y, u, rc.h));

  if (1+u < rc.w)
    g->fillRect(getColor(ThemeColor::Background), gfx::Rect(rc.x+u, rc.y, rc.w-u, rc.h));
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

// static
she::Font* SkinTheme::loadFont(const char* userFont, const std::string& path)
{
  ResourceFinder rf;

  // Directories to find the font
  const char* user_font = get_config_string("Options", userFont, "");
  if (user_font && *user_font)
    rf.addPath(user_font);

  rf.includeDataDir(path.c_str());

  // Try to load the font
  while (rf.next()) {
    she::Font* f = she::load_bitmap_font(rf.filename().c_str(), jguiscale());
    if (f) {
      if (f->isScalable())
        f->setSize(8);

      return f;
    }
  }

  return she::instance()->defaultFont();
}

} // namespace skin
} // namespace app
