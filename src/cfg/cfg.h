// Aseprite Config Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2014-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CFG_CFG_H_INCLUDED
#define CFG_CFG_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace cfg {

  class CfgFile {
  public:
    CfgFile();
    ~CfgFile();

    const std::string& filename() const;

    void getAllSections(std::vector<std::string>& sections) const;
    void getAllKeys(const char* section, std::vector<std::string>& keys) const;

    const char* getValue(const char* section, const char* name, const char* defaultValue) const;
    bool getBoolValue(const char* section, const char* name, bool defaultValue);
    int getIntValue(const char* section, const char* name, int defaultValue);
    double getDoubleValue(const char* section, const char* name, double defaultValue);

    void setValue(const char* section, const char* name, const char* value);
    void setBoolValue(const char* section, const char* name, bool value);
    void setIntValue(const char* section, const char* name, int value);
    void setDoubleValue(const char* section, const char* name, double value);

    void deleteValue(const char* section, const char* name);
    void deleteSection(const char* section);

    void load(const std::string& filename);
    void save();

  private:
    class CfgFileImpl;
    CfgFileImpl* m_impl;
  };

} // namespace cfg

#endif
