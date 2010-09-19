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
#include <utility>
#include <vector>

#include "jinete/jinete.h"

#include "app.h"
#include "commands/command.h"
#include "console.h"
#include "gfxmode.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "resource_finder.h"
#include "sprite_wrappers.h"

#include "tinyxml.h"

static int timer_to_accept;
static int seconds_to_accept;

static bool alert_msg_proc(JWidget widget, JMessage msg);

//////////////////////////////////////////////////////////////////////

class ConfigureScreen : public Command
{
public:
  ConfigureScreen();
  Command* clone() const { return new ConfigureScreen(*this); }

protected:
  void onExecute(Context* context);

private:
  void load_resolutions(ComboBox* resolution, ComboBox* color_depth, ComboBox* pixel_scale);

  GfxMode m_newMode;
  std::vector<std::pair<int, int> > m_resolutions;
  std::vector<int> m_colordepths;
  std::vector<int> m_pixelscale;
};

ConfigureScreen::ConfigureScreen()
  : Command("configure_screen",
	    "Configure Screen",
	    CmdUIOnlyFlag)
{
}

void ConfigureScreen::onExecute(Context* context)
{
  CurrentGfxModeGuard currentGfxModeGuard;
  m_newMode = currentGfxModeGuard.getOriginal(); // Default values

  ComboBox* resolution, *color_depth, *pixel_scale;
  Widget* fullscreen;
  FramePtr window(load_widget("configure_screen.xml", "configure_screen"));
  get_widgets(window,
	      "resolution", &resolution,
	      "color_depth", &color_depth,
	      "pixel_scale", &pixel_scale,
	      "fullscreen", &fullscreen, NULL);

  load_resolutions(resolution, color_depth, pixel_scale);

  fullscreen->setSelected(!is_windowed_mode());

  window->open_window_fg();

  if (window->get_killer() == jwidget_find_name(window, "ok")) {
    m_newMode.setWidth(m_resolutions[resolution->getSelectedItem()].first);
    m_newMode.setHeight(m_resolutions[resolution->getSelectedItem()].second);
    m_newMode.setDepth(m_colordepths[color_depth->getSelectedItem()]);
    m_newMode.setScaling(m_pixelscale[pixel_scale->getSelectedItem()]);
    m_newMode.setCard(fullscreen->isSelected() ? GFX_AUTODETECT_FULLSCREEN:
						 GFX_AUTODETECT_WINDOWED);

    // Setup graphics mode
    if (currentGfxModeGuard.tryGfxMode(m_newMode)) {
      FramePtr alert_window(jalert_new("Confirm Screen"
				       "<<Do you want to keep this screen resolution?"
				       "<<In 10 seconds the screen will be restored."
				       "||&Yes||&No"));
      jwidget_add_hook(alert_window, -1, alert_msg_proc, NULL);

      seconds_to_accept = 10;
      timer_to_accept = jmanager_add_timer(alert_window, 1000);
      jmanager_start_timer(timer_to_accept);

      alert_window->open_window_fg();
      jmanager_remove_timer(timer_to_accept);

      if (alert_window->get_killer() != NULL &&
	  ustrcmp(alert_window->get_killer()->getName(), "button-1") == 0) {
	// Keep the current graphics mode
	currentGfxModeGuard.keep();
      }
    }
  }

  // "currentGfxModeGuard" destruction keeps the new graphics mode or restores the old one
}

void ConfigureScreen::load_resolutions(ComboBox* resolution, ComboBox* color_depth, ComboBox* pixel_scale)
{
  char buf[512];
  bool old_res_selected = false;
  int newItem;

  m_resolutions.clear();
  m_colordepths.clear();
  m_pixelscale.clear();

  // Read from gui.xml
  ResourceFinder rf;
  rf.findInDataDir("gui.xml");

  while (const char* path = rf.next()) {
    PRINTF("Trying to load screen resolutions file from \"%s\"...\n", path);

    if (!exists(path))
      continue;

    PRINTF(" - \"%s\" found\n", path);

    TiXmlDocument doc;
    if (!doc.LoadFile(path))
      throw ase_exception(&doc);

    TiXmlHandle handle(&doc);

    TiXmlElement* xmlElement = handle
      .FirstChild("gui")
      .FirstChild("resolutions")
      .FirstChildElement().ToElement();

    while (xmlElement) {
      if (strcmp(xmlElement->Value(), "screensize") == 0) {
	int w = ustrtol(xmlElement->Attribute("width"), NULL, 10);
	int h = ustrtol(xmlElement->Attribute("height"), NULL, 10);
	const char* aspect = xmlElement->Attribute("aspect");

	if (w > 0 && h > 0) {
	  m_resolutions.push_back(std::make_pair(w, h));

	  if (aspect)
	    sprintf(buf, "%dx%d (%s)", w, h, aspect);
	  else
	    sprintf(buf, "%dx%d", w, h);

	  newItem = resolution->addItem(buf);
	  if (m_newMode.getWidth() == w && m_newMode.getHeight() == h) {
	    old_res_selected = true;
	    resolution->setSelectedItem(newItem);
	  }
	}
      }
      else if (strcmp(xmlElement->Value(), "colordepth") == 0) {
	int bpp = ustrtol(xmlElement->Attribute("bpp"), NULL, 10);
	const char* label = xmlElement->Attribute("label");

	if (bpp > 0 && label) {
	  m_colordepths.push_back(bpp);

	  newItem = color_depth->addItem(label);
	  if (m_newMode.getDepth() == bpp)
	    color_depth->setSelectedItem(newItem);
	}
      }
      else if (strcmp(xmlElement->Value(), "pixelscale") == 0) {
	int factor = ustrtol(xmlElement->Attribute("factor"), NULL, 10);
	const char* label = xmlElement->Attribute("label");

	if (factor > 0 && label) {
	  m_pixelscale.push_back(factor);

	  newItem = pixel_scale->addItem(label);
	  if (m_newMode.getScaling() == factor)
	    pixel_scale->setSelectedItem(newItem);
	}
      }

      xmlElement = xmlElement->NextSiblingElement();
    }

    // We just load the first gui.xml found
    break;
  }

  // Current screen size
  if (!old_res_selected) {
    m_resolutions.insert(m_resolutions.begin(), std::make_pair(m_newMode.getWidth(), m_newMode.getHeight()));

    sprintf(buf, "%dx%d (Current)", m_resolutions[0].first, m_resolutions[0].second);
    resolution->insertItem(0, buf);
    resolution->setSelectedItem(0);
  }
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
      ((JWidget)labels->end->next->next->data)->setText(buf);
      jlist_free(labels);

      if (seconds_to_accept == 0) {
	jmanager_stop_timer(timer_to_accept);
	static_cast<Frame*>(widget)->closeWindow(NULL);
	return true;
      }
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_configure_screen_command()
{
  return new ConfigureScreen;
}
