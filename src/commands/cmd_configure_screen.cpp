/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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
#include <vector>
#include <utility>

#include "jinete/jinete.h"

#include "commands/command.h"
#include "console.h"
#include "core/app.h"
#include "core/dirs.h"
#include "intl/intl.h"
#include "dialogs/options.h"
#include "modules/gui.h"
#include "sprite_wrappers.h"
#include "modules/palettes.h"

static int new_card, new_w, new_h, new_depth, new_scaling;
static int old_card, old_w, old_h, old_depth, old_scaling;

static int timer_to_accept;
static int seconds_to_accept;

static void show_dialog(Context* context);
static bool try_new_gfx_mode(Context* context);
static bool alert_msg_proc(JWidget widget, JMessage msg);

//////////////////////////////////////////////////////////////////////

class ConfigureScreen : public Command
{
  std::vector<std::pair<int, int> > m_resolutions;
  std::vector<int> m_colordepths;
  std::vector<int> m_pixelscale;

public:
  ConfigureScreen();
  Command* clone() const { return new ConfigureScreen(*this); }

protected:
  void execute(Context* context);

private:
  void show_dialog(Context* context);
  void load_resolutions(JWidget resolution, JWidget color_depth, JWidget pixel_scale);
};

ConfigureScreen::ConfigureScreen()
  : Command("configure_screen",
	    "Configure Screen",
	    CmdUIOnlyFlag)
{
}

void ConfigureScreen::execute(Context* context)
{
  /* get the active status */
  old_card    = gfx_driver->id;
  old_w       = SCREEN_W;
  old_h       = SCREEN_H;
  old_depth   = bitmap_color_depth(screen);
  old_scaling = get_screen_scaling();

  /* default values */
  new_card = old_card;
  new_w = old_w;
  new_h = old_h;
  new_depth = old_depth;
  new_scaling = old_scaling;

  show_dialog(context);
}

void ConfigureScreen::show_dialog(Context* context)
{
  JWidget resolution, color_depth, pixel_scale, fullscreen;

  JWidgetPtr window(load_widget("confscr.jid", "configure_screen"));
  get_widgets(window,
	      "resolution", &resolution,
	      "color_depth", &color_depth,
	      "pixel_scale", &pixel_scale,
	      "fullscreen", &fullscreen, NULL);

  load_resolutions(resolution, color_depth, pixel_scale);

  if (is_windowed_mode())
    jwidget_deselect(fullscreen);
  else
    jwidget_select(fullscreen);

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == jwidget_find_name(window, "ok")) {
    new_w = m_resolutions[jcombobox_get_selected_index(resolution)].first;
    new_h = m_resolutions[jcombobox_get_selected_index(resolution)].second;
    new_depth = m_colordepths[jcombobox_get_selected_index(color_depth)];
    new_scaling = m_pixelscale[jcombobox_get_selected_index(pixel_scale)];
    new_card = jwidget_is_selected(fullscreen) ? GFX_AUTODETECT_FULLSCREEN:
						 GFX_AUTODETECT_WINDOWED;

    /* setup graphics mode */
    if (try_new_gfx_mode(context)) {
      JWidgetPtr alert_window(jalert_new("Confirm Screen"
					 "<<Do you want to keep this screen resolution?"
					 "<<In 10 seconds the screen will be restored."
					 "||&Yes||&No"));
      jwidget_add_hook(alert_window, -1, alert_msg_proc, NULL);

      seconds_to_accept = 10;
      timer_to_accept = jmanager_add_timer(alert_window, 1000);
      jmanager_start_timer(timer_to_accept);

      jwindow_open_fg(alert_window);
      jmanager_remove_timer(timer_to_accept);

      if (jwindow_get_killer(alert_window) != NULL &&
	  ustrcmp(jwidget_get_name(jwindow_get_killer(alert_window)), "button-1") == 0) {
	/* do nothing */
      }
      else {
	new_card = old_card;
	new_w = old_w;
	new_h = old_h;
	new_depth = old_depth;
	new_scaling = old_scaling;

	try_new_gfx_mode(context);
      }
    }
  }
}

void ConfigureScreen::load_resolutions(JWidget resolution, JWidget color_depth, JWidget pixel_scale)
{
  DIRS *dirs, *dir;
  JXml xml;
  char buf[512];

  dirs = filename_in_datadir("usergui.xml");
  {
    sprintf(buf, "gui-%s.xml", intl_get_lang());
    dirs_cat_dirs(dirs, filename_in_datadir(buf));
    dirs_cat_dirs(dirs, filename_in_datadir("gui-en.xml"));
  }

  m_resolutions.clear();
  m_colordepths.clear();
  m_pixelscale.clear();

  for (dir=dirs; dir; dir=dir->next) {
    PRINTF("Trying to load screen resolutions file from \"%s\"...\n", dir->path);
    
    // open the XML menu definition file
    xml = jxml_new_from_file(dir->path);
    if (xml && jxml_get_root(xml)) {
      JXmlElem xml_resolutions = jxmlelem_get_elem_by_name(jxml_get_root(xml), "resolutions");
      if (xml_resolutions) {
	JLink link;
	JI_LIST_FOR_EACH(((JXmlNode)xml_resolutions)->children, link) {
	  JXmlNode child = (JXmlNode)link->data;
	  JXmlElem elem = (JXmlElem)child;

	  if (strcmp(jxmlelem_get_name(elem), "screensize") == 0) {
	    int w = ustrtol(jxmlelem_get_attr(elem, "width"), NULL, 10);
	    int h = ustrtol(jxmlelem_get_attr(elem, "height"), NULL, 10);
	    const char* aspect = jxmlelem_get_attr(elem, "aspect");

	    if (w > 0 && h > 0) {
	      m_resolutions.push_back(std::make_pair(w, h));

	      if (aspect)
		sprintf(buf, "%dx%d (%s)", w, h, aspect);
	      else
		sprintf(buf, "%dx%d", w, h, aspect);

	      jcombobox_add_string(resolution, buf, NULL);
	      if (old_w == w && old_h == h)
		jcombobox_select_index(resolution, jcombobox_get_count(resolution)-1);
	    }
	  }
	  else if (strcmp(jxmlelem_get_name(elem), "colordepth") == 0) {
	    int bpp = ustrtol(jxmlelem_get_attr(elem, "bpp"), NULL, 10);
	    const char* label = jxmlelem_get_attr(elem, "label");

	    if (bpp > 0 && label) {
	      m_colordepths.push_back(bpp);

	      jcombobox_add_string(color_depth, label, NULL);
	      if (old_depth == bpp)
		jcombobox_select_index(color_depth, jcombobox_get_count(color_depth)-1);
	    }
	  }
	  else if (strcmp(jxmlelem_get_name(elem), "pixelscale") == 0) {
	    int factor = ustrtol(jxmlelem_get_attr(elem, "factor"), NULL, 10);
	    const char* label = jxmlelem_get_attr(elem, "label");

	    if (factor > 0 && label) {
	      m_pixelscale.push_back(factor);

	      jcombobox_add_string(pixel_scale, label, NULL);
	      if (old_scaling == factor)
		jcombobox_select_index(pixel_scale, jcombobox_get_count(pixel_scale)-1);
	    }
	  }
	}
      }

      // free the XML file
      jxml_free(xml);
      break;
    }
  }

  dirs_free(dirs);
}

static bool try_new_gfx_mode(Context* context)
{
  /* try change the new graphics mode */
  set_color_depth(new_depth);
  set_screen_scaling(new_scaling);
  if (set_gfx_mode(new_card, new_w, new_h, 0, 0) < 0) {
    /* error!, well, we need to return to the old graphics mode */
    set_color_depth(old_depth);
    set_screen_scaling(old_scaling);
    if (set_gfx_mode(old_card, old_w, old_h, 0, 0) < 0) {
      /* oh no! more errors!, we can't restore the old graphics mode! */
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      user_printf(_("FATAL ERROR: Unable to restore the old graphics mode!\n"));
      exit(1);
    }
    /* only print a message of the old error */
    else {
      gui_setup_screen(true);

      /* set to a black palette */
      set_black_palette();

      /* restore palette all screen stuff */
      {
	const CurrentSpriteReader sprite(context);
	app_refresh_screen(sprite);
      }

      Console console;
      console.printf(_("Error setting graphics mode: %dx%d %d bpp\n"),
		     new_w, new_h, new_depth);

      return false;
    }
  }
  /* the new graphics mode is working */
  else {
    gui_setup_screen(true);

    /* set to a black palette */
    set_black_palette();

    // restore palette all screen stuff
    {
      const CurrentSpriteReader sprite(context);
      app_refresh_screen(sprite);
    }
  }
  
  /* setup mouse */
  _setup_mouse_speed();

  /* redraw top window */
  if (app_get_top_window()) {
    jwindow_remap(app_get_top_window());
    jmanager_refresh_screen();
  }

  return true;
}

static bool alert_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_TIMER) {
    if (msg->timer.timer_id == timer_to_accept) {
      JList labels = jwidget_get_children(jwidget_find_name(widget, "labels"));
      char buf[512];

      seconds_to_accept -= msg->timer.count;
      seconds_to_accept = MAX(0, seconds_to_accept);

      usprintf(buf, "In %d seconds the screen will be restored.", seconds_to_accept);
      jwidget_set_text((JWidget)labels->end->next->next->data, buf);
      jlist_free(labels);

      if (seconds_to_accept == 0) {
	jmanager_stop_timer(timer_to_accept);
	jwindow_close(widget, NULL);
	return TRUE;
      }
    }
  }
  return FALSE;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_configure_screen_command()
{
  return new ConfigureScreen;
}
