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

#ifndef APP_PREF_PREFERENCES_H_INCLUDED
#define APP_PREF_PREFERENCES_H_INCLUDED
#pragma once

#include "app/pref/option.h"

#include "generated_pref_types.h"

#include <map>
#include <string>
#include <vector>

namespace app {
  namespace tools {
    class Tool;
  }

  class Document;

  typedef app::gen::ToolPref ToolPreferences;
  typedef app::gen::DocPref DocumentPreferences;

  class Preferences : public app::gen::GlobalPref {
  public:
    Preferences();
    ~Preferences();

    void load();
    void save();

    ToolPreferences& tool(tools::Tool* tool);
    DocumentPreferences& document(app::Document* document);

  private:
    std::string docConfigFileName(app::Document* doc);

    std::map<std::string, app::ToolPreferences*> m_tools;
    std::map<app::Document*, app::DocumentPreferences*> m_docs;
  };

} // namespace app

#endif
