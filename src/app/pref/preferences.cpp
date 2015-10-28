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
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "doc/sprite.h"

namespace app {

static Preferences* singleton = nullptr;

// static
Preferences& Preferences::instance()
{
  ASSERT(singleton);
  return *singleton;
}

Preferences::Preferences()
  : app::gen::GlobalPref("")
{
  ASSERT(!singleton);
  singleton = this;

  load();
}

Preferences::~Preferences()
{
  save();

  for (auto& pair : m_tools)
    delete pair.second;

  for (auto& pair : m_docs)
    delete pair.second;

  ASSERT(singleton == this);
  singleton = nullptr;
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
    serializeDocPref(pair.first, pair.second, true);

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
    std::string section = std::string("tool.") + tool->getId();
    ToolPreferences* toolPref = new ToolPreferences(section);

    // Default size for eraser, blur, etc.
    if (tool->getInk(0)->isEraser() ||
        tool->getInk(0)->isEffect()) {
      toolPref->brush.size.setDefaultValue(8);
    }

    m_tools[tool->getId()] = toolPref;
    toolPref->load();
    return *toolPref;
  }
}

DocumentPreferences& Preferences::document(const app::Document* doc)
{
  auto it = m_docs.find(doc);
  if (it != m_docs.end()) {
    return *it->second;
  }
  else {
    DocumentPreferences* docPref;
    if (doc) {
      docPref = new DocumentPreferences("");
      *docPref = this->document(nullptr);

      // Default values for symmetry
      docPref->symmetry.xAxis.setDefaultValue(doc->sprite()->width()/2);
      docPref->symmetry.yAxis.setDefaultValue(doc->sprite()->height()/2);
    }
    else
      docPref = new DocumentPreferences("");

    m_docs[doc] = docPref;
    serializeDocPref(doc, docPref, false);
    return *docPref;
  }
}

void Preferences::removeDocument(doc::Document* doc)
{
  ASSERT(dynamic_cast<app::Document*>(doc));

  auto it = m_docs.find(static_cast<app::Document*>(doc));
  if (it != m_docs.end()) {
    serializeDocPref(it->first, it->second, true);
    delete it->second;
    m_docs.erase(it);
  }
}

void Preferences::onRemoveDocument(doc::Document* doc)
{
  removeDocument(doc);
}

std::string Preferences::docConfigFileName(const app::Document* doc)
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

void Preferences::serializeDocPref(const app::Document* doc, app::DocumentPreferences* docPref, bool save)
{
  bool specific_file = false;

  if (doc) {
    if (doc->isAssociatedToFile()) {
      push_config_state();
      set_config_file(docConfigFileName(doc).c_str());
      specific_file = true;
    }
    else if (save)
      return;
  }

  if (save)
    docPref->save();
  else {
    // Load default preferences, or preferences from .ini file.
    docPref->load();
  }

  if (specific_file) {
    flush_config_file();
    pop_config_state();
  }
}

} // namespace app
