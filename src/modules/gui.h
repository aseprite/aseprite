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

#ifndef MODULES_GUI_H_INCLUDED
#define MODULES_GUI_H_INCLUDED

#include <string>
#include <list>
#include "ase_exception.h"
#include "jinete/jbase.h"
#include "jinete/jaccel.h"

class Command;
class Params;
class Tool;
class Widget;
class Frame;

//////////////////////////////////////////////////////////////////////

class widget_file_not_found : public ase_exception
{
public:
  widget_file_not_found(const char* file_name) throw()
    : ase_exception("Cannot load file: %s\nPlease reinstall %s", file_name, PACKAGE) { }
};

/**
 * Exception thrown by find_widget() if a widget is not found.
 */
class widget_not_found : public ase_exception
{
public:
  widget_not_found(const char* widget_name) throw()
    : ase_exception("A data file is corrupted.\nPlease reinstall %s\n\n" 
		    "Details: Widget not found: %s", PACKAGE, widget_name) { }
};

//////////////////////////////////////////////////////////////////////

#define HOOK(widget, signal, signal_handler, data)			\
  hook_signal((widget), (signal), (signal_handler), (void *)(data))

class Sprite;
struct Monitor;
typedef std::list<Monitor*> MonitorList;

int init_module_gui();
void exit_module_gui();

int get_screen_scaling();
void set_screen_scaling(int scaling);

void update_screen_for_sprite(const Sprite* sprite);

void gui_run();
void gui_feedback();
void gui_flip_screen();
void gui_setup_screen(bool reload_font);

void load_window_pos(Widget* window, const char *section);
void save_window_pos(Widget* window, const char *section);

Widget* load_widget(const char *filename, const char *name);
Widget* find_widget(Widget* widget, const char *name);

void schedule_rebuild_recent_list();

void hook_signal(Widget* widget,
		 int signal_num,
		 bool (*signal_handler)(Widget* widget, void *data),
		 void *data);

void get_widgets(Widget* window, ...);

void setup_mini_look(Widget* widget);

void add_gfxicon_to_button(Widget* button, int gfx_id, int icon_align);
void set_gfxicon_in_button(Widget* button, int gfx_id);

Widget* radio_button_new(int radio_group, int b1, int b2, int b3, int b4);
Widget* check_button_new(const char *text, int b1, int b2, int b3, int b4);
/* void change_to_button_style(Widget* widget, int b1, int b2, int b3, int b4); */

//////////////////////////////////////////////////////////////////////
// Keyboard shortcuts

JAccel add_keyboard_shortcut_to_execute_command(const char* shortcut, const char* command_name, Params* params);
JAccel add_keyboard_shortcut_to_change_tool(const char* shortcut, Tool* tool);

Command* get_command_from_key_message(JMessage msg);
JAccel get_accel_to_execute_command(const char* command, Params* params = NULL);
JAccel get_accel_to_change_tool(Tool* tool);

//////////////////////////////////////////////////////////////////////
// Monitors

Monitor* add_gui_monitor(void (*proc)(void*),
			 void (*free)(void*), void* data);
void remove_gui_monitor(Monitor* monitor);
void* get_monitor_data(Monitor* monitor);

//////////////////////////////////////////////////////////////////////
// Smart Widget* pointer

template<typename T>
class ScopedPtr
{
  T* m_ptr;

  // TODO make this class copyable and count references (so this is
  //      really "smart" pointer)...
  ScopedPtr(const ScopedPtr&);
  ScopedPtr& operator=(const ScopedPtr&);

public:
  ScopedPtr() {
    m_ptr = NULL;
  }

  explicit ScopedPtr(T* widget) {
    m_ptr = widget;
  }

  template<typename T2>
  explicit ScopedPtr(T2* widget) {
    m_ptr = static_cast<T*>(widget);
  }

  ~ScopedPtr() {
    delete m_ptr;
  }

  ScopedPtr& operator=(T* widget) {
    if (m_ptr)
      delete m_ptr;

    m_ptr = widget;
    return *this;
  }

  operator T*() {
    return m_ptr;
  }

  T* operator->() {
    ASSERT(m_ptr != NULL);
    return m_ptr;
  }

};

typedef ScopedPtr<Widget> WidgetPtr;
typedef ScopedPtr<Frame> FramePtr;

#endif
