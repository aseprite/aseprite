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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "Vaca/Bind.h"
#include "jinete/jinete.h"

#include "app.h"
#include "commands/command.h"
#include "commands/params.h"
#include "core/cfg.h"
#include "core/color.h"
#include "dialogs/filesel.h"
#include "modules/gui.h"
#include "modules/editors.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "util/quantize.h"
#include "widgets/colbar.h"
#include "widgets/colview.h"
#include "widgets/editor.h"
#include "widgets/paledit.h"
#include "sprite_wrappers.h"
#include "ui_context.h"

static Frame* window = NULL;
static int redraw_timer_id = -1;
static bool redraw_all = false;

// Slot for App::Exit signal 
static void on_exit_delete_this_widget()
{
  assert(window != NULL);

  jmanager_remove_timer(redraw_timer_id);
  redraw_timer_id = -1;

  jwidget_free(window);
}

//////////////////////////////////////////////////////////////////////
// palette_editor

class PaletteEditorCommand : public Command
{
public:
  PaletteEditorCommand();
  Command* clone() { return new PaletteEditorCommand(*this); }

protected:
  void load_params(Params* params);
  void execute(Context* context);

private:
  bool m_open;
  bool m_close;
  bool m_switch;
  bool m_background;
};

// #define get_sprite(wgt) (*(const SpriteReader*)(wgt->getRoot())->user_data[0])

static Widget *R_label, *G_label, *B_label;
static Widget *H_label, *S_label, *V_label;
static Widget *R_slider, *G_slider, *B_slider;
static Widget *H_slider, *S_slider, *V_slider;
static Widget *R_entry, *G_entry, *B_entry;
static Widget *H_entry, *S_entry, *V_entry;
static Widget *hex_entry;
static PalEdit* palette_editor;
static bool disable_colorbar_signals = false;

static bool window_msg_proc(JWidget widget, JMessage msg);
static bool window_close_hook(JWidget widget, void *data);
static void load_command(JWidget widget);
static void save_command(JWidget widget);
static void ramp_command(JWidget widget);
static void quantize_command(JWidget widget);

static bool sliderRGB_change_hook(JWidget widget, void *data);
static bool sliderHSV_change_hook(JWidget widget, void *data);
static bool entryRGB_change_hook(JWidget widget, void *data);
static bool entryHSV_change_hook(JWidget widget, void *data);
static bool hex_entry_change_hook(JWidget widget, void *data);
static void update_entries_from_sliders();
static void update_sliders_from_entries();
static void update_hex_entry();
static void update_current_sprite_palette();
static void update_colorbar();
static bool palette_editor_change_hook(JWidget widget, void *data);
static bool select_rgb_hook(JWidget widget, void *data);
static bool select_hsv_hook(JWidget widget, void *data);
static void modify_all_selected_entries_in_palette(int r, int g, int b);
static void on_color_changed(color_t color);

static void set_new_palette(Palette *palette);

PaletteEditorCommand::PaletteEditorCommand()
  : Command("palette_editor",
	    "PaletteEditor",
	    CmdRecordableFlag)
{
  m_open = true;
  m_close = false;
  m_switch = false;
  m_background = false;
}

void PaletteEditorCommand::load_params(Params* params)
{
  std::string target = params->get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;

  std::string open_str = params->get("open");
  if (open_str == "true") m_open = true;
  else m_open = false;

  std::string close_str = params->get("close");
  if (close_str == "true") m_close = true;
  else m_close = false;

  std::string switch_str = params->get("switch");
  if (switch_str == "true") m_switch = true;
  else m_switch = false;
}

void PaletteEditorCommand::execute(Context* context)
{
  Widget* palette_editor_view;
  Widget* select_rgb;
  Widget* select_hsv;
  bool first_time = false;

  // If the window was never loaded yet, load it
  if (!window) {
    if (m_close)
      return;			// Do nothing (the user want to close and inexistent window)

    // Load the palette editor window
    window = static_cast<Frame*>(load_widget("palette_editor.xml", "palette_editor"));
    redraw_timer_id = jmanager_add_timer(window, 250);

    first_time = true;

    // Append hooks
    window->Close.connect(Vaca::Bind<bool>(&window_close_hook, (JWidget)window, (void*)0));

    // Hook fg/bg color changes (by eyedropper mainly)
    app_get_colorbar()->FgColorChange.connect(&on_color_changed);
    app_get_colorbar()->BgColorChange.connect(&on_color_changed);

    // Hook App::Exit signal
    App::instance()->Exit.connect(&on_exit_delete_this_widget);
  }
  // If the window is opened, close it (only in "switch" mode)
  else if (jwidget_is_visible(window) && (m_switch || m_close)) {
    window->closeWindow(NULL);
    return;
  }

  get_widgets(window,
	      "R_label",  &R_label,
	      "R_slider", &R_slider,
	      "R_entry",  &R_entry,
	      "G_label",  &G_label,
	      "G_slider", &G_slider,
	      "G_entry",  &G_entry,
	      "B_label",  &B_label,
	      "B_slider", &B_slider,
	      "B_entry",  &B_entry,
	      "H_label",  &H_label,
	      "H_slider", &H_slider,
	      "H_entry",  &H_entry,
	      "S_label",  &S_label,
	      "S_slider", &S_slider,
	      "S_entry",  &S_entry,
	      "V_label",  &V_label,
	      "V_slider", &V_slider,
	      "V_entry",  &V_entry,
	      "hex_entry", &hex_entry,
	      "select_rgb", &select_rgb,
	      "select_hsv", &select_hsv,
	      // "load", &button_load,
	      // "save", &button_save,
	      // "ramp", &button_ramp,
	      // "quantize", &button_quantize,
	      // "button_ok", &button_ok,
	      // "colorviewer_box", &colorviewer_box,
	      "palette_editor", &palette_editor_view, NULL);

  // window->user_data[0] = (void*)&sprite;
      
  // Custom widgets
  if (first_time) {
    // colorviewer = colorviewer_new(color_index(0), IMAGE_INDEXED);
    palette_editor = new PalEdit(true);
    palette_editor->setBoxSize(4);

    // jwidget_expansive(colorviewer, true);
    // jwidget_add_child(colorviewer_box, colorviewer);

    // jwidget_disable(button_undo);
    // jwidget_disable(button_redo);

    jview_attach(palette_editor_view, palette_editor);
    jview_maxsize(palette_editor_view);

    // Set palette editor columns
    palette_editor->setColumns(16);
  
    // Hook signals
    jwidget_add_hook(window, -1, window_msg_proc, NULL);
    HOOK(R_slider, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_hook, 0);
    HOOK(G_slider, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_hook, 0);
    HOOK(B_slider, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_hook, 0);
    HOOK(H_slider, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_hook, 0);
    HOOK(S_slider, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_hook, 0);
    HOOK(V_slider, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_hook, 0);
    HOOK(R_entry, JI_SIGNAL_ENTRY_CHANGE, entryRGB_change_hook, 0);
    HOOK(G_entry, JI_SIGNAL_ENTRY_CHANGE, entryRGB_change_hook, 0);
    HOOK(B_entry, JI_SIGNAL_ENTRY_CHANGE, entryRGB_change_hook, 0);
    HOOK(H_entry, JI_SIGNAL_ENTRY_CHANGE, entryHSV_change_hook, 0);
    HOOK(S_entry, JI_SIGNAL_ENTRY_CHANGE, entryHSV_change_hook, 0);
    HOOK(V_entry, JI_SIGNAL_ENTRY_CHANGE, entryHSV_change_hook, 0);
    HOOK(hex_entry, JI_SIGNAL_ENTRY_CHANGE, hex_entry_change_hook, 0);
    HOOK(palette_editor, SIGNAL_PALETTE_EDITOR_CHANGE, palette_editor_change_hook, 0);
    HOOK(select_rgb, JI_SIGNAL_RADIO_CHANGE, select_rgb_hook, 0);
    HOOK(select_hsv, JI_SIGNAL_RADIO_CHANGE, select_hsv_hook, 0);

    setup_mini_look(select_rgb);
    setup_mini_look(select_hsv);

    // jbutton_add_command(button_load, load_command);
    // jbutton_add_command(button_save, save_command);
    // jbutton_add_command(button_ramp, ramp_command);
    // jbutton_add_command(button_quantize, quantize_command);

    select_rgb_hook(NULL, NULL);
  }

  // Show the specified target color
  {
    int imgtype = app_get_current_image_type();
    color_t color =
      (m_background ? context->getSettings()->getBgColor():
  		      context->getSettings()->getFgColor());

    on_color_changed(color);
  }

  if (m_switch || m_open) {
    if (!jwidget_is_visible(window)) {
      // Default position
      window->remap_window();
      window->center_window();

      // Load window configuration
      load_window_pos(window, "PaletteEditor");
    }

    // Run the window in background
    window->open_window_bg();
  }
}

static bool window_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_TIMER &&
      msg->timer.timer_id == redraw_timer_id) {
    // Redraw all editors
    if (redraw_all) {
      redraw_all = false;
      jmanager_stop_timer(redraw_timer_id);

      try {
	const CurrentSpriteReader sprite(UIContext::instance());
	update_editors_with_sprite(sprite);
      }
      catch (...) {
	// Do nothing
      }
    }
    // Redraw just the current editor
    else {
      redraw_all = true;
      current_editor->editor_update();
    }
  }
  return false;
}

static bool window_close_hook(JWidget widget, void *data)
{
  // Save window configuration
  save_window_pos(window, "PaletteEditor");
  return false;
}

static void load_command(JWidget widget)
{
  Palette *palette;
  jstring filename = ase_file_selector(_("Load Palette"), "", "png,pcx,bmp,tga,lbm,col");
  if (!filename.empty()) {
    palette = Palette::load(filename.c_str());
    if (!palette) {
      jalert(_("Error<<Loading palette file||&Close"));
    }
    else {
      set_new_palette(palette);
      delete palette;
    }
  }
}

static void save_command(JWidget widget)
{
  jstring filename;
  int ret;

 again:
  filename = ase_file_selector(_("Save Palette"), "", "png,pcx,bmp,tga,col");
  if (!filename.empty()) {
    if (exists(filename.c_str())) {
      ret = jalert("%s<<%s<<%s||%s",
		   _("Warning"),
		   _("File exists, overwrite it?"),
		   get_filename(filename.c_str()),
		   _("&Yes||&No||&Cancel"));

      if (ret == 2)
	goto again;
      else if (ret != 1)
	return;
    }

    // TODO
    // if (!palette_editor->getPalette()->save(filename.c_str())) {
    //   jalert(_("Error<<Saving palette file||&Close"));
    // }
  }
}

static void ramp_command(JWidget widget)
{
#if 0
  int range_type = palette_editor->getRangeType();
  int i1 = palette_editor->get1stColor();
  int i2 = palette_editor->get2ndColor();
  Palette* palette = new Palette(0, 256);
  bool array[256];

  palette_editor->getSelectedEntries(array);
  palette_editor->getPalette()->copyColorsTo(palette);

  if ((i1 >= 0) && (i2 >= 0)) {
    /* make the ramp */
    if (range_type == PALETTE_EDITOR_RANGE_LINEAL) {
      /* lineal ramp */
      palette->makeHorzRamp(i1, i2);
    }
    else if (range_type == PALETTE_EDITOR_RANGE_RECTANGULAR) {
      /* rectangular ramp */
      palette->makeRectRamp(i1, i2, paledit_get_columns(palette_editor));
    }
  }

  set_new_palette(palette);
  delete palette;
#endif
}

static void quantize_command(JWidget widget)
{
  // const SpriteReader& sprite = get_sprite(widget);
  // assert(sprite != NULL);

  // Palette* palette = new Palette(0, 256);
  // bool array[256];

  // paledit_get_selected_entries(palette_editor, array);
  // paledit_get_palette(palette_editor)->copyColorsTo(palette);

  // if (sprite->getImgType() == IMAGE_RGB) {
  //   SpriteWriter sprite_writer(sprite);
  //   sprite_quantize_ex(sprite_writer, palette);
  // }
  // else {
  //   jalert(_("Error<<You can use this command only for RGB sprites||&OK"));
  // }

  // set_new_palette(palette);
  // delete palette;
}

static bool sliderRGB_change_hook(JWidget widget, void *data)
{
  Palette* palette = get_current_palette();
  int r = jslider_get_value(R_slider);
  int g = jslider_get_value(G_slider);
  int b = jslider_get_value(B_slider);
  color_t color = color_rgb(r, g, b);

  jslider_set_value(H_slider, color_get_hue(color));
  jslider_set_value(V_slider, color_get_value(color));
  jslider_set_value(S_slider, color_get_saturation(color));

  modify_all_selected_entries_in_palette(r, g, b);

  update_entries_from_sliders();
  update_hex_entry();
  update_current_sprite_palette();
  update_colorbar();
  return false;
}

static bool sliderHSV_change_hook(JWidget widget, void *data)
{
  Palette* palette = get_current_palette();
  int h = jslider_get_value(H_slider);
  int s = jslider_get_value(S_slider);
  int v = jslider_get_value(V_slider);
  color_t color = color_hsv(h, s, v);
  int r, g, b;

  jslider_set_value(R_slider, r = color_get_red(color));
  jslider_set_value(G_slider, g = color_get_green(color));
  jslider_set_value(B_slider, b = color_get_blue(color));

  modify_all_selected_entries_in_palette(r, g, b);

  update_entries_from_sliders();
  update_hex_entry();
  update_current_sprite_palette();
  update_colorbar();
  return false;
}

static bool entryRGB_change_hook(JWidget widget, void *data)
{
  int r = R_entry->getTextInt();
  int g = G_entry->getTextInt();
  int b = B_entry->getTextInt();
  r = MID(0, r, 255);
  g = MID(0, g, 255);
  b = MID(0, b, 255);
  color_t color = color_rgb(r, g, b);

  H_entry->setTextf("%d", color_get_hue(color));
  V_entry->setTextf("%d", color_get_value(color));
  S_entry->setTextf("%d", color_get_saturation(color));

  modify_all_selected_entries_in_palette(r, g, b);

  update_sliders_from_entries();
  update_hex_entry();
  update_current_sprite_palette();
  update_colorbar();
  return false;
}

static bool entryHSV_change_hook(JWidget widget, void *data)
{
  Palette* palette = get_current_palette();
  int h = H_entry->getTextInt();
  int s = S_entry->getTextInt();
  int v = V_entry->getTextInt();
  color_t color = color_hsv(h, s, v);
  int r, g, b;

  R_entry->setTextf("%d", r = color_get_red(color));
  G_entry->setTextf("%d", g = color_get_green(color));
  B_entry->setTextf("%d", b = color_get_blue(color));

  modify_all_selected_entries_in_palette(r, g, b);

  update_sliders_from_entries();
  update_hex_entry();
  update_current_sprite_palette();
  update_colorbar();
  return false;
}

static bool hex_entry_change_hook(JWidget widget, void *data)
{
  Palette* palette = get_current_palette();
  std::string text = hex_entry->getText();
  int r, g, b;
  float h, s, v;
  bool array[256];
  int c;

  // Fill with zeros at the end of the text
  while (text.size() < 6)
    text.push_back('0');

  // Convert text (Base 16) to integer
  int hex = strtol(text.c_str(), NULL, 16);

  jslider_set_value(R_slider, r = ((hex & 0xff0000) >> 16));
  jslider_set_value(G_slider, g = ((hex & 0xff00) >> 8));
  jslider_set_value(B_slider, b = ((hex & 0xff)));

  rgb_to_hsv(r, g, b, &h, &s, &v);

  palette_editor->getSelectedEntries(array);
  for (c=0; c<256; c++) {
    if (array[c]) {
      palette->setEntry(c, _rgba(r, g, b, 255));
    }
  }

  jslider_set_value(H_slider, 255.0 * h / 360.0);
  jslider_set_value(V_slider, 255.0 * v);
  jslider_set_value(S_slider, 255.0 * s);

  update_entries_from_sliders();
  update_current_sprite_palette();
  update_colorbar();
  return false;
}

static void update_entries_from_sliders()
{
  R_entry->setTextf("%d", jslider_get_value(R_slider));
  G_entry->setTextf("%d", jslider_get_value(G_slider));
  B_entry->setTextf("%d", jslider_get_value(B_slider));

  H_entry->setTextf("%d", jslider_get_value(H_slider));
  S_entry->setTextf("%d", jslider_get_value(S_slider));
  V_entry->setTextf("%d", jslider_get_value(V_slider));
}

static void update_sliders_from_entries()
{
  jslider_set_value(R_slider, R_entry->getTextInt());
  jslider_set_value(G_slider, G_entry->getTextInt());
  jslider_set_value(B_slider, B_entry->getTextInt());

  jslider_set_value(H_slider, H_entry->getTextInt());
  jslider_set_value(S_slider, S_entry->getTextInt());
  jslider_set_value(V_slider, V_entry->getTextInt());
}

static void update_hex_entry()
{
  hex_entry->setTextf("%02x%02x%02x",
		      jslider_get_value(R_slider),
		      jslider_get_value(G_slider),
		      jslider_get_value(B_slider));
}

static void update_current_sprite_palette()
{
  if (UIContext::instance()->get_current_sprite()) {
    try {
      CurrentSpriteWriter sprite(UIContext::instance());
      sprite->setPalette(get_current_palette(), false);
    }
    catch (...) {
      // Ignore
    }
  }

  jwidget_dirty(palette_editor);

  if (!jmanager_timer_is_running(redraw_timer_id))
    jmanager_start_timer(redraw_timer_id);
  redraw_all = false;
}

static void update_colorbar()
{
  app_get_colorbar()->dirty();
}

static void update_sliders_from_color(color_t color)
{
  jslider_set_value(R_slider, color_get_red(color));
  jslider_set_value(G_slider, color_get_green(color));
  jslider_set_value(B_slider, color_get_blue(color));
  jslider_set_value(H_slider, color_get_hue(color));
  jslider_set_value(S_slider, color_get_saturation(color));
  jslider_set_value(V_slider, color_get_hue(color));
}

static bool palette_editor_change_hook(JWidget widget, void *data)
{
  int imgtype = app_get_current_image_type();
  color_t color = color_index(palette_editor->get2ndColor());

  // colorviewer_set_color(colorviewer, color);

  {
    disable_colorbar_signals = true;

    if (jmouse_b(0) & 2)
      app_get_colorbar()->setBgColor(color);
    else
      app_get_colorbar()->setFgColor(color);

    disable_colorbar_signals = false;
  }

  update_sliders_from_color(color); // Update sliders
  update_entries_from_sliders();    // Update entries
  update_hex_entry();		    // Update hex field
  return false;
}

static bool select_rgb_hook(JWidget widget, void *data)
{
  jwidget_show(R_label);
  jwidget_show(R_slider);
  jwidget_show(R_entry);
  jwidget_show(G_label);
  jwidget_show(G_slider);
  jwidget_show(G_entry);
  jwidget_show(B_label);
  jwidget_show(B_slider);
  jwidget_show(B_entry);

  jwidget_hide(H_label);
  jwidget_hide(H_slider);
  jwidget_hide(H_entry);
  jwidget_hide(S_label);
  jwidget_hide(S_slider);
  jwidget_hide(S_entry);
  jwidget_hide(V_label);
  jwidget_hide(V_slider);
  jwidget_hide(V_entry);

  window->setBounds(window->getBounds());
  window->dirty();

  return true;
}

static bool select_hsv_hook(JWidget widget, void *data)
{
  jwidget_hide(R_label);
  jwidget_hide(R_slider);
  jwidget_hide(R_entry);
  jwidget_hide(G_label);
  jwidget_hide(G_slider);
  jwidget_hide(G_entry);
  jwidget_hide(B_label);
  jwidget_hide(B_slider);
  jwidget_hide(B_entry);

  jwidget_show(H_label);
  jwidget_show(H_slider);
  jwidget_show(H_entry);
  jwidget_show(S_label);
  jwidget_show(S_slider);
  jwidget_show(S_entry);
  jwidget_show(V_label);
  jwidget_show(V_slider);
  jwidget_show(V_entry);

  window->setBounds(window->getBounds());
  window->dirty();

  return true;
}

static void modify_all_selected_entries_in_palette(int r, int g, int b)
{
  bool array[256];
  palette_editor->getSelectedEntries(array);

  Palette* palette = get_current_palette();
  for (int c=0; c<256; c++)
    if (array[c])
      palette->setEntry(c, _rgba(r, g, b, 255));
}

static void on_color_changed(color_t color)
{
  if (disable_colorbar_signals)
    return;

  int imgtype = app_get_current_image_type();
  int index = color_get_index(color);
  palette_editor->selectColor(index);

  update_sliders_from_color(color); // Update sliders
  update_entries_from_sliders();    // Update entries
  update_hex_entry();		    // Update hex field

  jwidget_flush_redraw(window);
}

static void set_new_palette(Palette* palette)
{
  // Copy the palette
  palette->copyColorsTo(get_current_palette());

  // Set the palette calling the hooks
  set_current_palette(palette, false);

  // Redraw the entire screen
  jmanager_refresh_screen();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_palette_editor_command()
{
  return new PaletteEditorCommand;
}
