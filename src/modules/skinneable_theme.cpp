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

#include "jinete/jinete.h"
#include "jinete/jintern.h"
#include "Vaca/SharedPtr.h"

#include "core/dirs.h"
#include "loadpng.h"
#include "ase_exception.h"
#include "modules/skinneable_theme.h"
#include "modules/gui.h"

#include "tinyxml.h"

#define CHARACTER_LENGTH(f, c)	((f)->vtable->char_length((f), (c)))

#define BGCOLOR			(get_bg_color(widget))

#define COLOR_FOREGROUND	makecol(0, 0, 0)
#define COLOR_DISABLED		makecol(150, 130, 117)
#define COLOR_FACE		makecol(211, 203, 190)
#define COLOR_HOTFACE		makecol(250, 240, 230)
#define COLOR_SELECTED		makecol(44, 76, 145)
#define COLOR_BACKGROUND	makecol(255, 255, 255)

static struct
{
  int x, y, w, h;
} sheet_info[PARTS] = {

  { 80,  0, 16, 16 }, //  PART_CURSOR_NORMAL,
  { 80, 16, 16, 16 }, //  PART_CURSOR_NORMAL_ADD,
  { 80, 32, 16, 16 }, //  PART_CURSOR_FORBIDDEN,
  { 80, 48, 16, 16 }, //  PART_CURSOR_HAND,
  { 80, 64, 16, 16 }, //  PART_CURSOR_SCROLL,
  { 80, 80, 16, 16 }, //  PART_CURSOR_MOVE,
  { 80, 96, 16, 16 }, //  PART_CURSOR_SIZE_TL,
  { 80,112, 16, 16 }, //  PART_CURSOR_SIZE_T,
  { 80,128, 16, 16 }, //  PART_CURSOR_SIZE_TR,
  { 80,144, 16, 16 }, //  PART_CURSOR_SIZE_L,
  { 80,160, 16, 16 }, //  PART_CURSOR_SIZE_R,
  { 80,176, 16, 16 }, //  PART_CURSOR_SIZE_BL,
  { 80,192, 16, 16 }, //  PART_CURSOR_SIZE_B,
  { 80,208, 16, 16 }, //  PART_CURSOR_SIZE_BR,
  { 80,224, 16, 16 }, //  PART_CURSOR_EYEDROPPER,

  { 32, 32,  8,  8 }, //  PART_RADIO_NORMAL
  { 32, 48,  8,  8 }, //  PART_RADIO_SELECTED
  { 32, 32,  8,  8 }, //  PART_RADIO_DISABLED

  { 48, 32,  8,  8 }, //  PART_CHECK_NORMAL
  { 48, 48,  8,  8 }, //  PART_CHECK_SELECTED
  { 48, 32,  8,  8 }, //  PART_CHECK_DISABLED

  { 32, 64,  2,  2 }, //  PART_CHECK_FOCUS_NW
  { 34, 64,  6,  2 }, //  PART_CHECK_FOCUS_N
  { 40, 64,  2,  2 }, //  PART_CHECK_FOCUS_NE
  { 40, 66,  2,  6 }, //  PART_CHECK_FOCUS_E
  { 40, 72,  2,  2 }, //  PART_CHECK_FOCUS_SE
  { 34, 72,  6,  2 }, //  PART_CHECK_FOCUS_S
  { 32, 72,  2,  2 }, //  PART_CHECK_FOCUS_SW
  { 32, 66,  2,  6 }, //  PART_CHECK_FOCUS_W

  { 32, 64,  2,  2 }, //  PART_RADIO_FOCUS_NW
  { 34, 64,  6,  2 }, //  PART_RADIO_FOCUS_N
  { 40, 64,  2,  2 }, //  PART_RADIO_FOCUS_NE
  { 40, 66,  2,  6 }, //  PART_RADIO_FOCUS_E
  { 40, 72,  2,  2 }, //  PART_RADIO_FOCUS_SE
  { 34, 72,  6,  2 }, //  PART_RADIO_FOCUS_S
  { 32, 72,  2,  2 }, //  PART_RADIO_FOCUS_SW
  { 32, 66,  2,  6 }, //  PART_RADIO_FOCUS_W

  { 48,  0,  4,  4 }, //  PART_BUTTON_NORMAL_NW
  { 52,  0,  6,  4 }, //  PART_BUTTON_NORMAL_N
  { 58,  0,  4,  4 }, //  PART_BUTTON_NORMAL_NE
  { 58,  4,  4,  6 }, //  PART_BUTTON_NORMAL_E
  { 58, 10,  4,  6 }, //  PART_BUTTON_NORMAL_SE
  { 52, 10,  6,  6 }, //  PART_BUTTON_NORMAL_S
  { 48, 10,  4,  6 }, //  PART_BUTTON_NORMAL_SW
  { 48,  4,  4,  6 }, //  PART_BUTTON_NORMAL_W

  { 64,  0,  4,  4 }, //  PART_BUTTON_HOT_NW
  { 68,  0,  6,  4 }, //  PART_BUTTON_HOT_N
  { 74,  0,  4,  4 }, //  PART_BUTTON_HOT_NE
  { 74,  4,  4,  6 }, //  PART_BUTTON_HOT_E
  { 74, 10,  4,  6 }, //  PART_BUTTON_HOT_SE
  { 68, 10,  6,  6 }, //  PART_BUTTON_HOT_S
  { 64, 10,  4,  6 }, //  PART_BUTTON_HOT_SW
  { 64,  4,  4,  6 }, //  PART_BUTTON_HOT_W

  { 48, 16,  4,  4 }, //  PART_BUTTON_FOCUSED_NW
  { 52, 16,  6,  4 }, //  PART_BUTTON_FOCUSED_N
  { 58, 16,  4,  4 }, //  PART_BUTTON_FOCUSED_NE
  { 58, 20,  4,  6 }, //  PART_BUTTON_FOCUSED_E
  { 58, 26,  4,  6 }, //  PART_BUTTON_FOCUSED_SE
  { 52, 26,  6,  6 }, //  PART_BUTTON_FOCUSED_S
  { 48, 26,  4,  6 }, //  PART_BUTTON_FOCUSED_SW
  { 48, 20,  4,  6 }, //  PART_BUTTON_FOCUSED_W

  { 64, 16,  4,  4 }, //  PART_BUTTON_SELECTED_NW
  { 68, 16,  6,  4 }, //  PART_BUTTON_SELECTED_N
  { 74, 16,  4,  4 }, //  PART_BUTTON_SELECTED_NE
  { 74, 20,  4,  6 }, //  PART_BUTTON_SELECTED_E
  { 74, 26,  4,  6 }, //  PART_BUTTON_SELECTED_SE
  { 68, 26,  6,  6 }, //  PART_BUTTON_SELECTED_S
  { 64, 26,  4,  6 }, //  PART_BUTTON_SELECTED_SW
  { 64, 20,  4,  6 }, //  PART_BUTTON_SELECTED_W

  {  0, 32,  4,  4 }, //  PART_SUNKEN_NORMAL_NW
  {  4, 32,  4,  4 }, //  PART_SUNKEN_NORMAL_N
  {  8, 32,  4,  4 }, //  PART_SUNKEN_NORMAL_NE
  {  8, 36,  4,  4 }, //  PART_SUNKEN_NORMAL_E
  {  8, 40,  4,  4 }, //  PART_SUNKEN_NORMAL_SE
  {  4, 40,  4,  4 }, //  PART_SUNKEN_NORMAL_S
  {  0, 40,  4,  4 }, //  PART_SUNKEN_NORMAL_SW
  {  0, 36,  4,  4 }, //  PART_SUNKEN_NORMAL_W

  {  0, 48,  4,  4 }, //  PART_SUNKEN_FOCUSED_NW
  {  4, 48,  4,  4 }, //  PART_SUNKEN_FOCUSED_N
  {  8, 48,  4,  4 }, //  PART_SUNKEN_FOCUSED_NE
  {  8, 52,  4,  4 }, //  PART_SUNKEN_FOCUSED_E
  {  8, 56,  4,  4 }, //  PART_SUNKEN_FOCUSED_SE
  {  4, 56,  4,  4 }, //  PART_SUNKEN_FOCUSED_S
  {  0, 56,  4,  4 }, //  PART_SUNKEN_FOCUSED_SW
  {  0, 52,  4,  4 }, //  PART_SUNKEN_FOCUSED_W

  {  0, 64,  5,  5 }, //  PART_SUNKEN2_NORMAL_NW
  {  5, 64,  6,  5 }, //  PART_SUNKEN2_NORMAL_N
  { 11, 64,  5,  5 }, //  PART_SUNKEN2_NORMAL_NE
  { 11, 69,  5,  6 }, //  PART_SUNKEN2_NORMAL_E
  { 11, 75,  5,  5 }, //  PART_SUNKEN2_NORMAL_SE
  {  5, 75,  6,  5 }, //  PART_SUNKEN2_NORMAL_S
  {  0, 75,  5,  5 }, //  PART_SUNKEN2_NORMAL_SW
  {  0, 69,  5,  6 }, //  PART_SUNKEN2_NORMAL_W

  {  0, 80,  5,  5 }, //  PART_SUNKEN2_FOCUSED_NW
  {  5, 80,  6,  5 }, //  PART_SUNKEN2_FOCUSED_N
  { 11, 80,  5,  5 }, //  PART_SUNKEN2_FOCUSED_NE
  { 11, 85,  5,  6 }, //  PART_SUNKEN2_FOCUSED_E
  { 11, 91,  5,  5 }, //  PART_SUNKEN2_FOCUSED_SE
  {  5, 91,  6,  5 }, //  PART_SUNKEN2_FOCUSED_S
  {  0, 91,  5,  5 }, //  PART_SUNKEN2_FOCUSED_SW
  {  0, 85,  5,  6 }, //  PART_SUNKEN2_FOCUSED_W

  {  0,  0,  3, 15 }, //  PART_WINDOW_NW
  {  3,  0,  7, 15 }, //  PART_WINDOW_N
  { 10,  0,  3, 15 }, //  PART_WINDOW_NE
  { 10, 15,  3,  4 }, //  PART_WINDOW_E
  { 10, 19,  3,  5 }, //  PART_WINDOW_SE
  {  3, 19,  7,  5 }, //  PART_WINDOW_S
  {  0, 19,  3,  5 }, //  PART_WINDOW_SW
  {  0, 15,  3,  4 }, //  PART_WINDOW_W

  {  0, 96,  3,  3 }, //  PART_MENU_NW
  {  3, 96, 10,  3 }, //  PART_MENU_N
  { 13, 96,  3,  3 }, //  PART_MENU_NE
  { 13, 99,  3,  9 }, //  PART_MENU_E
  { 13,108,  3,  4 }, //  PART_MENU_SE
  {  3,108, 10,  4 }, //  PART_MENU_S
  {  0,108,  3,  4 }, //  PART_MENU_SW
  {  0, 99,  3,  9 }, //  PART_MENU_W

  { 16,  0,  9, 11 }, //  PART_WINDOW_CLOSE_BUTTON_NORMAL
  { 16, 16,  9, 11 }, //  PART_WINDOW_CLOSE_BUTTON_HOT
  { 16, 32,  9, 11 }, //  PART_WINDOW_CLOSE_BUTTON_SELECTED

  {  0,144,  5,  5 }, //  PART_SLIDER_FULL_NW
  {  5,144,  6,  5 }, //  PART_SLIDER_FULL_N
  { 11,144,  5,  5 }, //  PART_SLIDER_FULL_NE
  { 11,149,  5,  5 }, //  PART_SLIDER_FULL_E
  { 11,154,  5,  6 }, //  PART_SLIDER_FULL_SE
  {  5,154,  6,  6 }, //  PART_SLIDER_FULL_S
  {  0,154,  5,  6 }, //  PART_SLIDER_FULL_SW
  {  0,149,  5,  5 }, //  PART_SLIDER_FULL_W

  { 16,144,  5,  5 }, //  PART_SLIDER_EMPTY_NW
  { 21,144,  6,  5 }, //  PART_SLIDER_EMPTY_N
  { 27,144,  5,  5 }, //  PART_SLIDER_EMPTY_NE
  { 27,149,  5,  5 }, //  PART_SLIDER_EMPTY_E
  { 27,154,  5,  6 }, //  PART_SLIDER_EMPTY_SE
  { 21,154,  6,  6 }, //  PART_SLIDER_EMPTY_S
  { 16,154,  5,  6 }, //  PART_SLIDER_EMPTY_SW
  { 16,149,  5,  5 }, //  PART_SLIDER_EMPTY_W

  {  0,160,  5,  5 }, //  PART_SLIDER_FULL_FOCUSED_NW
  {  5,160,  6,  5 }, //  PART_SLIDER_FULL_FOCUSED_N
  { 11,160,  5,  5 }, //  PART_SLIDER_FULL_FOCUSED_NE
  { 11,165,  5,  5 }, //  PART_SLIDER_FULL_FOCUSED_E
  { 11,170,  5,  6 }, //  PART_SLIDER_FULL_FOCUSED_SE
  {  5,170,  6,  6 }, //  PART_SLIDER_FULL_FOCUSED_S
  {  0,170,  5,  6 }, //  PART_SLIDER_FULL_FOCUSED_SW
  {  0,165,  5,  5 }, //  PART_SLIDER_FULL_FOCUSED_W

  { 16,160,  5,  5 }, //  PART_SLIDER_EMPTY_FOCUSED_NW
  { 21,160,  6,  5 }, //  PART_SLIDER_EMPTY_FOCUSED_N
  { 27,160,  5,  5 }, //  PART_SLIDER_EMPTY_FOCUSED_NE
  { 27,165,  5,  5 }, //  PART_SLIDER_EMPTY_FOCUSED_E
  { 27,170,  5,  6 }, //  PART_SLIDER_EMPTY_FOCUSED_SE
  { 21,170,  6,  6 }, //  PART_SLIDER_EMPTY_FOCUSED_S
  { 16,170,  5,  6 }, //  PART_SLIDER_EMPTY_FOCUSED_SW
  { 16,165,  5,  5 }, //  PART_SLIDER_EMPTY_FOCUSED_W

  { 32,144,  5,  5 }, //  PART_MINI_SLIDER_FULL_NW
  { 37,144,  6,  5 }, //  PART_MINI_SLIDER_FULL_N
  { 43,144,  5,  5 }, //  PART_MINI_SLIDER_FULL_NE
  { 43,149,  5,  5 }, //  PART_MINI_SLIDER_FULL_E
  { 43,154,  5,  6 }, //  PART_MINI_SLIDER_FULL_SE
  { 37,154,  6,  6 }, //  PART_MINI_SLIDER_FULL_S
  { 32,154,  5,  6 }, //  PART_MINI_SLIDER_FULL_SW
  { 32,149,  5,  5 }, //  PART_MINI_SLIDER_FULL_W

  { 48,144,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_NW
  { 53,144,  6,  5 }, //  PART_MINI_SLIDER_EMPTY_N
  { 59,144,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_NE
  { 59,149,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_E
  { 59,154,  5,  6 }, //  PART_MINI_SLIDER_EMPTY_SE
  { 53,154,  6,  6 }, //  PART_MINI_SLIDER_EMPTY_S
  { 48,154,  5,  6 }, //  PART_MINI_SLIDER_EMPTY_SW
  { 48,149,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_W

  { 32,160,  5,  5 }, //  PART_MINI_SLIDER_FULL_FOCUSED_NW
  { 37,160,  6,  5 }, //  PART_MINI_SLIDER_FULL_FOCUSED_N
  { 43,160,  5,  5 }, //  PART_MINI_SLIDER_FULL_FOCUSED_NE
  { 43,165,  5,  5 }, //  PART_MINI_SLIDER_FULL_FOCUSED_E
  { 43,170,  5,  6 }, //  PART_MINI_SLIDER_FULL_FOCUSED_SE
  { 37,170,  6,  6 }, //  PART_MINI_SLIDER_FULL_FOCUSED_S
  { 32,170,  5,  6 }, //  PART_MINI_SLIDER_FULL_FOCUSED_SW
  { 32,165,  5,  5 }, //  PART_MINI_SLIDER_FULL_FOCUSED_W

  { 48,160,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_NW
  { 53,160,  6,  5 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_N
  { 59,160,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_NE
  { 59,165,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_E
  { 59,170,  5,  6 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_SE
  { 53,170,  6,  6 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_S
  { 48,170,  5,  6 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_SW
  { 48,165,  5,  5 }, //  PART_MINI_SLIDER_EMPTY_FOCUSED_W

  { 32, 80,  9,  5 }, //  PART_SEPARATOR

  { 96, 32, 16, 16 }, //  PART_COMBOBOX_ARROW

  { 96,  0,  3,  3 }, //  PART_TOOLBUTTON_NORMAL_NW
  { 99,  0, 10,  3 }, //  PART_TOOLBUTTON_NORMAL_N
  {109,  0,  3,  3 }, //  PART_TOOLBUTTON_NORMAL_NE
  {109,  3,  3,  9 }, //  PART_TOOLBUTTON_NORMAL_E
  {109, 12,  3,  4 }, //  PART_TOOLBUTTON_NORMAL_SE
  { 99, 12, 10,  4 }, //  PART_TOOLBUTTON_NORMAL_S
  { 96, 12,  3,  4 }, //  PART_TOOLBUTTON_NORMAL_SW
  { 96,  3,  3,  9 }, //  PART_TOOLBUTTON_NORMAL_W

  {112,  0,  3,  3 }, //  PART_TOOLBUTTON_HOT_NW
  {115,  0, 10,  3 }, //  PART_TOOLBUTTON_HOT_N
  {125,  0,  3,  3 }, //  PART_TOOLBUTTON_HOT_NE
  {125,  3,  3,  9 }, //  PART_TOOLBUTTON_HOT_E
  {125, 12,  3,  4 }, //  PART_TOOLBUTTON_HOT_SE
  {115, 12, 10,  4 }, //  PART_TOOLBUTTON_HOT_S
  {112, 12,  3,  4 }, //  PART_TOOLBUTTON_HOT_SW
  {112,  3,  3,  9 }, //  PART_TOOLBUTTON_HOT_W

  { 96, 16,  3,  3 }, //  PART_TOOLBUTTON_LAST_NORMAL_NW
  { 99, 16, 10,  3 }, //  PART_TOOLBUTTON_LAST_NORMAL_N
  {109, 16,  3,  3 }, //  PART_TOOLBUTTON_LAST_NORMAL_NE
  {109, 19,  3,  9 }, //  PART_TOOLBUTTON_LAST_NORMAL_E
  {109, 28,  3,  4 }, //  PART_TOOLBUTTON_LAST_NORMAL_SE
  { 99, 28, 10,  4 }, //  PART_TOOLBUTTON_LAST_NORMAL_S
  { 96, 28,  3,  4 }, //  PART_TOOLBUTTON_LAST_NORMAL_SW
  { 96, 19,  3,  9 }, //  PART_TOOLBUTTON_LAST_NORMAL_W

  // Tabs

  {  2,112,  4,  4 }, //  PART_TAB_NORMAL_NW
  {  6,112,  5,  4 }, //  PART_TAB_NORMAL_N
  { 11,112,  5,  4 }, //  PART_TAB_NORMAL_NE
  { 11,116,  5,  6 }, //  PART_TAB_NORMAL_E
  { 11,122,  5,  2 }, //  PART_TAB_NORMAL_SE
  {  6,122,  4,  2 }, //  PART_TAB_NORMAL_S
  {  2,122,  4,  2 }, //  PART_TAB_NORMAL_SW
  {  2,116,  4,  6 }, //  PART_TAB_NORMAL_W

  { 16,112,  4,  4 }, //  PART_TAB_SELECTED_NW
  { 20,112,  7,  4 }, //  PART_TAB_SELECTED_N
  { 27,112,  5,  4 }, //  PART_TAB_SELECTED_NE
  { 27,116,  5,  6 }, //  PART_TAB_SELECTED_E
  { 27,122,  5,  2 }, //  PART_TAB_SELECTED_SE
  { 20,122,  7,  2 }, //  PART_TAB_SELECTED_S
  { 16,122,  4,  2 }, //  PART_TAB_SELECTED_SW
  { 16,116,  4,  6 }, //  PART_TAB_SELECTED_W

  { 16,124,  4,  4 }, //  PART_TAB_BOTTOM_SELECTED_NW
  { 20,124,  7,  4 }, //  PART_TAB_BOTTOM_SELECTED_N
  { 27,124,  5,  4 }, //  PART_TAB_BOTTOM_SELECTED_NE
  { 27,128,  5,  1 }, //  PART_TAB_BOTTOM_SELECTED_E
  { 27,129,  5,  4 }, //  PART_TAB_BOTTOM_SELECTED_SE
  { 20,129,  7,  4 }, //  PART_TAB_BOTTOM_SELECTED_S
  { 16,129,  4,  4 }, //  PART_TAB_BOTTOM_SELECTED_SW
  { 16,128,  4,  1 }, //  PART_TAB_BOTTOM_SELECTED_W

  {  2,124, 12,  9 }, //  PART_TAB_BOTTOM_NORMAL

  {  0,112,  2, 12 }, //  PART_TAB_FILLER

  { 32,112,  3,  3 }, //  PART_EDITOR_NORMAL_NW
  { 35,112, 10,  3 }, //  PART_EDITOR_NORMAL_N
  { 45,112,  3,  3 }, //  PART_EDITOR_NORMAL_NE
  { 45,115,  3, 10 }, //  PART_EDITOR_NORMAL_E
  { 45,125,  3,  3 }, //  PART_EDITOR_NORMAL_SE
  { 35,125, 10,  3 }, //  PART_EDITOR_NORMAL_S
  { 32,125,  3,  3 }, //  PART_EDITOR_NORMAL_SW
  { 32,115,  3, 10 }, //  PART_EDITOR_NORMAL_W

  { 48,112,  3,  3 }, //  PART_EDITOR_SELECTED_NW
  { 51,112, 10,  3 }, //  PART_EDITOR_SELECTED_N
  { 61,112,  3,  3 }, //  PART_EDITOR_SELECTED_NE
  { 61,115,  3, 10 }, //  PART_EDITOR_SELECTED_E
  { 61,125,  3,  3 }, //  PART_EDITOR_SELECTED_SE
  { 51,125, 10,  3 }, //  PART_EDITOR_SELECTED_S
  { 48,125,  3,  3 }, //  PART_EDITOR_SELECTED_SW
  { 48,115,  3, 10 }, //  PART_EDITOR_SELECTED_W

};

static struct
{
  int x, y;
} focus_info[] = {
  {  0,  0 }, //  PART_CURSOR_NORMAL,
  {  0,  0 }, //  PART_CURSOR_NORMAL_ADD,
  {  0,  0 }, //  PART_CURSOR_FORBIDDEN,
  {  5,  3 }, //  PART_CURSOR_HAND,
  {  8,  8 }, //  PART_CURSOR_SCROLL,
  {  0,  0 }, //  PART_CURSOR_MOVE,
  {  8,  8 }, //  PART_CURSOR_SIZE_TL,
  {  8,  8 }, //  PART_CURSOR_SIZE_T,
  {  8,  8 }, //  PART_CURSOR_SIZE_TR,
  {  8,  8 }, //  PART_CURSOR_SIZE_L,
  {  8,  8 }, //  PART_CURSOR_SIZE_R,
  {  8,  8 }, //  PART_CURSOR_SIZE_BL,
  {  8,  8 }, //  PART_CURSOR_SIZE_B,
  {  8,  8 }, //  PART_CURSOR_SIZE_BR,
  {  0, 15 }, //  PART_CURSOR_EYEDROPPER,
};

SkinneableTheme::SkinneableTheme()
{
  this->name = "Skinneable Theme";

  m_sheet_bmp = NULL;
  for (int c=0; c<PARTS; ++c)
    m_part[c] = NULL;

  // Load the skin sheet
  {
    DIRS* dirs = filename_in_datadir("skins/default_skin_sheet.png");
    for (DIRS* dir=dirs; dir; dir=dir->next) {
      if ((dir->path) && exists(dir->path)) {
	int old_color_conv = _color_conv;
	set_color_conversion(COLORCONV_NONE);

	PALETTE pal;
	m_sheet_bmp = load_png(dir->path, pal);

	set_color_conversion(old_color_conv);
	break;
      }
    }
    dirs_free(dirs);
  }
  if (!m_sheet_bmp)
    throw ase_exception("Error loading data/skins/default_skin_sheet.png file");
}

SkinneableTheme::~SkinneableTheme()
{
  for (int c=0; c<PARTS; ++c)
    destroy_bitmap(m_part[c]);

  for (std::map<std::string, BITMAP*>::iterator
	 it = m_toolicon.begin(); it != m_toolicon.end(); ++it) {
    destroy_bitmap(it->second);
  }

  destroy_bitmap(m_sheet_bmp);
}

void SkinneableTheme::regen()
{
  check_icon_size = 8 * jguiscale();
  radio_icon_size = 8 * jguiscale();
  scrollbar_size = 12 * jguiscale();

  desktop_color = COLOR_DISABLED;
  textbox_fg_color = COLOR_FOREGROUND;
  textbox_bg_color = COLOR_BACKGROUND;

  for (int c=0; c<PARTS; ++c) {
    if (m_part[c]) destroy_bitmap(m_part[c]);

    m_part[c] = create_bitmap(sheet_info[c].w, sheet_info[c].h);
    clear_to_color(m_part[c], bitmap_mask_color(m_part[c]));

    set_alpha_blender();
    draw_trans_sprite(m_part[c], m_sheet_bmp, -sheet_info[c].x, -sheet_info[c].y);
    set_trans_blender(0, 0, 0, 0);

    m_part[c] = ji_apply_guiscale(m_part[c]);
  }

  // Load tool icons
  {
    DIRS* dirs = filename_in_datadir("skins/default_skin.xml");
    for (DIRS* dir=dirs; dir; dir=dir->next) {
      if ((dir->path) && exists(dir->path)) {

	TiXmlDocument doc;
	if (!doc.LoadFile(dir->path))
	  throw ase_exception(&doc);

	TiXmlHandle handle(&doc);
	
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
	  BITMAP* toolicon = create_bitmap(w, h);
	  clear_to_color(toolicon, bitmap_mask_color(m_sheet_bmp));

	  set_alpha_blender();
	  draw_trans_sprite(toolicon, m_sheet_bmp, -x, -y);
	  set_trans_blender(0, 0, 0, 0);

	  // Add the tool-icon in the map
	  if (m_toolicon[tool_id]) destroy_bitmap(m_toolicon[tool_id]);
	  m_toolicon[tool_id] = ji_apply_guiscale(toolicon);

	  xmlIcon = xmlIcon->NextSiblingElement();
	}
	break;
      }
    }
    dirs_free(dirs);
  }
}

BITMAP* SkinneableTheme::set_cursor(int type, int* focus_x, int* focus_y)
{
  if (type == JI_CURSOR_NULL) {
    *focus_x = 0;
    *focus_y = 0;
    return NULL;
  }
  else {
    *focus_x = focus_info[type-JI_CURSOR_NORMAL].x;
    *focus_y = focus_info[type-JI_CURSOR_NORMAL].y;
    return m_part[PART_CURSOR_NORMAL+type-JI_CURSOR_NORMAL];
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
	JWidget button = jcombobox_get_button_widget(widget);

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
  if (jwidget_is_selected(widget)) {
    fg = get_button_selected_text_color();
    bg = get_button_selected_face_color();
    part_nw = isMiniLook ? PART_TOOLBUTTON_NORMAL_NW:
			   PART_BUTTON_SELECTED_NW;
  }
  // with mouse
  else if (jwidget_is_enabled(widget) && jwidget_has_mouse(widget)) {
    fg = get_button_hot_text_color();
    bg = get_button_hot_face_color();
    part_nw = isMiniLook ? PART_TOOLBUTTON_HOT_NW:
			   PART_BUTTON_HOT_NW;
  }
  // without mouse
  else {
    fg = get_button_normal_text_color();
    bg = get_button_normal_face_color();

    if (jwidget_has_focus(widget))
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
  draw_bounds(x1, y1, x2, y2, part_nw, bg);

  // text
  crect = jwidget_get_child_rect(widget);
  draw_textstring(NULL, fg, bg, false, widget, crect, get_button_selected_offset());
  jrect_free(crect);

  // icon
  if (icon_bmp) {
    if (jwidget_is_selected(widget))
      jrect_displace(&icon,
		     get_button_selected_offset(),
		     get_button_selected_offset());

    // enabled
    if (jwidget_is_enabled(widget)) {
      // selected
      if (jwidget_is_selected(widget)) {
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
  if (jwidget_is_enabled(widget)) {
    if (jwidget_has_mouse(widget))
      jdraw_rectfill(widget->rc, bg = get_check_hot_face_color());
    else if (jwidget_has_focus(widget))
      jdraw_rectfill(widget->rc, bg = get_check_focus_face_color());
  }

  /* text */
  draw_textstring(NULL, -1, bg, false, widget, &text, 0);

  /* icon */
  draw_sprite(ji_screen,
	      jwidget_is_selected(widget) ? m_part[PART_CHECK_SELECTED]:
					    m_part[PART_CHECK_NORMAL],
	      icon.x1, icon.y1);

  // draw focus
  if (jwidget_has_focus(widget)) {
    draw_bounds(widget->rc->x1,
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

  draw_bounds(x1, y1, x2, y2,
	      jwidget_has_focus(widget) ? PART_SUNKEN_FOCUSED_NW:
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
      if (jwidget_has_focus(widget))
	bg = COLOR_SELECTED;
      else
	bg = COLOR_DISABLED;
      fg = COLOR_BACKGROUND;
    }

    /* disabled */
    if (jwidget_is_disabled (widget)) {
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
    if ((c == cursor) && (state) && (jwidget_has_focus (widget)))
      draw_entry_cursor(widget, cursor_x, y);
  }

  /* draw the cursor if it is next of the last character */
  if ((c == cursor) && (state) &&
      (jwidget_has_focus(widget)) &&
      (jwidget_is_enabled(widget)))
    draw_entry_cursor(widget, x, y);
}

void SkinneableTheme::draw_label(JWidget widget, JRect clip)
{
  int bg = BGCOLOR;

  jdraw_rectfill(widget->rc, bg);

  draw_textstring(NULL, -1, bg, false, widget, widget->rc, 0);
}

void SkinneableTheme::draw_listbox(JWidget widget, JRect clip)
{
  jdraw_rectfill(widget->rc, COLOR_BACKGROUND);
}

void SkinneableTheme::draw_listitem(JWidget widget, JRect clip)
{
  int fg, bg;
  int x, y;

  if (jwidget_is_disabled(widget)) {
    bg = COLOR_FACE;
    fg = COLOR_DISABLED;
  }
  else if (jwidget_is_selected(widget)) {
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
    jdraw_text(widget->getFont(), widget->getText(), x, y, fg, bg, true, jguiscale());

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

  /* TODO assert? */
  if (!widget->parent->parent)
    return;

  bar = (widget->parent->parent->type == JI_MENUBAR);

  /* colors */
  if (jwidget_is_disabled(widget)) {
    fg = -1;
    bg = get_menuitem_normal_face_color();
  }
  else {
    if (jmenuitem_is_highlight(widget)) {
      fg = get_menuitem_highlight_text_color();
      bg = get_menuitem_highlight_face_color();
    }
    else if (jwidget_has_mouse(widget)) {
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
  if (jwidget_is_selected(widget)) {
    BITMAP* icon = m_part[jwidget_is_enabled(widget) ? PART_CHECK_SELECTED:
						       PART_CHECK_DISABLED];

    int x = widget->rc->x1+4-icon->w/2;
    int y = (widget->rc->y1+widget->rc->y2)/2-icon->h/2;

    draw_sprite(ji_screen, icon, x, y);
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
      if (jwidget_is_enabled(widget)) {
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
  if (jwidget_is_enabled(widget)) {
    if (jwidget_has_mouse(widget))
      jdraw_rectfill(widget->rc, bg = get_radio_hot_face_color());
    else if (jwidget_has_focus(widget))
      jdraw_rectfill(widget->rc, bg = get_radio_focus_face_color());
  }

  /* text */
  draw_textstring(NULL, -1, bg, false, widget, &text, 0);

  /* icon */
  draw_sprite(ji_screen,
	      jwidget_is_selected(widget) ? m_part[PART_RADIO_SELECTED]:
					    m_part[PART_RADIO_NORMAL],
	      icon.x1, icon.y1);

  // draw focus
  if (jwidget_has_focus(widget)) {
    draw_bounds(widget->rc->x1,
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
    draw_hline(widget->rc->x1,
	       widget->rc->y1,
	       widget->rc->x2-1,
	       widget->rc->y2-1, PART_SEPARATOR);
  }

  // text
  if (widget->hasText()) {
    int h = jwidget_get_text_height(widget);
    struct jrect r = { x1+h/2, y1-h/2, x2+1-h, y2+1+h };
    draw_textstring(NULL, -1, BGCOLOR, false, widget, &r, 0);
  }
}

#if 1
/* TODO when Allegro 4.1 will be officially released, replace this
   with the get_clip_rect, add_clip_rect, set_clip_rect functions */
static int my_add_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
  int u1 = MAX(x1, bitmap->cl);
  int v1 = MAX(y1, bitmap->ct);
  int u2 = MIN(x2, bitmap->cr-1);
  int v2 = MIN(y2, bitmap->cb-1);

  if (u1 > u2 || v1 > v2)
    return false;
  else
    set_clip(bitmap, u1, v1, u2, v2);

  return true;
}
#endif

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
    full_part_nw = jwidget_has_mouse(widget) ? PART_MINI_SLIDER_FULL_FOCUSED_NW:
					       PART_MINI_SLIDER_FULL_NW;
    empty_part_nw = jwidget_has_mouse(widget) ? PART_MINI_SLIDER_EMPTY_FOCUSED_NW:
						PART_MINI_SLIDER_EMPTY_NW;
  }
  else {
    full_part_nw = jwidget_has_focus(widget) ? PART_SLIDER_FULL_FOCUSED_NW:
					       PART_SLIDER_FULL_NW;
    empty_part_nw = jwidget_has_focus(widget) ? PART_SLIDER_EMPTY_FOCUSED_NW:
						PART_SLIDER_EMPTY_NW;
  }

  if (value == min)
    draw_bounds(x1, y1, x2, y2, empty_part_nw, get_slider_empty_face_color());
  else if (value == max)
    draw_bounds(x1, y1, x2, y2, full_part_nw, get_slider_full_face_color());
  else
    draw_bounds2(x1, y1, x2, y2, x,
		 full_part_nw, empty_part_nw,
		 get_slider_full_face_color(),
		 get_slider_empty_face_color());

  /* text */
  {
    std::string old_text = widget->getText();
    int cx1, cy1, cx2, cy2;
    JRect r;

    usprintf(buf, "%d", value);

    widget->setTextQuiet(buf);

    r = jrect_new(x1, y1, x2+1, y2+1);

    /* TODO when Allegro 4.1 will be officially released, replace this
       with the get_clip_rect, add_clip_rect, set_clip_rect
       functions */

    cx1 = ji_screen->cl;
    cy1 = ji_screen->ct;
    cx2 = ji_screen->cr-1;
    cy2 = ji_screen->cb-1;

    if (my_add_clip_rect(ji_screen, x1, y1, x, y2))
      draw_textstring(NULL,
		      get_slider_full_text_color(),
		      get_slider_full_face_color(), false, widget, r, 0);

    set_clip(ji_screen, cx1, cy1, cx2, cy2);

    if (my_add_clip_rect(ji_screen, x+1, y1, x2, y2))
      draw_textstring(NULL, 
		      get_slider_empty_text_color(),
		      get_slider_empty_face_color(), false, widget, r, 0);

    set_clip(ji_screen, cx1, cy1, cx2, cy2);

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

  draw_bounds(x1, y1, x2, y2,
	      jwidget_has_focus(widget) ? PART_SUNKEN2_FOCUSED_NW:
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
      if (jwidget_has_focus(widget))
	bg = COLOR_SELECTED;
      else
	bg = COLOR_DISABLED;
      fg = COLOR_BACKGROUND;
    }

    /* disabled */
    if (jwidget_is_disabled (widget)) {
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
    if ((c == cursor) && (state) && (jwidget_has_focus (widget)))
      draw_entry_cursor(widget, cursor_x, y);
  }

  /* draw the cursor if it is next of the last character */
  if ((c == cursor) && (state) &&
      (jwidget_has_focus(widget)) &&
      (jwidget_is_enabled(widget)))
    draw_entry_cursor(widget, x, y);
}

void SkinneableTheme::draw_combobox_button(JWidget widget, JRect clip)
{
  BITMAP* icon_bmp = m_part[PART_COMBOBOX_ARROW];
  struct jrect icon;
  int x1, y1, x2, y2;
  int fg, bg, part_nw;

  /* with mouse */
  if (jwidget_is_selected(widget) ||
      (jwidget_is_enabled(widget) && jwidget_has_mouse(widget))) {
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
  draw_bounds(x1, y1, x2, y2, part_nw, bg);

  // icon
  icon.x1 = (x1+x2)/2 - icon_bmp->w/2;
  icon.y1 = (y1+y2)/2 - icon_bmp->h/2;
  icon.x2 = icon.x1 + icon_bmp->w;
  icon.y2 = icon.y1 + icon_bmp->h;

  if (jwidget_is_selected(widget))
    jrect_displace(&icon,
		   get_button_selected_offset(),
		   get_button_selected_offset());

  draw_sprite(ji_screen, icon_bmp, icon.x1, icon.y1);
}

void SkinneableTheme::draw_textbox(JWidget widget, JRect clip)
{
  _ji_theme_textbox_draw(ji_screen, widget, NULL, NULL,
			 widget->theme->textbox_bg_color,
			 widget->theme->textbox_fg_color);
}

void SkinneableTheme::draw_view(JWidget widget, JRect clip)
{
  draw_bounds(widget->rc->x1,
	      widget->rc->y1,
	      widget->rc->x2-1,
	      widget->rc->y2-1,
	      jwidget_has_focus(widget) ? PART_SUNKEN_FOCUSED_NW:
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

  /* border */
  rect(ji_screen, x1, y1, x2, y2, BGCOLOR);

  /* draw the content */
  x1++, y1++, x2--, y2--;

  /* horizontal bar */
  if (widget->getAlign() & JI_HORIZONTAL) {
    u1 = x1+pos;
    v1 = y1;
    u2 = x1+pos+len-1;
    v2 = y2;
  }
  /* vertical bar */
  else {
    u1 = x1;
    v1 = y1+pos;
    u2 = x2;
    v2 = y1+pos+len-1;
  }

  /* background */
  jrectexclude(ji_screen,
	       x1, y1, x2, y2,
	       u1, v1, u2, v2, BGCOLOR);

  /* 1st border */
  if (jwidget_is_selected(widget))
    jrectedge(ji_screen, u1, v1, u2, v2,
	      COLOR_DISABLED, COLOR_BACKGROUND);
  else
    jrectedge(ji_screen, u1, v1, u2, v2,
	      COLOR_BACKGROUND, COLOR_DISABLED);

  /* bar-block background */
  u1++, v1++, u2--, v2--;
  if (jwidget_is_enabled(widget) && jwidget_has_mouse(widget))
    rectfill(ji_screen, u1, v1, u2, v2, COLOR_HOTFACE);
  else
    rectfill(ji_screen, u1, v1, u2, v2, BGCOLOR);
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
      draw_bounds(pos->x1,
		  pos->y1,
		  pos->x2-1,
		  pos->y2-1, PART_WINDOW_NW,
		  get_window_face_color());

      pos->y2 = cpos->y1;

      // titlebar
      jdraw_text(window->getFont(), window->getText(),
		 cpos->x1,
		 pos->y1+jrect_h(pos)/2-text_height(window->getFont())/2,
		 COLOR_BACKGROUND, -1, false, jguiscale());
    }
    // menubox
    else {
      draw_bounds(pos->x1,
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
  else if (jwidget_has_mouse(widget))
    part = PART_WINDOW_CLOSE_BUTTON_HOT;
  else
    part = PART_WINDOW_CLOSE_BUTTON_NORMAL;

  draw_sprite(ji_screen, m_part[part], widget->rc->x1, widget->rc->y1);
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

    if (jwidget_is_selected (widget)) {
      x += selected_offset;
      y += selected_offset;
    }

    /* background */
    if (bg_color >= 0) {
      if (jwidget_is_disabled (widget))
	rectfill(ji_screen, x, y, x+w, y+h, bg_color);
      else
	rectfill(ji_screen, x, y, x+w-1, y+h-1, bg_color);
    }

    /* text */
    if (jwidget_is_disabled (widget)) {
      /* TODO avoid this */
      if (fill_bg)		/* only to draw the background */
	jdraw_text(widget->getFont(), t, x, y, 0, bg_color, fill_bg, jguiscale());

      /* draw white part */
      jdraw_text(widget->getFont(), t, x+1, y+1,
		 COLOR_BACKGROUND, bg_color, fill_bg, jguiscale());

      if (fill_bg)
	fill_bg = false;
    }

    jdraw_text(widget->getFont(), t, x, y,
	       jwidget_is_disabled(widget) ?
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

void SkinneableTheme::draw_bounds(int x1, int y1, int x2, int y2, int nw, int bg)
{
  int x, y;

  // top

  draw_sprite(ji_screen, m_part[nw], x1, y1);
  for (x = x1+m_part[nw]->w;
       x < x2-m_part[nw+2]->w-m_part[nw+1]->w+1;
       x += m_part[nw+1]->w) {
    draw_sprite(ji_screen, m_part[nw+1], x, y1);
  }
  draw_sprite(ji_screen, m_part[nw+1], x2-m_part[nw+2]->w-m_part[nw+1]->w+1, y1);
  draw_sprite(ji_screen, m_part[nw+2], x2-m_part[nw+2]->w+1, y1);

  // bottom

  draw_sprite(ji_screen, m_part[nw+6], x1, y2-m_part[nw+6]->h+1);
  for (x = x1+m_part[nw+6]->w;
       x < x2-m_part[nw+4]->w-m_part[nw+5]->w+1;
       x += m_part[nw+5]->w) {
    draw_sprite(ji_screen, m_part[nw+5], x, y2-m_part[nw+5]->h+1);
  }
  draw_sprite(ji_screen, m_part[nw+5], x2-m_part[nw+4]->w-m_part[nw+5]->w+1, y2-m_part[nw+5]->h+1);
  draw_sprite(ji_screen, m_part[nw+4], x2-m_part[nw+4]->w+1, y2-m_part[nw+4]->h+1);

  // left

  for (y = y1+m_part[nw]->h;
       y < y2-m_part[nw+6]->h-m_part[nw+7]->h+1;
       y += m_part[nw+7]->h) {
    draw_sprite(ji_screen, m_part[nw+7], x1, y);
  }
  draw_sprite(ji_screen, m_part[nw+7], x1, y2-m_part[nw+6]->h-m_part[nw+7]->h+1);

  // right

  for (y = y1+m_part[nw+2]->h;
       y < y2-m_part[nw+4]->h-m_part[nw+3]->h+1;
       y += m_part[nw+3]->h) {
    draw_sprite(ji_screen, m_part[nw+3], x2-m_part[nw+3]->w+1, y);
  }
  draw_sprite(ji_screen, m_part[nw+3], x2-m_part[nw+3]->w+1, y2-m_part[nw+4]->h-m_part[nw+3]->h+1);

  // background 
  if (bg >= 0) {
    x1 += m_part[nw+7]->w;
    y1 += m_part[nw+1]->h;
    x2 -= m_part[nw+3]->w;
    y2 -= m_part[nw+5]->h;

    rectfill(ji_screen, x1, y1, x2, y2, bg);
  }
}

void SkinneableTheme::draw_bounds2(int x1, int y1, int x2, int y2, int x_mid, int nw1, int nw2, int bg1, int bg2)
{
  int cx1, cy1, cx2, cy2;

  /* TODO when Allegro 4.1 will be officially released, replace this
     with the get_clip_rect, add_clip_rect, set_clip_rect
     functions */

  cx1 = ji_screen->cl;
  cy1 = ji_screen->ct;
  cx2 = ji_screen->cr-1;
  cy2 = ji_screen->cb-1;

  if (my_add_clip_rect(ji_screen, x1, y1, x_mid, y2))
    draw_bounds(x1, y1, x2, y2, nw1, bg1);

  set_clip(ji_screen, cx1, cy1, cx2, cy2);

  if (my_add_clip_rect(ji_screen, x_mid+1, y1, x2, y2))
    draw_bounds(x1, y1, x2, y2, nw2, bg2);

  set_clip(ji_screen, cx1, cy1, cx2, cy2);
}

void SkinneableTheme::draw_hline(int x1, int y1, int x2, int y2, int part)
{
  int x;

  for (x = x1;
       x <= x2-m_part[part]->w;
       x += m_part[part]->w) {
    draw_sprite(ji_screen, m_part[part], x, y1);
  }

  if (x <= x2)
    blit(m_part[part], ji_screen, 0, 0, x, y1, x2-x+1, m_part[part]->h);
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
	jwidget_select(widget);
	return true;
      }
      break;

    case JM_KEYRELEASED:
      if (msg->key.scancode == KEY_ESC) {
	if (jwidget_is_selected(widget)) {
	  jwidget_deselect(widget);
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
