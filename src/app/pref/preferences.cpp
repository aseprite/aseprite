// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
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
#include "base/fs.h"
#include "doc/sprite.h"
#include "os/system.h"
#include "ui/system.h"

namespace app {

static Preferences* singleton = nullptr;

// static
Preferences& Preferences::instance()
{
#ifdef _DEBUG
  // Preferences can be used only from the main UI thread. In other
  // case access to std::map<> could crash the program.
  if (ui::UISystem::instance())
    ui::assert_ui_thread();
#endif

  ASSERT(singleton);
  return *singleton;
}

Preferences::Preferences()
  : app::gen::GlobalPref("")
{
  ASSERT(!singleton);
  singleton = this;

  // Main configuration file
  const std::string fn = main_config_filename();
  ASSERT(!fn.empty());

  // The first time we execute the program, the configuration file
  // doesn't exist.
  const bool firstTime = (!base::is_file(fn));

  load();

  // Create a connection with the default document preferences grid
  // bounds to sync the default grid bounds for new sprites in the
  // "doc" layer.
  auto& defPref = document(nullptr);
  defPref.grid.bounds.AfterChange.connect(
    [](const gfx::Rect& newValue){
      doc::Sprite::SetDefaultGridBounds(newValue);
    });
  doc::Sprite::SetDefaultGridBounds(defPref.grid.bounds());

  // Reset confusing defaults for a new instance of the program.
  defPref.grid.snap(false);
  if (selection.mode() != gen::SelectionMode::DEFAULT &&
      selection.mode() != gen::SelectionMode::ADD) {
    selection.mode(gen::SelectionMode::DEFAULT);
  }

  // Hide the menu bar depending on:
  // 1. this is the first run of the program
  // 2. the native menu bar is available
  if (firstTime &&
      os::instance() &&
      os::instance()->menus()) {
    general.showMenuBar(false);
  }
}

Preferences::~Preferences()
{
  // Don't save preferences, this must be done by the client of the
  // Preferences instance (the App class).
  //save();

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
  ui::assert_ui_thread();
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
    // Create a new DocumentPreferences for the given "doc" pointer
    DocumentPreferences* docPref = new DocumentPreferences("");
    m_docs[doc] = docPref;

    // Setup the preferences of this document with the default ones
    // (these preferences will be overwritten in the next statement
    // loading the preferences from the .ini file of this doc)
    if (doc) {
      // The default preferences for this document are the current
      // defaults for (document=nullptr).
      DocumentPreferences& defPref = this->document(nullptr);
      *docPref = defPref;

      // Default values for symmetry
      docPref->symmetry.xAxis.setValueAndDefault(doc->sprite()->width()/2);
      docPref->symmetry.yAxis.setValueAndDefault(doc->sprite()->height()/2);
    }

    // Load specific settings of this document
    serializeDocPref(doc, docPref, false);

    // Turn off snap to grid setting (it's confusing for most of the
    // users to load this setting).
    docPref->grid.snap.setValueAndDefault(false);

    return *docPref;
  }
}

void Preferences::resetToolPreferences(tools::Tool* tool)
{
  auto it = m_tools.find(tool->getId());
  if (it != m_tools.end())
    m_tools.erase(it);

  std::string section = std::string("tool.") + tool->getId();
  del_config_section(section.c_str());

  // TODO improve this, if we add new sections in pref.xml we have to
  //      update this manually :(
  del_config_section((section + ".brush").c_str());
  del_config_section((section + ".spray").c_str());
  del_config_section((section + ".floodfill").c_str());
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
    if (save && flush_config)
      flush_config_file();

    pop_config_state();
  }
}

} // namespace app
