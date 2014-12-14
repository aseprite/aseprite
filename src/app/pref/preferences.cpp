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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document.h"
#include "app/ini_file.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/tools/tool.h"

namespace app {

Preferences::Preferences()
  : app::gen::GlobalPref("")
{
  load();
}

Preferences::~Preferences()
{
  save();

  for (auto& pair : m_tools)
    delete pair.second;

  for (auto& pair : m_docs)
    delete pair.second;
}

void Preferences::load()
{
  app::gen::GlobalPref::load();
}

void Preferences::save()
{
  app::gen::GlobalPref::save();

  for (auto& pair : m_tools)
    pair.second->save();

  for (auto& pair : m_docs) {
    app::Document* doc = pair.first;
    bool specific_file = false;

    if (doc && doc->isAssociatedToFile()) {
      push_config_state();
      set_config_file(docConfigFileName(doc).c_str());
      specific_file = true;
    }

    pair.second->save();

    if (specific_file) {
      flush_config_file();
      pop_config_state();
    }
  }

  flush_config_file();
}

ToolPreferences& Preferences::tool(tools::Tool* tool)
{
  ASSERT(tool != NULL);

  auto it = m_tools.find(tool->getId());
  if (it != m_tools.end()) {
    return *it->second;
  }
  else {
    std::string section = "tool.";
    section += tool->getId();

    ToolPreferences* toolPref = new ToolPreferences(section);
    m_tools[tool->getId()] = toolPref;
    return *toolPref;
  }
}

DocumentPreferences& Preferences::document(app::Document* document)
{
  ASSERT(document != NULL);

  auto it = m_docs.find(document);
  if (it != m_docs.end()) {
    return *it->second;
  }
  else {
    DocumentPreferences* docPref = new DocumentPreferences("");
    m_docs[document] = docPref;
    return *docPref;
  }
}

std::string Preferences::docConfigFileName(app::Document* doc)
{
  if (!doc)
    return "";

  ResourceFinder rf;
  std::string fn = doc->filename();
  for (size_t i=0; i<fn.size(); ++i) {
    if (fn[i] == ' ' || fn[i] == '/' || fn[i] == '\\' || fn[i] == ':' || fn[i] == '.') {
      fn[i] = '-';
    }
  }
  rf.includeUserDir(("files/" + fn + ".ini").c_str());
  return rf.getFirstOrCreateDefault();
}

} // namespace app
