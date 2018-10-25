// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc.h"
#include "app/ini_file.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "doc/sprite.h"
#include "os/system.h"

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

  // Hide the menu bar depending on:
  // 1. the native menu bar is available
  // 2. this is the first run of the program
  if (os::instance() &&
      os::instance()->menus() &&
      updater.uuid().empty()) {
    general.showMenuBar(false);
  }
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

bool Preferences::isSet(OptionBase& opt) const
{
  return (get_config_string(opt.section(), opt.id(), nullptr) != nullptr);
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

DocumentPreferences& Preferences::document(const Doc* doc)
{
  auto it = m_docs.find(doc);
  if (it != m_docs.end()) {
    return *it->second;
  }
  else {
    DocumentPreferences* docPref;
    if (doc) {
      docPref = new DocumentPreferences("");

      // The default preferences for this document are the current
      // defaults for (document=nullptr).
      DocumentPreferences& defPref = this->document(nullptr);
      *docPref = defPref;

      // Default values for symmetry
      docPref->symmetry.xAxis.setDefaultValue(doc->sprite()->width()/2);
      docPref->symmetry.yAxis.setDefaultValue(doc->sprite()->height()/2);
    }
    else
      docPref = new DocumentPreferences("");

    m_docs[doc] = docPref;

    // Load document preferences
    serializeDocPref(doc, docPref, false);

    return *docPref;
  }
}

void Preferences::removeDocument(Doc* doc)
{
  ASSERT(doc);

  auto it = m_docs.find(doc);
  if (it != m_docs.end()) {
    serializeDocPref(it->first, it->second, true);
    delete it->second;
    m_docs.erase(it);
  }
}

void Preferences::onRemoveDocument(Doc* doc)
{
  removeDocument(doc);
}

std::string Preferences::docConfigFileName(const Doc* doc)
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

void Preferences::serializeDocPref(const Doc* doc, app::DocumentPreferences* docPref, bool save)
{
  bool flush_config = false;

  if (doc) {
    // We do nothing if the document isn't associated to a file and we
    // want to save its specific preferences.
    if (save && !doc->isAssociatedToFile())
      return;

    // We always push a new configuration file in the stack to avoid
    // modifying the default preferences when a document in "doc" is
    // specified.
    push_config_state();
    if (doc->isAssociatedToFile()) {
      set_config_file(docConfigFileName(doc).c_str());
      flush_config = true;
    }
  }

  if (save) {
    docPref->save();
  }
  else {
    // Load default preferences, or preferences from .ini file.
    docPref->load();
  }

  if (doc) {
    if (flush_config)
      flush_config_file();

    pop_config_state();
  }
}

} // namespace app
