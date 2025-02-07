// Aseprite Code Generator
// Copyright (c) 2021-2024 Igara Studio S.A.
// Copyright (c) 2016-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gen/check_strings.h"

#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/split_string.h"
#include "base/string.h"
#include "cfg/cfg.h"
#include "gen/common.h"

#include "tinyxml2.h"

#include <cctype>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

// Check only the existence of strings from the main "en.ini" file
// All other translations will be considered work-in-progress.
#define ENGLISH_ONLY 1

using namespace tinyxml2;
using XmlElements = std::vector<XMLElement*>;

static std::string find_first_id(XMLElement* elem)
{
  XMLElement* child = elem->FirstChildElement();
  while (child) {
    const char* id = child->Attribute("id");
    if (id)
      return id;

    std::string idStr = find_first_id(child);
    if (!idStr.empty())
      return idStr;

    child = child->NextSiblingElement();
  }
  return "";
}

static void collect_elements_with_strings(XMLElement* elem, XmlElements& elems)
{
  XMLElement* child = elem->FirstChildElement();
  while (child) {
    const char* text = child->Attribute("text");
    const char* tooltip = child->Attribute("tooltip");
    if (text || tooltip)
      elems.push_back(child);
    collect_elements_with_strings(child, elems);
    child = child->NextSiblingElement();
  }
}

static bool has_alpha_char(const char* p)
{
  while (*p) {
    if (std::isalpha(*p))
      return true;
    else
      ++p;
  }
  return false;
}

static bool is_email(const char* p)
{
  if (!*p || !std::isalpha(*p))
    return false;
  ++p;

  while (*p && (std::isalpha(*p) || *p == '.'))
    ++p;

  if (*p != '@')
    return false;
  ++p;

  while (*p && (std::isalpha(*p) || *p == '.'))
    ++p;

  // Return true if we are in the end of string
  return (*p == 0);
}

class CheckStrings {
public:
  void loadStrings(const std::string& dir)
  {
#if ENGLISH_ONLY
    std::string fn = "en.ini";
#else
    for (const auto& fn : base::list_files(dir))
#endif
    {
      std::unique_ptr<cfg::CfgFile> f(new cfg::CfgFile);
      f->load(base::join_path(dir, fn));
      m_stringFiles.push_back(std::move(f));
    }
  }

  void checkStringsOnWidgets(const std::string& dir)
  {
    for (const auto& fn : base::list_files(dir)) {
      std::string fullFn = base::join_path(dir, fn);
      base::FileHandle inputFile(base::open_file(fullFn, "rb"));
      auto doc = std::make_unique<XMLDocument>();
      if (doc->LoadFile(inputFile.get()) != XML_SUCCESS) {
        std::cerr << fullFn << ":" << doc->ErrorLineNum() << ": "
                  << "error " << int(doc->ErrorID()) << ": " << doc->ErrorStr() << "\n";

        throw std::runtime_error("invalid input file");
      }

      XMLHandle handle(doc.get());
      XmlElements widgets;

      const char* warnings = doc->RootElement()->Attribute("i18nwarnings");
      if (warnings && strcmp(warnings, "false") == 0)
        continue;

      m_prefixId = find_first_id(doc->RootElement());

      collect_elements_with_strings(doc->RootElement(), widgets);
      for (XMLElement* elem : widgets) {
        checkString(fullFn, elem, elem->Attribute("text"));
        checkString(fullFn, elem, elem->Attribute("tooltip"));
      }
    }
  }

  void checkStringsOnGuiFile(const std::string& fullFn)
  {
    base::FileHandle inputFile(base::open_file(fullFn, "rb"));
    auto doc = std::make_unique<XMLDocument>();
    if (doc->LoadFile(inputFile.get()) != XML_SUCCESS) {
      std::cerr << fullFn << ":" << doc->ErrorLineNum() << ": "
                << "error " << int(doc->ErrorID()) << ": " << doc->ErrorStr() << "\n";

      throw std::runtime_error("invalid input file");
    }

    XMLHandle handle(doc.get());

    // For each menu
    XMLElement* xmlMenu = handle.FirstChildElement("gui")
                            .FirstChildElement("menus")
                            .FirstChildElement("menu")
                            .ToElement();
    while (xmlMenu) {
      const char* menuId = xmlMenu->Attribute("id");
      if (menuId) {
        m_prefixId = menuId;
        XmlElements menus;
        collect_elements_with_strings(xmlMenu, menus);
        for (XMLElement* elem : menus)
          checkString(fullFn, elem, elem->Attribute("text"));
      }
      xmlMenu = xmlMenu->NextSiblingElement();
    }

    // For each tool
    m_prefixId = "tools";
    XMLElement* xmlGroup = handle.FirstChildElement("gui")
                             .FirstChildElement("tools")
                             .FirstChildElement("group")
                             .ToElement();
    while (xmlGroup) {
      XmlElements tools;
      collect_elements_with_strings(xmlGroup, tools);
      for (XMLElement* elem : tools) {
        checkString(fullFn, elem, elem->Attribute("text"));
        checkString(fullFn, elem, elem->Attribute("tooltip"));
      }
      xmlGroup = xmlGroup->NextSiblingElement();
    }
  }

  void checkString(const std::string& filename, XMLElement* elem, const char* text)
  {
    if (!text)
      return; // Do nothing
    else if (text[0] == '@') {
      for (auto& cfg : m_stringFiles) {
        std::string lang = base::get_file_title(cfg->filename());
        std::string section, var;

        if (text[1] == '.') {
          section = m_prefixId.c_str();
          var = text + 2;
        }
        else {
          std::vector<std::string> parts;
          base::split_string(text, parts, ".");
          if (parts.size() >= 1)
            section = parts[0].c_str() + 1;
          if (parts.size() >= 2)
            var = parts[1];
        }

        const char* translated = cfg->getValue(section.c_str(), var.c_str(), nullptr);
        if (!translated || translated[0] == 0) {
          std::cerr << filename << ":" << elem->GetLineNum() << ": "
                    << "warning: <" << lang << "> translation for a string ID wasn't found '"
                    << text << "' (" << section << "." << var << ")\n";
        }
      }
    }
    else if (text[0] != '!' && has_alpha_char(text) && !is_email(text)) {
      std::cerr << filename << ":" << elem->GetLineNum() << ": "
                << "warning: raw string found '" << text << "'\n";
    }
  }

private:
  std::vector<std::unique_ptr<cfg::CfgFile>> m_stringFiles;
  std::string m_prefixId;
};

void check_strings(const std::string& widgetsDir,
                   const std::string& stringsDir,
                   const std::string& guiFile)
{
  CheckStrings cs;
  cs.loadStrings(stringsDir);
  cs.checkStringsOnWidgets(widgetsDir);
  cs.checkStringsOnGuiFile(guiFile);
}
