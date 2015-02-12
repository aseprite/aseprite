// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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

  for (auto& pair : m_docs)
    saveDocPref(pair.first, pair.second);

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

void Preferences::onRemoveDocument(doc::Document* doc)
{
  ASSERT(dynamic_cast<app::Document*>(doc));

  auto it = m_docs.find(static_cast<app::Document*>(doc));
  if (it != m_docs.end()) {
    saveDocPref(it->first, it->second);
    delete it->second;
    m_docs.erase(it);
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

void Preferences::saveDocPref(app::Document* doc, app::DocumentPreferences* docPref)
{
  bool specific_file = false;

  if (doc && doc->isAssociatedToFile()) {
    push_config_state();
    set_config_file(docConfigFileName(doc).c_str());
    specific_file = true;
  }

  docPref->save();

  if (specific_file) {
    flush_config_file();
    pop_config_state();
  }
}

} // namespace app
