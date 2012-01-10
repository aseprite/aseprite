/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include "gui/gui.h"
#include "gui/intern.h"
#include "loadpng.h"
#include "modules/gui.h"
#include "resource_finder.h"
#include "skin/button_icon_impl.h"
#include "skin/skin_property.h"
#include "skin/skin_slider_property.h"
#include "skin/skin_theme.h"
#include "xml_exception.h"

#include "tinyxml.h"

#define CHARACTER_LENGTH(f, c)  ((f)->vtable->char_length((f), (c)))

#define BGCOLOR                 (get_bg_color(widget))

#define COLOR_FOREGROUND        makecol(0, 0, 0)
#define COLOR_DISABLED          makecol(150, 130, 117)
#define COLOR_FACE              makecol(211, 203, 190)
#define COLOR_HOTFACE           makecol(250, 240, 230)
#define COLOR_SELECTED          makecol(44, 76, 145)
#define COLOR_BACKGROUND        makecol(255, 255, 255)

static std::map<std::string, int> sheet_mapping;

static struct
{
  const char* id;
  int focusx, focusy;
} cursors_info[JI_CURSORS] = {
  { "null",             0, 0 }, // JI_CURSOR_NULL
  { "normal",           0, 0 }, // JI_CURSOR_NORMAL
  { "normal_add",       0, 0 }, // JI_CURSOR_NORMAL_ADD
  { "forbidden",        0, 0 }, // JI_CURSOR_FORBIDDEN
  { "hand",             0, 0 }, // JI_CURSOR_HAND
  { "scroll",           0, 0 }, // JI_CURSOR_SCROLL
  { "move",             0, 0 }, // JI_CURSOR_MOVE
  { "size_tl",          0, 0 }, // JI_CURSOR_SIZE_TL
  { "size_t",           0, 0 }, // JI_CURSOR_SIZE_T
  { "size_tr",          0, 0 }, // JI_CURSOR_SIZE_TR
  { "size_l",           0, 0 }, // JI_CURSOR_SIZE_L
  { "size_r",           0, 0 }, // JI_CURSOR_SIZE_R
  { "size_bl",          0, 0 }, // JI_CURSOR_SIZE_BL
  { "size_b",           0, 0 }, // JI_CURSOR_SIZE_B
  { "size_br",          0, 0 }, // JI_CURSOR_SIZE_BR
  { "rotate_tl",        0, 0 }, // JI_CURSOR_ROTATE_TL
  { "rotate_t",         0, 0 }, // JI_CURSOR_ROTATE_T
  { "rotate_tr",        0, 0 }, // JI_CURSOR_ROTATE_TR
  { "rotate_l",         0, 0 }, // JI_CURSOR_ROTATE_L
  { "rotate_r",         0, 0 }, // JI_CURSOR_ROTATE_R
  { "rotate_bl",        0, 0 }, // JI_CURSOR_ROTATE_BL
  { "rotate_b",         0, 0 }, // JI_CURSOR_ROTATE_B
  { "rotate_br",        0, 0 }, // JI_CURSOR_ROTATE_BR
  { "eyedropper",       0, 0 }, // JI_CURSOR_EYEDROPPER
};

SkinTheme::SkinTheme()
{
  this->name = "Skin Theme";
  m_selected_skin = get_config_string("Skin", "Selected", "default");
  m_minifont = font;

  // Initialize all graphics in NULL (these bitmaps are loaded from the skin)
  m_sheet_bmp = NULL;
  for (int c=0; c<JI_CURSORS; ++c)
    m_cursors[c] = NULL;
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

  reload_skin();
}

SkinTheme::~SkinTheme()
{
  for (int c=0; c<JI_CURSORS; ++c)
    if (m_cursors[c])
      destroy_bitmap(m_cursors[c]);

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

void SkinTheme::onRegenerate()
{
  scrollbar_size = 12 * jguiscale();

  desktop_color = COLOR_DISABLED;
  textbox_fg_color = COLOR_FOREGROUND;
  textbox_bg_color = COLOR_BACKGROUND;

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

        for (c=0; c<JI_CURSORS; ++c) {
          if (id != cursors_info[c].id)
            continue;

          cursors_info[c].focusx = focusx;
          cursors_info[c].focusy = focusy;

          m_cursors[c] = cropPartFromSheet(m_cursors[c], x, y, w, h, true);
          break;
        }

        if (c == JI_CURSORS) {
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

BITMAP* SkinTheme::cropPartFromSheet(BITMAP* bmp, int x, int y, int w, int h, bool cursor)
{
  int colordepth = (cursor ? bitmap_color_depth(screen): 32);

  if (bmp &&
      (bmp->w != w ||
       bmp->h != h ||
       bitmap_color_depth(bmp) != colordepth)) {
    destroy_bitmap(bmp);
    bmp = NULL;
  }

  if (!bmp)
    bmp = create_bitmap_ex(colordepth, w, h);

  if (cursor) {
    clear_to_color(bmp, bitmap_mask_color(bmp));

    set_alpha_blender();
    draw_trans_sprite(bmp, m_sheet_bmp, -x, -y);
    set_trans_blender(0, 0, 0, 0);
  }
  else {
    blit(m_sheet_bmp, bmp, x, y, 0, 0, w, h);
  }

  return ji_apply_guiscale(bmp);
}

BITMAP* SkinTheme::set_cursor(int type, int* focus_x, int* focus_y)
{
  if (type == JI_CURSOR_NULL) {
    *focus_x = 0;
    *focus_y = 0;
    return NULL;
  }
  else {
    ASSERT(type >= 0 && type < JI_CURSORS);

    *focus_x = cursors_info[type].focusx*jguiscale();
    *focus_y = cursors_info[type].focusy*jguiscale();
    return m_cursors[type];
  }
}

void SkinTheme::init_widget(JWidget widget)
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

    case JI_BOX:
      BORDER(0);
      widget->child_spacing = 4 * scale;
      break;

    case JI_BUTTON:
      BORDER4(m_part[PART_BUTTON_NORMAL_W]->w,
              m_part[PART_BUTTON_NORMAL_N]->h,
              m_part[PART_BUTTON_NORMAL_E]->w,
              m_part[PART_BUTTON_NORMAL_S]->h);
      widget->child_spacing = 0;
      break;

    case JI_CHECK:
      BORDER(2 * scale);
      widget->child_spacing = 4 * scale;

      static_cast<ButtonBase*>(widget)->setIconInterface
        (new ButtonIconImpl(static_cast<SkinTheme*>(widget->getTheme()),
                            PART_CHECK_NORMAL,
                            PART_CHECK_SELECTED,
                            PART_CHECK_DISABLED,
                            JI_LEFT | JI_MIDDLE));
      break;

    case JI_ENTRY:
      BORDER4(m_part[PART_SUNKEN_NORMAL_W]->w,
              m_part[PART_SUNKEN_NORMAL_N]->h,
              m_part[PART_SUNKEN_NORMAL_E]->w,
              m_part[PART_SUNKEN_NORMAL_S]->h);
      break;

    case JI_GRID:
      BORDER(0);
      widget->child_spacing = 4 * scale;
      break;

    case JI_LABEL:
      BORDER(1 * scale);
      break;

    case JI_LISTBOX:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case JI_LISTITEM:
      BORDER(1 * scale);
      break;

    case JI_COMBOBOX:
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

    case JI_MENU:
    case JI_MENUBAR:
    case JI_MENUBOX:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case JI_MENUITEM:
      BORDER(2 * scale);
      widget->child_spacing = 18 * scale;
      break;

    case JI_PANEL:
      BORDER(0);
      widget->child_spacing = 3 * scale;
      break;

    case JI_RADIO:
      BORDER(2 * scale);
      widget->child_spacing = 4 * scale;

      static_cast<ButtonBase*>(widget)->setIconInterface
        (new ButtonIconImpl(static_cast<SkinTheme*>(widget->getTheme()),
                            PART_RADIO_NORMAL,
                            PART_RADIO_SELECTED,
                            PART_RADIO_DISABLED,
                            JI_LEFT | JI_MIDDLE));
      break;

    case JI_SEPARATOR:
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

    case JI_SLIDER:
      BORDER4(m_part[PART_SLIDER_EMPTY_W]->w-1*scale,
              m_part[PART_SLIDER_EMPTY_N]->h,
              m_part[PART_SLIDER_EMPTY_E]->w-1*scale,
              m_part[PART_SLIDER_EMPTY_S]->h-1*scale);
      widget->child_spacing = jwidget_get_text_height(widget);
      widget->setAlign(JI_CENTER | JI_MIDDLE);
      break;

    case JI_TEXTBOX:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case JI_VIEW:
      BORDER4(m_part[PART_SUNKEN_NORMAL_W]->w-1*scale,
              m_part[PART_SUNKEN_NORMAL_N]->h,
              m_part[PART_SUNKEN_NORMAL_E]->w-1*scale,
              m_part[PART_SUNKEN_NORMAL_S]->h-1*scale);
      widget->child_spacing = 0;
      break;

    case JI_VIEW_SCROLLBAR:
      BORDER(1 * scale);
      widget->child_spacing = 0;
      break;

    case JI_VIEW_VIEWPORT:
      BORDER(0);
      widget->child_spacing = 0;
      break;

    case JI_FRAME:
      if (!static_cast<Frame*>(widget)->is_desktop()) {
        if (widget->hasText()) {
          BORDER4(6 * scale, (4+6) * scale, 6 * scale, 6 * scale);
          widget->border_width.t += jwidget_get_text_height(widget);

#if 1                           /* add close button */
          if (!(widget->flags & JI_INITIALIZED)) {
            Button* button = new Button("");
            setup_bevels(button, 0, 0, 0, 0);
            jwidget_add_hook(button, JI_WIDGET,
                             &SkinTheme::theme_frame_button_msg_proc, NULL);
            jwidget_decorative(button, true);
            widget->addChild(button);
            button->setName("theme_close_button");
            button->Click.connect(Bind<void>(&Frame::closeWindow, (Frame*)widget, button));
          }
#endif
        }
        else {
          BORDER(3 * scale);
        }
      }
      else {
        BORDER(0);
      }
      widget->child_spacing = 4 * scale;
      widget->setBgColor(get_window_face_color());
      break;

    default:
      break;
  }
}

JRegion SkinTheme::get_window_mask(JWidget widget)
{
  return jregion_new(widget->rc, 1);
}

void SkinTheme::map_decorative_widget(JWidget widget)
{
  if (widget->name != NULL &&
      strcmp(widget->name, "theme_close_button") == 0) {
    JWidget window = widget->parent;
    JRect rect = jrect_new(0, 0, 0, 0);

    rect->x2 = m_part[PART_WINDOW_CLOSE_BUTTON_NORMAL]->w;
    rect->y2 = m_part[PART_WINDOW_CLOSE_BUTTON_NORMAL]->h;

    jrect_displace(rect,
                   window->rc->x2 - 3 - jrect_w(rect),
                   window->rc->y1 + 3);

    jwidget_set_rect(widget, rect);
    jrect_free(rect);
  }
}

int SkinTheme::color_foreground()
{
  return COLOR_FOREGROUND;
}

int SkinTheme::color_disabled()
{
  return COLOR_DISABLED;
}

int SkinTheme::color_face()
{
  return COLOR_FACE;
}

int SkinTheme::color_hotface()
{
  return COLOR_HOTFACE;
}

int SkinTheme::color_selected()
{
  return COLOR_SELECTED;
}

int SkinTheme::color_background()
{
  return COLOR_BACKGROUND;
}

void SkinTheme::paintBox(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  g->fillRect(BGCOLOR, g->getClipBounds());
}

void SkinTheme::paintButton(PaintEvent& ev)
{
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  IButtonIcon* iconInterface = widget->getIconInterface();
  struct jrect box, text, icon;
  int x1, y1, x2, y2;
  int fg, bg, part_nw;
  JRect crect;

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
    fg = get_button_selected_text_color();
    bg = get_button_selected_face_color();
    part_nw = (look == MiniLook ? PART_TOOLBUTTON_NORMAL_NW:
               look == LeftButtonLook ? PART_DROP_DOWN_BUTTON_LEFT_SELECTED_NW:
               look == RightButtonLook ? PART_DROP_DOWN_BUTTON_RIGHT_SELECTED_NW:
                                         PART_BUTTON_SELECTED_NW);
  }
  // With mouse
  else if (widget->isEnabled() && widget->hasMouseOver()) {
    fg = get_button_hot_text_color();
    bg = get_button_hot_face_color();
    part_nw = (look == MiniLook ? PART_TOOLBUTTON_HOT_NW:
               look == LeftButtonLook ? PART_DROP_DOWN_BUTTON_LEFT_HOT_NW:
               look == RightButtonLook ? PART_DROP_DOWN_BUTTON_RIGHT_HOT_NW:
                                         PART_BUTTON_HOT_NW);
  }
  // Without mouse
  else {
    fg = get_button_normal_text_color();
    bg = get_button_normal_face_color();

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

  // widget position
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  // external background
  rectfill(ji_screen, x1, y1, x2, y2, BGCOLOR);

  // draw borders
  draw_bounds_nw(ji_screen, x1, y1, x2, y2, part_nw, bg);

  // text
  crect = jwidget_get_child_rect(widget);
  draw_textstring(NULL, fg, bg, false, widget, crect, get_button_selected_offset());
  jrect_free(crect);

  // Paint the icon
  if (iconInterface) {
    if (widget->isSelected())
      jrect_displace(&icon,
                     get_button_selected_offset(),
                     get_button_selected_offset());

    paintIcon(widget, ev.getGraphics(), iconInterface, icon.x1-widget->rc->x1, icon.y1-widget->rc->y1);
  }
}

void SkinTheme::paintCheckBox(PaintEvent& ev)
{
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  IButtonIcon* iconInterface = widget->getIconInterface();
  struct jrect box, text, icon;
  int bg;

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
      jdraw_rectfill(widget->rc, bg = get_check_hot_face_color());
    else if (widget->hasFocus())
      jdraw_rectfill(widget->rc, bg = get_check_focus_face_color());
  }

  // Text
  draw_textstring(NULL, -1, bg, false, widget, &text, 0);

  // Paint the icon
  if (iconInterface)
    paintIcon(widget, ev.getGraphics(), iconInterface, icon.x1-widget->rc->x1, icon.y1-widget->rc->y1);

  // draw focus
  if (look != WithoutBordersLook && widget->hasFocus()) {
    draw_bounds_nw(ji_screen,
                   widget->rc->x1,
                   widget->rc->y1,
                   widget->rc->x2-1,
                   widget->rc->y2-1, PART_CHECK_FOCUS_NW, -1);
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
  const char *text = widget->getText();
  int c, ch, x, y, w, fg, bg;
  int x1, y1, x2, y2;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  /* main pos */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  bg = COLOR_BACKGROUND;

  bool isMiniLook = false;
  SharedPtr<SkinProperty> skinPropery = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinPropery != NULL)
    isMiniLook = (skinPropery->getLook() == MiniLook);

  draw_bounds_nw(ji_screen,
                 x1, y1, x2, y2,
                 (widget->hasFocus() ?
                  (isMiniLook ? PART_SUNKEN_MINI_FOCUSED_NW: PART_SUNKEN_FOCUSED_NW):
                  (isMiniLook ? PART_SUNKEN_MINI_NORMAL_NW : PART_SUNKEN_NORMAL_NW)), bg);

  /* draw the text */
  x = widget->rc->x1 + widget->border_width.l;
  y = (widget->rc->y1+widget->rc->y2)/2 - jwidget_get_text_height(widget)/2;

  for (c=scroll; ugetat(text, c); c++) {
    ch = password ? '*': ugetat(text, c);

    /* normal text */
    bg = -1;
    fg = COLOR_FOREGROUND;

    /* selected */
    if ((c >= selbeg) && (c <= selend)) {
      if (widget->hasFocus())
        bg = COLOR_SELECTED;
      else
        bg = COLOR_DISABLED;
      fg = COLOR_BACKGROUND;
    }

    /* disabled */
    if (!widget->isEnabled()) {
      bg = -1;
      fg = COLOR_DISABLED;
    }

    w = CHARACTER_LENGTH(widget->getFont(), ch);
    if (x+w > widget->rc->x2-3)
      return;

    caret_x = x;
    ji_font_set_aa_mode(widget->getFont(), bg >= 0 ? bg: COLOR_BACKGROUND);
    widget->getFont()->vtable->render_char(widget->getFont(),
                                           ch, fg, bg, ji_screen, x, y);
    x += w;

    /* caret */
    if ((c == caret) && (state) && (widget->hasFocus()))
      draw_entry_caret(widget, caret_x, y);
  }

  /* draw the caret if it is next of the last character */
  if ((c == caret) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled()))
    draw_entry_caret(widget, x, y);
}

void SkinTheme::paintLabel(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Label* widget = static_cast<Label*>(ev.getSource());
  struct jrect text;
  int bg = BGCOLOR;
  int fg = widget->getTextColor();
  gfx::Rect rc = widget->getClientBounds();

  g->fillRect(bg, rc);
  rc.shrink(widget->getBorder());

  jwidget_get_texticon_info(widget, NULL, &text, NULL, 0, 0, 0);

  g->drawString(widget->getText(), fg, bg, false,
                // TODO "text" coordinates are absolute and we are drawing on client area
                gfx::Point(text.x1, text.y1) - gfx::Point(widget->rc->x1, widget->rc->y1));
}

void SkinTheme::paintLinkLabel(PaintEvent& ev)
{
  Widget* widget = static_cast<Widget*>(ev.getSource());
  int bg = BGCOLOR;

  jdraw_rectfill(widget->rc, bg);

  draw_textstring(NULL, makecol(0, 0, 255), bg, false, widget, widget->rc, 0);

  if (widget->hasMouseOver()) {
    int w = jwidget_get_text_length(widget);
    //int h = jwidget_get_text_height(widget);

    hline(ji_screen,
          widget->rc->x1, widget->rc->y2-1, widget->rc->x1+w-1, makecol(0, 0, 255));
  }
}

void SkinTheme::draw_listbox(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, COLOR_BACKGROUND);
}

void SkinTheme::draw_listitem(JWidget widget, JRect clip)
{
  int fg, bg;
  int x, y;

  if (!widget->isEnabled()) {
    bg = COLOR_FACE;
    fg = COLOR_DISABLED;
  }
  else if (widget->isSelected()) {
    fg = get_listitem_selected_text_color();
    bg = get_listitem_selected_face_color();
  }
  else {
    fg = get_listitem_normal_text_color();
    bg = get_listitem_normal_face_color();
  }

  x = widget->rc->x1+widget->border_width.l;
  y = widget->rc->y1+widget->border_width.t;

  if (widget->hasText()) {
    /* text */
    jdraw_text(ji_screen, widget->getFont(), widget->getText(), x, y, fg, bg, true, jguiscale());

    /* background */
    jrectexclude
      (ji_screen,
       widget->rc->x1, widget->rc->y1,
       widget->rc->x2-1, widget->rc->y2-1,
       x, y,
       x+jwidget_get_text_length(widget)-1,
       y+jwidget_get_text_height(widget)-1, bg);
  }
  /* background */
  else {
    jdraw_rectfill(widget->rc, bg);
  }
}

void SkinTheme::draw_menu(Menu* widget, JRect clip)
{
  jdraw_rectfill(widget->rc, BGCOLOR);
}

void SkinTheme::draw_menuitem(MenuItem* widget, JRect clip)
{
  int c, bg, fg, bar;
  int x1, y1, x2, y2;
  JRect pos;

  /* TODO ASSERT? */
  if (!widget->parent->parent)
    return;

  bar = (widget->parent->parent->type == JI_MENUBAR);

  /* colors */
  if (!widget->isEnabled()) {
    fg = -1;
    bg = get_menuitem_normal_face_color();
  }
  else {
    if (widget->isHighlighted()) {
      fg = get_menuitem_highlight_text_color();
      bg = get_menuitem_highlight_face_color();
    }
    else if (widget->hasMouseOver()) {
      fg = get_menuitem_hot_text_color();
      bg = get_menuitem_hot_face_color();
    }
    else {
      fg = get_menuitem_normal_text_color();
      bg = get_menuitem_normal_face_color();
    }
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* background */
  rectfill(ji_screen, x1, y1, x2, y2, bg);

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

  pos = jwidget_get_rect(widget);
  if (!bar)
    jrect_displace(pos, widget->child_spacing/2, 0);
  draw_textstring(NULL, fg, bg, false, widget, pos, 0);
  jrect_free(pos);

  /* for menu-box */
  if (!bar) {
    /* draw the arrown (to indicate which this menu has a sub-menu) */
    if (widget->getSubmenu()) {
      int scale = jguiscale();

      /* enabled */
      if (widget->isEnabled()) {
        for (c=0; c<3*scale; c++)
          vline(ji_screen,
                widget->rc->x2-3*scale-c,
                (widget->rc->y1+widget->rc->y2)/2-c,
                (widget->rc->y1+widget->rc->y2)/2+c, fg);
      }
      /* disabled */
      else {
        for (c=0; c<3*scale; c++)
          vline(ji_screen,
                widget->rc->x2-3*scale-c+1,
                (widget->rc->y1+widget->rc->y2)/2-c+1,
                (widget->rc->y1+widget->rc->y2)/2+c+1, COLOR_BACKGROUND);
        for (c=0; c<3*scale; c++)
          vline(ji_screen,
                widget->rc->x2-3*scale-c,
                (widget->rc->y1+widget->rc->y2)/2-c,
                (widget->rc->y1+widget->rc->y2)/2+c, COLOR_DISABLED);
      }
    }
    /* draw the keyboard shortcut */
    else if (widget->getAccel()) {
      int old_align = widget->getAlign();
      char buf[256];

      pos = jwidget_get_rect(widget);
      pos->x2 -= widget->child_spacing/4;

      jaccel_to_string(widget->getAccel(), buf);

      widget->setAlign(JI_RIGHT | JI_MIDDLE);
      draw_textstring(buf, fg, bg, false, widget, pos, 0);
      widget->setAlign(old_align);

      jrect_free(pos);
    }
  }
}

void SkinTheme::draw_panel(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, get_panel_face_color());
}

void SkinTheme::paintRadioButton(PaintEvent& ev)
{
  ButtonBase* widget = static_cast<ButtonBase*>(ev.getSource());
  IButtonIcon* iconInterface = widget->getIconInterface();
  struct jrect box, text, icon;
  int bg = BGCOLOR;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
                            iconInterface ? iconInterface->getIconAlign(): 0,
                            iconInterface ? iconInterface->getWidth() : 0,
                            iconInterface ? iconInterface->getHeight() : 0);

  /* background */
  jdraw_rectfill(widget->rc, bg);

  /* mouse */
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      jdraw_rectfill(widget->rc, bg = get_radio_hot_face_color());
    else if (widget->hasFocus())
      jdraw_rectfill(widget->rc, bg = get_radio_focus_face_color());
  }

  /* text */
  draw_textstring(NULL, -1, bg, false, widget, &text, 0);

  // Paint the icon
  if (iconInterface)
    paintIcon(widget, ev.getGraphics(), iconInterface, icon.x1-widget->rc->x1, icon.y1-widget->rc->y1);

  // draw focus
  if (widget->hasFocus()) {
    draw_bounds_nw(ji_screen,
                   widget->rc->x1,
                   widget->rc->y1,
                   widget->rc->x2-1,
                   widget->rc->y2-1, PART_RADIO_FOCUS_NW, -1);
  }
}

void SkinTheme::draw_separator(JWidget widget, JRect clip)
{
  int x1, y1, x2, y2;

  // frame position
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
    struct jrect r = { x1+h/2, y1-h/2, x2+1-h, y2+1+h };
    draw_textstring(NULL, COLOR_SELECTED, BGCOLOR, false, widget, &r, 0);
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

  widget->getSliderThemeInfo(&min, &max, &value);

  gfx::Rect rc(widget->getClientBounds().shrink(widget->getBorder()));
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
    rc.shrink(gfx::Border(3 * jguiscale(),
                          thumb->h,
                          3 * jguiscale(),
                          1 * jguiscale()));

    draw_bounds_nw(g, rc, nw, -1);

    // Draw background (using the customized ISliderBgPainter implementation)
    rc.shrink(gfx::Border(1, 1, 1, 2) * jguiscale());
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
      draw_bounds_nw(g, rc, empty_part_nw, get_slider_empty_face_color());
    else if (value == max)
      draw_bounds_nw(g, rc, full_part_nw, get_slider_full_face_color());
    else
      draw_bounds_nw2(g, rc, x,
                      full_part_nw, empty_part_nw,
                      get_slider_full_face_color(),
                      get_slider_empty_face_color());

    // Draw text
    std::string old_text = widget->getText();

    usprintf(buf, "%d", value);

    widget->setTextQuiet(buf);

    if (IntersectClip clip = IntersectClip(g, gfx::Rect(rc.x, rc.y, x-rc.x, rc.h))) {
      draw_textstring(g, NULL,
                      get_slider_full_text_color(),
                      get_slider_full_face_color(), false, widget, rc, 0);
    }

    if (IntersectClip clip = IntersectClip(g, gfx::Rect(x+1, rc.y, rc.w-(x-rc.x+1), rc.h))) {
      draw_textstring(g, NULL,
                      get_slider_empty_text_color(),
                      get_slider_empty_face_color(), false, widget, rc, 0);
    }

    widget->setTextQuiet(old_text.c_str());
  }
}

void SkinTheme::draw_combobox_entry(Entry* widget, JRect clip)
{
  bool password = widget->isPassword();
  int scroll, caret, state, selbeg, selend;
  const char *text = widget->getText();
  int c, ch, x, y, w, fg, bg;
  int x1, y1, x2, y2;
  int caret_x;

  widget->getEntryThemeInfo(&scroll, &caret, &state, &selbeg, &selend);

  /* main pos */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  bg = COLOR_BACKGROUND;

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
    bg = -1;
    fg = COLOR_FOREGROUND;

    /* selected */
    if ((c >= selbeg) && (c <= selend)) {
      if (widget->hasFocus())
        bg = COLOR_SELECTED;
      else
        bg = COLOR_DISABLED;
      fg = COLOR_BACKGROUND;
    }

    // Disabled
    if (!widget->isEnabled()) {
      bg = -1;
      fg = COLOR_DISABLED;
    }

    w = CHARACTER_LENGTH(widget->getFont(), ch);
    if (x+w > widget->rc->x2-3)
      return;

    caret_x = x;
    ji_font_set_aa_mode(widget->getFont(), bg >= 0 ? bg: COLOR_BACKGROUND);
    widget->getFont()->vtable->render_char(widget->getFont(),
                                           ch, fg, bg, ji_screen, x, y);
    x += w;

    /* caret */
    if ((c == caret) && (state) && (widget->hasFocus()))
      draw_entry_caret(widget, caret_x, y);
  }

  /* draw the caret if it is next of the last character */
  if ((c == caret) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled()))
    draw_entry_caret(widget, x, y);
}

void SkinTheme::paintComboBoxButton(PaintEvent& ev)
{
  Button* widget = static_cast<Button*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  IButtonIcon* iconInterface = widget->getIconInterface();
  int bg, part_nw;

  if (widget->isSelected()) {
    bg = get_button_selected_face_color();
    part_nw = PART_TOOLBUTTON_PUSHED_NW;
  }
  // With mouse
  else if (widget->isEnabled() && widget->hasMouseOver()) {
    bg = get_button_hot_face_color();
    part_nw = PART_TOOLBUTTON_HOT_NW;
  }
  // Without mouse
  else {
    bg = get_button_normal_face_color();
    part_nw = PART_TOOLBUTTON_LAST_NW;
  }

  gfx::Rect rc = widget->getClientBounds();

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

void SkinTheme::draw_textbox(JWidget widget, JRect clip)
{
  _ji_theme_textbox_draw(ji_screen, widget, NULL, NULL,
                         this->textbox_bg_color,
                         this->textbox_fg_color);
}

void SkinTheme::draw_view(JWidget widget, JRect clip)
{
  draw_bounds_nw(ji_screen,
                 widget->rc->x1,
                 widget->rc->y1,
                 widget->rc->x2-1,
                 widget->rc->y2-1,
                 widget->hasFocus() ? PART_SUNKEN_FOCUSED_NW:
                                      PART_SUNKEN_NORMAL_NW,
                 COLOR_BACKGROUND);
}

void SkinTheme::draw_view_scrollbar(JWidget _widget, JRect clip)
{
  ScrollBar* widget = static_cast<ScrollBar*>(_widget);
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int pos, len;

  widget->getScrollBarThemeInfo(&pos, &len);

  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  draw_bounds_nw(ji_screen,
                 x1, y1, x2, y2,
                 PART_SCROLLBAR_BG_NW,
                 get_scrollbar_bg_face_color());

  // Horizontal bar
  if (widget->getAlign() & JI_HORIZONTAL) {
    u1 = x1+pos;
    v1 = y1;
    u2 = x1+pos+len-1;
    v2 = y2;
  }
  // Vertical bar
  else {
    u1 = x1;
    v1 = y1+pos;
    u2 = x2;
    v2 = y1+pos+len-1;
  }

  draw_bounds_nw(ji_screen,
                 u1, v1, u2, v2,
                 PART_SCROLLBAR_THUMB_NW,
                 get_scrollbar_thumb_face_color());
}

void SkinTheme::draw_view_viewport(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, BGCOLOR);
}

void SkinTheme::paintFrame(PaintEvent& ev)
{
  Frame* window = static_cast<Frame*>(ev.getSource());
  JRect pos = jwidget_get_rect(window);
  JRect cpos = jwidget_get_child_rect(window);

  if (!window->is_desktop()) {
    // window frame
    if (window->hasText()) {
      draw_bounds_nw(ji_screen,
                     pos->x1,
                     pos->y1,
                     pos->x2-1,
                     pos->y2-1, PART_WINDOW_NW,
                     get_window_face_color());

      pos->y2 = cpos->y1;

      // titlebar
      jdraw_text(ji_screen, window->getFont(), window->getText(),
                 cpos->x1,
                 pos->y1+jrect_h(pos)/2-text_height(window->getFont())/2,
                 COLOR_BACKGROUND, -1, false, jguiscale());
    }
    // menubox
    else {
      draw_bounds_nw(ji_screen,
                     pos->x1,
                     pos->y1,
                     pos->x2-1,
                     pos->y2-1, PART_MENU_NW,
                     get_window_face_color());
    }
  }
  // desktop
  else {
    jdraw_rectfill(pos, this->desktop_color);
  }

  jrect_free(pos);
  jrect_free(cpos);
}

void SkinTheme::draw_frame_button(ButtonBase* widget, JRect clip)
{
  int part;

  if (widget->isSelected())
    part = PART_WINDOW_CLOSE_BUTTON_SELECTED;
  else if (widget->hasMouseOver())
    part = PART_WINDOW_CLOSE_BUTTON_HOT;
  else
    part = PART_WINDOW_CLOSE_BUTTON_NORMAL;

  set_alpha_blender();
  draw_trans_sprite(ji_screen, m_part[part], widget->rc->x1, widget->rc->y1);
}

void SkinTheme::paintTooltip(PaintEvent& ev)
{
  TipWindow* widget = static_cast<TipWindow*>(ev.getSource());
  Graphics* g = ev.getGraphics();
  gfx::Rect rc = widget->getClientBounds();
  int bg = makecol(255, 255, 125);

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
  g->fillRect(bg, gfx::Rect(rc).shrink(gfx::Border(m_part[w]->w,
                                                   m_part[n]->h,
                                                   m_part[e]->w,
                                                   m_part[s]->h)));

  rc.shrink(widget->getBorder());

  g->drawString(widget->getText(), ji_color_foreground(), bg, rc, widget->getAlign());
}

int SkinTheme::get_bg_color(JWidget widget)
{
  int c = jwidget_get_bg_color(widget);
  int decorative = jwidget_is_decorative(widget);

  return c >= 0 ? c: (decorative ? COLOR_SELECTED:
                                   COLOR_FACE);
}

void SkinTheme::draw_textstring(const char *t, int fg_color, int bg_color,
                                bool fill_bg, JWidget widget, const JRect rect,
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
    if (bg_color >= 0) {
      if (!widget->isEnabled())
        rectfill(ji_screen, x, y, x+w-1+jguiscale(), y+h-1+jguiscale(), bg_color);
      else
        rectfill(ji_screen, x, y, x+w-1, y+h-1, bg_color);
    }

    /* text */
    if (!widget->isEnabled()) {
      /* TODO avoid this */
      if (fill_bg)              /* only to draw the background */
        jdraw_text(ji_screen, widget->getFont(), t, x, y, 0, bg_color, fill_bg, jguiscale());

      /* draw white part */
      jdraw_text(ji_screen, widget->getFont(), t, x+jguiscale(), y+jguiscale(),
                 COLOR_BACKGROUND, bg_color, fill_bg, jguiscale());

      if (fill_bg)
        fill_bg = false;
    }

    jdraw_text(ji_screen, widget->getFont(), t, x, y,
               !widget->isEnabled() ?
               COLOR_DISABLED: (fg_color >= 0 ? fg_color :
                                                COLOR_FOREGROUND),
               bg_color, fill_bg, jguiscale());
  }
}

void SkinTheme::draw_textstring(Graphics* g, const char *t, int fg_color, int bg_color,
                                bool fill_bg, JWidget widget, const gfx::Rect& rc,
                                int selected_offset)
{
  if (t || widget->hasText()) {
    gfx::Rect textrc;

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
    if (bg_color >= 0) {
      if (!widget->isEnabled())
        g->fillRect(bg_color, gfx::Rect(textrc).inflate(jguiscale(), jguiscale()));
      else
        g->fillRect(bg_color, textrc);
    }

    // Text
    if (!widget->isEnabled()) {
      // TODO avoid this
      if (fill_bg)              // Only to draw the background
        g->drawString(t, 0, bg_color, fill_bg, textrc.getOrigin());

      // Draw white part
      g->drawString(t, COLOR_BACKGROUND, bg_color, fill_bg,
                    textrc.getOrigin() + gfx::Point(jguiscale(), jguiscale()));

      if (fill_bg)
        fill_bg = false;
    }

    g->drawString(t,
                  !widget->isEnabled() ?
                  COLOR_DISABLED: (fg_color >= 0 ? fg_color :
                                                   COLOR_FOREGROUND),
                  bg_color, fill_bg, textrc.getOrigin());
  }
}

void SkinTheme::draw_entry_caret(Entry* widget, int x, int y)
{
  int h = jwidget_get_text_height(widget);

  vline(ji_screen, x,   y-1, y+h, COLOR_FOREGROUND);
  vline(ji_screen, x+1, y-1, y+h, COLOR_FOREGROUND);
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

void SkinTheme::draw_bounds_template(Graphics* g, const gfx::Rect& rc,
                                     int nw, int n, int ne, int e, int se, int s, int sw, int w)
{
  int x, y;

  // Top

  g->drawAlphaBitmap(m_part[nw], rc.x, rc.y);

  if (IntersectClip clip = IntersectClip(g, gfx::Rect(rc.x+m_part[nw]->w, rc.y,
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

  if (IntersectClip clip = IntersectClip(g, gfx::Rect(rc.x+m_part[sw]->w, rc.y,
                                                      rc.w-m_part[sw]->w-m_part[se]->w, rc.h))) {
    for (x = rc.x+m_part[sw]->w;
         x < rc.x+rc.w-m_part[se]->w;
         x += m_part[s]->w) {
      g->drawAlphaBitmap(m_part[s], x, rc.y+rc.h-m_part[s]->h);
    }
  }

  g->drawAlphaBitmap(m_part[se], rc.x+rc.w-m_part[se]->w, rc.y+rc.h-m_part[se]->h);

  if (IntersectClip clip = IntersectClip(g, gfx::Rect(rc.x, rc.y+m_part[nw]->h,
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

void SkinTheme::draw_bounds_nw(BITMAP* bmp, int x1, int y1, int x2, int y2, int nw, int bg)
{
  set_alpha_blender();
  draw_bounds_template(bmp, x1, y1, x2, y2,
                       nw+0, nw+1, nw+2, nw+3,
                       nw+4, nw+5, nw+6, nw+7);

  // Center
  if (bg >= 0) {
    x1 += m_part[nw+7]->w;
    y1 += m_part[nw+1]->h;
    x2 -= m_part[nw+3]->w;
    y2 -= m_part[nw+5]->h;

    rectfill(bmp, x1, y1, x2, y2, bg);
  }
}

void SkinTheme::draw_bounds_nw(Graphics* g, const gfx::Rect& rc, int nw, int bg)
{
  draw_bounds_template(g, rc,
                       nw+0, nw+1, nw+2, nw+3,
                       nw+4, nw+5, nw+6, nw+7);

  // Center
  if (bg >= 0) {
    g->fillRect(bg, gfx::Rect(rc).shrink(gfx::Border(m_part[nw+7]->w,
                                                     m_part[nw+1]->h,
                                                     m_part[nw+3]->w,
                                                     m_part[nw+5]->h)));
  }
}

void SkinTheme::draw_bounds_nw2(Graphics* g, const gfx::Rect& rc, int x_mid, int nw1, int nw2, int bg1, int bg2)
{
  gfx::Rect rc2(rc.x, rc.y, x_mid-rc.x+1, rc.h);

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

void SkinTheme::draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel)
{
  hline(ji_screen, x1+bevel[0], y1, x2-bevel[1], c1); /* top */
  hline(ji_screen, x1+bevel[2], y2, x2-bevel[3], c2); /* bottom */

  vline(ji_screen, x1, y1+bevel[0], y2-bevel[2], c1); /* left */
  vline(ji_screen, x2, y1+bevel[1], y2-bevel[3], c2); /* right */

  line(ji_screen, x1, y1+bevel[0], x1+bevel[0], y1, c1); /* top-left */
  line(ji_screen, x1, y2-bevel[2], x1+bevel[2], y2, c2); /* bottom-left */

  line(ji_screen, x2-bevel[1], y1, x2, y1+bevel[1], c2); /* top-right */
  line(ji_screen, x2-bevel[3], y2, x2, y2-bevel[3], c2); /* bottom-right */
}

void SkinTheme::less_bevel(int *bevel)
{
  if (bevel[0] > 0) --bevel[0];
  if (bevel[1] > 0) --bevel[1];
  if (bevel[2] > 0) --bevel[2];
  if (bevel[3] > 0) --bevel[3];
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

/* controls the "X" button in a window to close it */
bool SkinTheme::theme_frame_button_msg_proc(JWidget widget, Message* msg)
{
  switch (msg->type) {

    case JM_SETCURSOR:
      jmouse_set_cursor(JI_CURSOR_NORMAL);
      return true;

    case JM_DRAW:
      {
        ButtonBase* button = dynamic_cast<ButtonBase*>(widget);
        ASSERT(button && "theme_frame_button_msg_proc() must be hooked in a ButtonBase widget");

        ((SkinTheme*)button->getTheme())
          ->draw_frame_button(button, &msg->draw.rect);
      }
      return true;

    case JM_KEYPRESSED:
      if (msg->key.scancode == KEY_ESC) {
        widget->setSelected(true);
        return true;
      }
      break;

    case JM_KEYRELEASED:
      if (msg->key.scancode == KEY_ESC) {
        if (widget->isSelected()) {
          widget->setSelected(false);
          widget->closeWindow();
          return true;
        }
      }
      break;
  }

  return false;
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
