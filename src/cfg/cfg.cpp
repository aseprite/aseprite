// Aseprite Config Library
// Copyright (c) 2014, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cfg/cfg.h"

#include "base/file_handle.h"
#include "base/string.h"

#include <stdlib.h>
#include "SimpleIni.h"

namespace cfg {

class CfgFile::CfgFileImpl {
public:
  const std::string& filename() const {
    return m_filename;
  }

  const char* getValue(const char* section, const char* name, const char* defaultValue) const {
    return m_ini.GetValue(section, name, defaultValue);
  }

  bool getBoolValue(const char* section, const char* name, bool defaultValue) const {
    return m_ini.GetBoolValue(section, name, defaultValue);
  }

  int getIntValue(const char* section, const char* name, int defaultValue) const {
    return m_ini.GetLongValue(section, name, defaultValue);
  }

  double getDoubleValue(const char* section, const char* name, double defaultValue) const {
    return m_ini.GetDoubleValue(section, name, defaultValue);
  }

  void setValue(const char* section, const char* name, const char* value) {
    m_ini.SetValue(section, name, value);
  }

  void setBoolValue(const char* section, const char* name, bool value) {
    m_ini.SetBoolValue(section, name, value);
  }

  void setIntValue(const char* section, const char* name, int value) {
    m_ini.SetLongValue(section, name, value);
  }

  void setDoubleValue(const char* section, const char* name, double value) {
    m_ini.SetDoubleValue(section, name, value);
  }

  void deleteValue(const char* section, const char* name) {
    m_ini.Delete(section, name, true);
  }

  void load(const std::string& filename) {
    m_filename = filename;

    base::FileHandle file(base::open_file(m_filename, "rb"));
    if (file) {
      SI_Error err = m_ini.LoadFile(file.get());
      if (err != SI_OK)
        LOG("Error '%d' loading configuration from '%s'.", err, m_filename.c_str());
    }
  }

  void save() {
    base::FileHandle file(base::open_file(m_filename, "wb"));
    if (file) {
      SI_Error err = m_ini.SaveFile(file.get());
      if (err != SI_OK)
        LOG("Error '%d' saving configuration into '%s'.", err, m_filename.c_str());
    }
  }

private:
  std::string m_filename;
  CSimpleIniA m_ini;
};

CfgFile::CfgFile()
  : m_impl(new CfgFileImpl)
{
}

CfgFile::~CfgFile()
{
  delete m_impl;
}

const std::string& CfgFile::filename() const
{
  return m_impl->filename();
}

const char* CfgFile::getValue(const char* section, const char* name, const char* defaultValue) const
{
  return m_impl->getValue(section, name, defaultValue);
}

bool CfgFile::getBoolValue(const char* section, const char* name, bool defaultValue)
{
  return m_impl->getBoolValue(section, name, defaultValue);
}

int CfgFile::getIntValue(const char* section, const char* name, int defaultValue)
{
  return m_impl->getIntValue(section, name, defaultValue);
}

double CfgFile::getDoubleValue(const char* section, const char* name, double defaultValue)
{
  return m_impl->getDoubleValue(section, name, defaultValue);
}

void CfgFile::setValue(const char* section, const char* name, const char* value)
{
  m_impl->setValue(section, name, value);
}

void CfgFile::setBoolValue(const char* section, const char* name, bool value)
{
  m_impl->setBoolValue(section, name, value);
}

void CfgFile::setIntValue(const char* section, const char* name, int value)
{
  m_impl->setIntValue(section, name, value);
}

void CfgFile::setDoubleValue(const char* section, const char* name, double value)
{
  m_impl->setDoubleValue(section, name, value);
}

void CfgFile::deleteValue(const char* section, const char* name)
{
  m_impl->deleteValue(section, name);
}

void CfgFile::load(const std::string& filename)
{
  m_impl->load(filename);
}

void CfgFile::save()
{
  m_impl->save();
}

} // namespace cfg
