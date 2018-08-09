// LAF OS Library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_COMMON_FILE_DIALOG_H
#define OS_COMMON_FILE_DIALOG_H
#pragma once

#include "os/native_dialogs.h"

namespace os {

class CommonFileDialog : public FileDialog {
public:
  CommonFileDialog()
    : m_type(Type::OpenFile) {
  }

  void dispose() override {
    delete this;
  }

  void setType(const Type type) override {
    m_type = type;
  }

  void setTitle(const std::string& title) override {
    m_title = title;
  }

  void setDefaultExtension(const std::string& extension) override {
    m_defExtension = extension;
  }

  void addFilter(const std::string& extension, const std::string& description) override {
    if (m_defExtension.empty())
      m_defExtension = extension;

    m_filters.push_back(std::make_pair(extension, description));
  }

protected:
  Type m_type;
  std::string m_title;
  std::string m_defExtension;
  std::vector<std::pair<std::string, std::string>> m_filters;
};

} // namespace os

#endif
