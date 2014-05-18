/* Aseprite
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

#ifndef APP_FIND_WIDGET_H_INCLUDED
#define APP_FIND_WIDGET_H_INCLUDED
#pragma once

#include "app/widget_not_found.h"
#include "ui/widget.h"

namespace app {

  template<class T>
  inline T* find_widget(ui::Widget* parent, const char* childId) {
    T* child = parent->findChildT<T>(childId);
    if (!child)
      throw WidgetNotFound(childId);

    return child;
  }

  class finder {
  public:
    finder(ui::Widget* parent) : m_parent(parent) {
    }

    finder& operator>>(const char* id) {
      m_lastId = id;
      return *this;
    }

    finder& operator>>(const std::string& id) {
      m_lastId = id;
      return *this;
    }

    template<typename T>
    finder& operator>>(T*& child) {
      child = app::find_widget<T>(m_parent, m_lastId.c_str());
      return *this;
    }

  private:
    ui::Widget* m_parent;
    std::string m_lastId;
  };

} // namespace app

#endif
