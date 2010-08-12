/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "Vaca/SharedPtr.h"
#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "ase_exception.h"
#include "loadpng.h"
#include "modules/gui.h"
#include "modules/skinneable_theme.h"
#include "resource_finder.h"

#include "tinyxml.h"

#define CHARACTER_LENGTH(f, c)	((f)->vtable->char_length((f), (c)))

#define BGCOLOR			(get_bg_color(widget))

#define COLOR_FOREGROUND	makecol(0, 0, 0)
#define COLOR_DISABLED		makecol(150, 130, 117)
#define COLOR_FACE		makecol(211, 203, 190)
#define COLOR_HOTFACE		makecol(250, 240, 230)
#define COLOR_SELECTED		makecol(44, 76, 145)
#define COLOR_BACKGROUND	makecol(255, 255, 255)

static std::map<std::string, int> sheet_mapping;

static struct
{
  const char* id;
  int focusx, focusy;
} cursors_info[JI_CURSORS] = {
  { "null",		0, 0 },	// JI_CURSOR_NULL
  { "normal",		0, 0 },	// JI_CURSOR_NORMAL
  { "normal_add",	0, 0 },	// JI_CURSOR_NORMAL_ADD
  { "forbidden",	0, 0 },	// JI_CURSOR_FORBIDDEN
  { "hand",		0, 0 },	// JI_CURSOR_HAND
  { "scroll",		0, 0 },	// JI_CURSOR_SCROLL
  { "move",		0, 0 },	// JI_CURSOR_MOVE
  { "size_tl",		0, 0 },	// JI_CURSOR_SIZE_TL
  { "size_t",		0, 0 },	// JI_CURSOR_SIZE_T
  { "size_tr",		0, 0 },	// JI_CURSOR_SIZE_TR
  { "size_l",		0, 0 },	// JI_CURSOR_SIZE_L
  { "size_r",		0, 0 },	// JI_CURSOR_SIZE_R
  { "size_bl",		0, 0 },	// JI_CURSOR_SIZE_BL
  { "size_b",		0, 0 },	// JI_CURSOR_SIZE_B
  { "size_br",		0, 0 },	// JI_CURSOR_SIZE_BR
  { "eyedropper",	0, 0 },	// JI_CURSOR_EYEDROPPER
};

SkinneableTheme::SkinneableTheme()
{
  this->name = "Skinneable Theme";
  m_selected_skin = get_config_string("Skin", "Selected", "default_skin");

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
  sheet_mapping["separator_horz"] = PART_SEPARATOR_HORZ;
  sheet_mapping["separator_vert"] = PART_SEPARATOR_VERT;
  sheet_mapping["combobox_arrow"] = PART_COMBOBOX_ARROW;
  sheet_mapping["toolbutton_normal"] = PART_TOOLBUTTON_NORMAL_NW;
  sheet_mapping["toolbutton_hot"] = PART_TOOLBUTTON_HOT_NW;
  sheet_mapping["toolbutton_last"] = PART_TOOLBUTTON_LAST_NW;
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

  reload_skin();
}

SkinneableTheme::~SkinneableTheme()
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
}

// Call ji_regen_theme after this
void SkinneableTheme::reload_skin()
{
  if (m_sheet_bmp) {
    destroy_bitmap(m_sheet_bmp);
    m_sheet_bmp = NULL;
  }

  // Load the skin sheet
  std::string sheet_filename = ("skins/" + m_selected_skin + "/sheet.png");
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
    throw ase_exception("Error loading %s file", sheet_filename.c_str());
}

std::string SkinneableTheme::get_font_filename() const
{
  return "skins/" + m_selected_skin + "/font.pcx";
}

void SkinneableTheme::regen()
{
  check_icon_size = 8 * jguiscale();
  radio_icon_size = 8 * jguiscale();
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
      throw ase_exception(&doc);

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
	  throw ase_exception("Unknown cursor specified in '%s':\n"
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
	int c = sheet_mapping[part_id];

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

BITMAP* SkinneableTheme::cropPartFromSheet(BITMAP* bmp, int x, int y, int w, int h, bool cursor)
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

BITMAP* SkinneableTheme::set_cursor(int type, int* focus_x, int* focus_y)
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

void SkinneableTheme::init_widget(JWidget widget)
{
#define BORDER(n)			\
  widget->border_width.l = (n);		\
  widget->border_width.t = (n);		\
  widget->border_width.r = (n);		\
  widget->border_width.b = (n);

#define BORDER4(L,T,R,B)		\
  widget->border_width.l = (L);		\
  widget->border_width.t = (T);		\
  widget->border_width.r = (R);		\
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

	Widget* button = combobox->getButtonWidget();

	button->border_width.l = 0;
	button->border_width.t = 0;
	button->border_width.r = 0;
	button->border_width.b = 0;
	button->child_spacing = 0;

	if (!(widget->flags & JI_INITIALIZED)) {
	  jwidget_add_hook(button, JI_WIDGET,
			   &SkinneableTheme::theme_combobox_button_msg_proc, NULL);
	}
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

#if 1				/* add close button */
	  if (!(widget->flags & JI_INITIALIZED)) {
	    JWidget button = jbutton_new("");
	    jbutton_set_bevel(button, 0, 0, 0, 0);
	    jwidget_add_hook(button, JI_WIDGET,
			     &SkinneableTheme::theme_frame_button_msg_proc, NULL);
	    jwidget_decorative(button, true);
	    jwidget_add_child(widget, button);
	    button->setName("theme_close_button");
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

JRegion SkinneableTheme::get_window_mask(JWidget widget)
{
  return jregion_new(widget->rc, 1);
}

void SkinneableTheme::map_decorative_widget(JWidget widget)
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

int SkinneableTheme::color_foreground()
{
  return COLOR_FOREGROUND;
}

int SkinneableTheme::color_disabled()
{
  return COLOR_DISABLED;
}

int SkinneableTheme::color_face()
{
  return COLOR_FACE;
}

int SkinneableTheme::color_hotface()
{
  return COLOR_HOTFACE;
}

int SkinneableTheme::color_selected()
{
  return COLOR_SELECTED;
}

int SkinneableTheme::color_background()
{
  return COLOR_BACKGROUND;
}

void SkinneableTheme::draw_box(JWidget widget, JRect clip)
{
  jdraw_rectfill(clip, BGCOLOR);
}

void SkinneableTheme::draw_button(JWidget widget, JRect clip)
{
  BITMAP *icon_bmp = ji_generic_button_get_icon(widget);
  int icon_align = ji_generic_button_get_icon_align(widget);
  struct jrect box, text, icon;
  int x1, y1, x2, y2;
  int fg, bg, part_nw;
  JRect crect;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			    icon_align,
			    icon_bmp ? icon_bmp->w : 0,
			    icon_bmp ? icon_bmp->h : 0);

  // Tool buttons are smaller
  bool isMiniLook = false;
  Vaca::SharedPtr<SkinProperty> skinPropery = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinPropery != NULL)
    isMiniLook = skinPropery->isMiniLook();

  // selected
  if (widget->isSelected()) {
    fg = get_button_selected_text_color();
    bg = get_button_selected_face_color();
    part_nw = isMiniLook ? PART_TOOLBUTTON_NORMAL_NW:
			   PART_BUTTON_SELECTED_NW;
  }
  // with mouse
  else if (widget->isEnabled() && widget->hasMouseOver()) {
    fg = get_button_hot_text_color();
    bg = get_button_hot_face_color();
    part_nw = isMiniLook ? PART_TOOLBUTTON_HOT_NW:
			   PART_BUTTON_HOT_NW;
  }
  // without mouse
  else {
    fg = get_button_normal_text_color();
    bg = get_button_normal_face_color();

    if (widget->hasFocus())
      part_nw = isMiniLook ? PART_TOOLBUTTON_HOT_NW:
			     PART_BUTTON_FOCUSED_NW;
    else
      part_nw = isMiniLook ? PART_TOOLBUTTON_NORMAL_NW:
			     PART_BUTTON_NORMAL_NW;
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

  // icon
  if (icon_bmp) {
    if (widget->isSelected())
      jrect_displace(&icon,
		     get_button_selected_offset(),
		     get_button_selected_offset());

    // enabled
    if (widget->isEnabled()) {
      // selected
      if (widget->isSelected()) {
	jdraw_inverted_sprite(ji_screen, icon_bmp, icon.x1, icon.y1);
      }
      // non-selected
      else {
	draw_sprite(ji_screen, icon_bmp, icon.x1, icon.y1);
      }
    }
    // disabled
    else {
      _ji_theme_draw_sprite_color(ji_screen, icon_bmp, icon.x1+jguiscale(), icon.y1+jguiscale(),
				  COLOR_BACKGROUND);
      _ji_theme_draw_sprite_color(ji_screen, icon_bmp, icon.x1, icon.y1,
				  COLOR_DISABLED);
    }
  }
}

void SkinneableTheme::draw_check(JWidget widget, JRect clip)
{
  struct jrect box, text, icon;
  int bg;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			    ji_generic_button_get_icon_align(widget),
			    widget->theme->check_icon_size,
			    widget->theme->check_icon_size);

  // background
  jdraw_rectfill(widget->rc, bg = BGCOLOR);

  // mouse
  if (widget->isEnabled()) {
    if (widget->hasMouseOver())
      jdraw_rectfill(widget->rc, bg = get_check_hot_face_color());
    else if (widget->hasFocus())
      jdraw_rectfill(widget->rc, bg = get_check_focus_face_color());
  }

  /* text */
  draw_textstring(NULL, -1, bg, false, widget, &text, 0);

  /* icon */
  set_alpha_blender();
  draw_trans_sprite(ji_screen,
		    widget->isSelected() ? m_part[PART_CHECK_SELECTED]:
					   m_part[PART_CHECK_NORMAL],
		    icon.x1, icon.y1);

  // draw focus
  if (widget->hasFocus()) {
    draw_bounds_nw(ji_screen,
		   widget->rc->x1,
		   widget->rc->y1,
		   widget->rc->x2-1,
		   widget->rc->y2-1, PART_CHECK_FOCUS_NW, -1);
  }
}

void SkinneableTheme::draw_grid(JWidget widget, JRect clip)
{
  jdraw_rectfill(clip, BGCOLOR);
}

void SkinneableTheme::draw_entry(JWidget widget, JRect clip)
{
  bool password = jentry_is_password(widget);
  int scroll, cursor, state, selbeg, selend;
  const char *text = widget->getText();
  int c, ch, x, y, w, fg, bg;
  int x1, y1, x2, y2;
  int cursor_x;

  jtheme_entry_info(widget, &scroll, &cursor, &state, &selbeg, &selend);

  /* main pos */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  bg = COLOR_BACKGROUND;

  draw_bounds_nw(ji_screen,
		 x1, y1, x2, y2,
		 widget->hasFocus() ? PART_SUNKEN_FOCUSED_NW:
				      PART_SUNKEN_NORMAL_NW, bg);

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

    cursor_x = x;
    ji_font_set_aa_mode(widget->getFont(), bg >= 0 ? bg: COLOR_BACKGROUND);
    widget->getFont()->vtable->render_char(widget->getFont(),
					   ch, fg, bg, ji_screen, x, y);
    x += w;

    /* cursor */
    if ((c == cursor) && (state) && (widget->hasFocus()))
      draw_entry_cursor(widget, cursor_x, y);
  }

  /* draw the cursor if it is next of the last character */
  if ((c == cursor) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled()))
    draw_entry_cursor(widget, x, y);
}

void SkinneableTheme::draw_label(JWidget widget, JRect clip)
{
  int bg = BGCOLOR;

  jdraw_rectfill(widget->rc, bg);

  draw_textstring(NULL, -1, bg, false, widget, widget->rc, 0);
}

void SkinneableTheme::draw_link_label(JWidget widget, JRect clip)
{
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

void SkinneableTheme::draw_listbox(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, COLOR_BACKGROUND);
}

void SkinneableTheme::draw_listitem(JWidget widget, JRect clip)
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

void SkinneableTheme::draw_menu(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, BGCOLOR);
}

void SkinneableTheme::draw_menuitem(JWidget widget, JRect clip)
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
    if (jmenuitem_is_highlight(widget)) {
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
    if (jmenuitem_get_submenu(widget)) {
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
    else if (jmenuitem_get_accel(widget)) {
      int old_align = widget->getAlign();
      char buf[256];

      pos = jwidget_get_rect(widget);
      pos->x2 -= widget->child_spacing/4;

      jaccel_to_string(jmenuitem_get_accel(widget), buf);

      widget->setAlign(JI_RIGHT | JI_MIDDLE);
      draw_textstring(buf, fg, bg, false, widget, pos, 0);
      widget->setAlign(old_align);

      jrect_free(pos);
    }
  }
}

void SkinneableTheme::draw_panel(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, get_panel_face_color());
}

void SkinneableTheme::draw_radio(JWidget widget, JRect clip)
{
  struct jrect box, text, icon;
  int bg = BGCOLOR;

  jwidget_get_texticon_info(widget, &box, &text, &icon,
			    ji_generic_button_get_icon_align(widget),
			    widget->theme->radio_icon_size,
			    widget->theme->radio_icon_size);

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

  /* icon */
  set_alpha_blender();
  draw_trans_sprite(ji_screen,
		    widget->isSelected() ? m_part[PART_RADIO_SELECTED]:
					   m_part[PART_RADIO_NORMAL],
		    icon.x1, icon.y1);

  // draw focus
  if (widget->hasFocus()) {
    draw_bounds_nw(ji_screen,
		   widget->rc->x1,
		   widget->rc->y1,
		   widget->rc->x2-1,
		   widget->rc->y2-1, PART_RADIO_FOCUS_NW, -1);
  }
}

void SkinneableTheme::draw_separator(JWidget widget, JRect clip)
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

void SkinneableTheme::draw_slider(JWidget widget, JRect clip)
{
  int x, x1, y1, x2, y2;
  int min, max, value;
  char buf[256];

  jtheme_slider_info(widget, &min, &max, &value);

  // Tool buttons are smaller
  bool isMiniLook = false;
  Vaca::SharedPtr<SkinProperty> skinPropery = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinPropery != NULL)
    isMiniLook = skinPropery->isMiniLook();

  x1 = widget->rc->x1 + widget->border_width.l;
  y1 = widget->rc->y1 + widget->border_width.t;
  x2 = widget->rc->x2 - widget->border_width.r - 1;
  y2 = widget->rc->y2 - widget->border_width.b - 1;

  if (min != max)
    x = x1 + (x2-x1) * (value-min) / (max-min);
  else
    x = x1;

  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2 - 1;
  y2 = widget->rc->y2 - 1;

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
    draw_bounds_nw(ji_screen, x1, y1, x2, y2, empty_part_nw, get_slider_empty_face_color());
  else if (value == max)
    draw_bounds_nw(ji_screen, x1, y1, x2, y2, full_part_nw, get_slider_full_face_color());
  else
    draw_bounds_nw2(ji_screen,
		    x1, y1, x2, y2, x,
		    full_part_nw, empty_part_nw,
		    get_slider_full_face_color(),
		    get_slider_empty_face_color());

  /* text */
  {
    std::string old_text = widget->getText();
    JRect r;
    int cx1, cy1, cx2, cy2;
    get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

    usprintf(buf, "%d", value);

    widget->setTextQuiet(buf);

    r = jrect_new(x1, y1, x2, y2);

    if (my_add_clip_rect(ji_screen, x1, y1, x, y2))
      draw_textstring(NULL,
		      get_slider_full_text_color(),
		      get_slider_full_face_color(), false, widget, r, 0);

    set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);

    if (my_add_clip_rect(ji_screen, x+1, y1, x2, y2))
      draw_textstring(NULL, 
		      get_slider_empty_text_color(),
		      get_slider_empty_face_color(), false, widget, r, 0);

    set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);

    widget->setTextQuiet(old_text.c_str());
    jrect_free(r);
  }
}

void SkinneableTheme::draw_combobox_entry(JWidget widget, JRect clip)
{
  bool password = jentry_is_password(widget);
  int scroll, cursor, state, selbeg, selend;
  const char *text = widget->getText();
  int c, ch, x, y, w, fg, bg;
  int x1, y1, x2, y2;
  int cursor_x;

  jtheme_entry_info(widget, &scroll, &cursor, &state, &selbeg, &selend);

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

    cursor_x = x;
    ji_font_set_aa_mode(widget->getFont(), bg >= 0 ? bg: COLOR_BACKGROUND);
    widget->getFont()->vtable->render_char(widget->getFont(),
					   ch, fg, bg, ji_screen, x, y);
    x += w;

    /* cursor */
    if ((c == cursor) && (state) && (widget->hasFocus()))
      draw_entry_cursor(widget, cursor_x, y);
  }

  /* draw the cursor if it is next of the last character */
  if ((c == cursor) && (state) &&
      (widget->hasFocus()) &&
      (widget->isEnabled()))
    draw_entry_cursor(widget, x, y);
}

void SkinneableTheme::draw_combobox_button(JWidget widget, JRect clip)
{
  BITMAP* icon_bmp = m_part[PART_COMBOBOX_ARROW];
  struct jrect icon;
  int x1, y1, x2, y2;
  int fg, bg, part_nw;

  /* with mouse */
  if (widget->isSelected() ||
      (widget->isEnabled() && widget->hasMouseOver())) {
    fg = get_button_hot_text_color();
    bg = get_button_hot_face_color();
    part_nw = PART_TOOLBUTTON_HOT_NW;
  }
  /* without mouse */
  else {
    fg = get_button_normal_text_color();
    bg = get_button_normal_face_color();
    part_nw = PART_TOOLBUTTON_LAST_NW;
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  // external background
  rectfill(ji_screen, x1, y1, x2, y2, BGCOLOR);

  // draw borders
  draw_bounds_nw(ji_screen, x1, y1, x2, y2, part_nw, bg);

  // icon
  icon.x1 = (x1+x2)/2 - icon_bmp->w/2;
  icon.y1 = (y1+y2)/2 - icon_bmp->h/2;
  icon.x2 = icon.x1 + icon_bmp->w;
  icon.y2 = icon.y1 + icon_bmp->h;

  if (widget->isSelected())
    jrect_displace(&icon,
		   get_button_selected_offset(),
		   get_button_selected_offset());

  set_alpha_blender();
  draw_trans_sprite(ji_screen, icon_bmp, icon.x1, icon.y1);
}

void SkinneableTheme::draw_textbox(JWidget widget, JRect clip)
{
  _ji_theme_textbox_draw(ji_screen, widget, NULL, NULL,
			 widget->theme->textbox_bg_color,
			 widget->theme->textbox_fg_color);
}

void SkinneableTheme::draw_view(JWidget widget, JRect clip)
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

void SkinneableTheme::draw_view_scrollbar(JWidget widget, JRect clip)
{
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int pos, len;

  jtheme_scrollbar_info(widget, &pos, &len);

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

void SkinneableTheme::draw_view_viewport(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, BGCOLOR);
}

void SkinneableTheme::draw_frame(Frame* window, JRect clip)
{
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
    jdraw_rectfill(pos, window->theme->desktop_color);
  }

  jrect_free(pos);
  jrect_free(cpos);
}

void SkinneableTheme::draw_frame_button(JWidget widget, JRect clip)
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

int SkinneableTheme::get_bg_color(JWidget widget)
{
  int c = jwidget_get_bg_color(widget);
  int decorative = jwidget_is_decorative(widget);

  return c >= 0 ? c: (decorative ? COLOR_SELECTED:
				   COLOR_FACE);
}

void SkinneableTheme::draw_textstring(const char *t, int fg_color, int bg_color,
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
	rectfill(ji_screen, x, y, x+w, y+h, bg_color);
      else
	rectfill(ji_screen, x, y, x+w-1, y+h-1, bg_color);
    }

    /* text */
    if (!widget->isEnabled()) {
      /* TODO avoid this */
      if (fill_bg)		/* only to draw the background */
	jdraw_text(ji_screen, widget->getFont(), t, x, y, 0, bg_color, fill_bg, jguiscale());

      /* draw white part */
      jdraw_text(ji_screen, widget->getFont(), t, x+1, y+1,
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

void SkinneableTheme::draw_entry_cursor(JWidget widget, int x, int y)
{
  int h = jwidget_get_text_height(widget);

  vline(ji_screen, x,   y-1, y+h, COLOR_FOREGROUND);
  vline(ji_screen, x+1, y-1, y+h, COLOR_FOREGROUND);
}

BITMAP* SkinneableTheme::get_toolicon(const char* tool_id) const
{
  std::map<std::string, BITMAP*>::const_iterator it = m_toolicon.find(tool_id);
  if (it != m_toolicon.end())
    return it->second;
  else
    return NULL;
}

#define draw_bounds_template(_bmp, _nw, _n, _ne, _e, _se, _s, _sw, _w, draw_func) \
  {									\
    int x, y;								\
    int cx1, cy1, cx2, cy2;						\
    get_clip_rect(_bmp, &cx1, &cy1, &cx2, &cy2);			\
									\
    /* top */								\
									\
    draw_func(_bmp, m_part[_nw], x1, y1);				\
									\
    if (my_add_clip_rect(_bmp,						\
			 x1+m_part[_nw]->w, y1,				\
			 x2-m_part[_ne]->w, y2)) {			\
      for (x = x1+m_part[_nw]->w;					\
	   x <= x2-m_part[_ne]->w;					\
	   x += m_part[_n]->w) {					\
	draw_func(_bmp, m_part[_n], x, y1);				\
      }									\
    }									\
    set_clip_rect(_bmp, cx1, cy1, cx2, cy2);				\
									\
    draw_func(_bmp, m_part[_ne], x2-m_part[_ne]->w+1, y1);		\
									\
    /* bottom */							\
									\
    draw_func(_bmp, m_part[_sw], x1, y2-m_part[_sw]->h+1);		\
									\
    if (my_add_clip_rect(_bmp,						\
			 x1+m_part[_sw]->w, y1,				\
			 x2-m_part[_se]->w, y2)) {			\
      for (x = x1+m_part[_sw]->w;					\
	   x <= x2-m_part[_se]->w;					\
	   x += m_part[_s]->w) {					\
	draw_func(_bmp, m_part[_s], x, y2-m_part[_s]->h+1);		\
      }									\
    }									\
    set_clip_rect(_bmp, cx1, cy1, cx2, cy2);				\
									\
    draw_func(_bmp, m_part[_se], x2-m_part[_se]->w+1, y2-m_part[_se]->h+1); \
									\
    if (my_add_clip_rect(_bmp,						\
			 x1, y1+m_part[_nw]->h,				\
			 x2, y2-m_part[_sw]->h)) {			\
      /* left */							\
      for (y = y1+m_part[_nw]->h;					\
	   y <= y2-m_part[_sw]->h;					\
	   y += m_part[_w]->h) {					\
	draw_func(_bmp, m_part[_w], x1, y);				\
      }									\
									\
      /* right */							\
      for (y = y1+m_part[_ne]->h;					\
	   y <= y2-m_part[_se]->h;					\
	   y += m_part[_e]->h) {					\
	draw_func(_bmp, m_part[_e], x2-m_part[_e]->w+1, y);		\
      }									\
    }									\
    set_clip_rect(_bmp, cx1, cy1, cx2, cy2);				\
  }

void SkinneableTheme::draw_bounds_array(BITMAP* bmp, int x1, int y1, int x2, int y2, int parts[8])
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
  draw_bounds_template(bmp,
		       nw, n, ne, e,
		       se, s, sw, w, draw_trans_sprite);
}

void SkinneableTheme::draw_bounds_nw(BITMAP* bmp, int x1, int y1, int x2, int y2, int nw, int bg)
{
  set_alpha_blender();
  draw_bounds_template(bmp,
		       nw+0, nw+1, nw+2, nw+3,
		       nw+4, nw+5, nw+6, nw+7, draw_trans_sprite);

  // Center 
  if (bg >= 0) {
    x1 += m_part[nw+7]->w;
    y1 += m_part[nw+1]->h;
    x2 -= m_part[nw+3]->w;
    y2 -= m_part[nw+5]->h;

    rectfill(bmp, x1, y1, x2, y2, bg);
  }
}

void SkinneableTheme::draw_bounds_nw2(BITMAP* bmp, int x1, int y1, int x2, int y2, int x_mid, int nw1, int nw2, int bg1, int bg2)
{
  int cx1, cy1, cx2, cy2;
  get_clip_rect(bmp, &cx1, &cy1, &cx2, &cy2);

  if (my_add_clip_rect(bmp, x1, y1, x_mid, y2))
    draw_bounds_nw(bmp, x1, y1, x2, y2, nw1, bg1);

  set_clip_rect(bmp, cx1, cy1, cx2, cy2);

  if (my_add_clip_rect(bmp, x_mid+1, y1, x2, y2))
    draw_bounds_nw(bmp, x1, y1, x2, y2, nw2, bg2);

  set_clip_rect(bmp, cx1, cy1, cx2, cy2);
}

void SkinneableTheme::draw_part_as_hline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part)
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

void SkinneableTheme::draw_part_as_vline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part)
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

void SkinneableTheme::draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel)
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

void SkinneableTheme::less_bevel(int *bevel)
{
  if (bevel[0] > 0) --bevel[0];
  if (bevel[1] > 0) --bevel[1];
  if (bevel[2] > 0) --bevel[2];
  if (bevel[3] > 0) --bevel[3];
}

/* controls the "X" button in a window to close it */
bool SkinneableTheme::theme_frame_button_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SETCURSOR:
      jmouse_set_cursor(JI_CURSOR_NORMAL);
      return true;

    case JM_DRAW:
      ((SkinneableTheme*)widget->theme)->draw_frame_button(widget, &msg->draw.rect);
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
	  jwidget_close_window(widget);
	  return true;
	}
      }
      break;
  }

  return false;
}

bool SkinneableTheme::theme_combobox_button_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      msg->reqsize.w = 15 * jguiscale();
      msg->reqsize.h = 16 * jguiscale();
      return true;

  }

  return false;
}

//////////////////////////////////////////////////////////////////////

const Vaca::Char* SkinProperty::SkinPropertyName = L"SkinProperty";

SkinProperty::SkinProperty()
  : Property(SkinPropertyName)
{
  m_isMiniLook = false;
}

SkinProperty::~SkinProperty()
{
}

bool SkinProperty::isMiniLook() const
{
  return m_isMiniLook;
}

void SkinProperty::setMiniLook(bool state)
{
  m_isMiniLook = state;
}
