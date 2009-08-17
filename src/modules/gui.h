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

#ifndef MODULES_GUI_H_INCLUDED
#define MODULES_GUI_H_INCLUDED

#include <cassert>
#include <string>
#include <list>
#include "ase_exception.h"
#include "jinete/jbase.h"
#include "jinete/jaccel.h"
#include "jinete/jwidget.h"

struct Command;
struct Tool;

//////////////////////////////////////////////////////////////////////

class widget_file_not_found : public ase_exception
{
public:
  widget_file_not_found(const char* file_name) throw()
    : ase_exception(std::string("Cannot load file: ") + file_name
		    + "\nPlease reinstall " PACKAGE) { }
};

/**
 * Exception thrown by find_widget() if a widget is not found.
 */
class widget_not_found : public ase_exception
{
public:
  widget_not_found(const char* widget_name) throw()
  : ase_exception("A data file is corrupted.\nPlease reinstall " PACKAGE
		  "\n\nDetails: Widget not found: " + std::string(widget_name)) { }
};

//////////////////////////////////////////////////////////////////////

#define HOOK(widget, signal, signal_handler, data)			\
  hook_signal((widget), (signal), (signal_handler), (void *)(data))

class Sprite;
struct Monitor;
typedef std::list<Monitor*> MonitorList;

int init_module_gui();
void exit_module_gui();

int guiscale();

int get_screen_scaling();
void set_screen_scaling(int scaling);

void update_screen_for_sprite(const Sprite* sprite);

void gui_run();
void gui_feedback();
void gui_setup_screen();

void reload_default_font();

void load_window_pos(JWidget window, const char *section);
void save_window_pos(JWidget window, const char *section);

JWidget load_widget(const char *filename, const char *name);
JWidget find_widget(JWidget widget, const char *name);

void schedule_rebuild_recent_list();

void hook_signal(JWidget widget,
		 int signal_num,
		 bool (*signal_handler)(JWidget widget, void *data),
		 void *data);

void get_widgets(JWidget window, ...);

void add_gfxicon_to_button(JWidget button, int gfx_id, int icon_align);
void set_gfxicon_in_button(JWidget button, int gfx_id);

JWidget radio_button_new(int radio_group, int b1, int b2, int b3, int b4);
JWidget check_button_new(const char *text, int b1, int b2, int b3, int b4);
/* void change_to_button_style(JWidget widget, int b1, int b2, int b3, int b4); */

//////////////////////////////////////////////////////////////////////
// Keyboard shortcuts

JAccel add_keyboard_shortcut_to_execute_command(const char* shortcut, Command* command, const char* argument);
JAccel add_keyboard_shortcut_to_change_tool(const char* shortcut, Tool* tool);

Command* get_command_from_key_message(JMessage msg);
JAccel get_accel_to_execute_command(Command* command, const char* argument);
JAccel get_accel_to_change_tool(Tool* tool);

//////////////////////////////////////////////////////////////////////
// Monitors

Monitor* add_gui_monitor(void (*proc)(void*),
			 void (*free)(void*), void* data);
void remove_gui_monitor(Monitor* monitor);
void* get_monitor_data(Monitor* monitor);

//////////////////////////////////////////////////////////////////////
// Smart JWidget pointer

class JWidgetPtr
{
  JWidget m_widget;

  // TODO make this class copyable and count references (so this is
  //      really "smart" pointer)...
  JWidgetPtr(const JWidgetPtr&);
  JWidgetPtr& operator=(const JWidgetPtr&);

public:
  JWidgetPtr() {
    m_widget = NULL;
  }

  explicit JWidgetPtr(JWidget widget) {
    m_widget = widget;
  }

  ~JWidgetPtr() {
    delete m_widget;
  }

  JWidgetPtr& operator=(JWidget widget) {
    if (m_widget)
      delete m_widget;

    m_widget = widget;
    return *this;
  }

  operator JWidget() {
    return m_widget;
  }

  JWidget operator->() {
    assert(m_widget != NULL);
    return m_widget;
  }

};

#endif
