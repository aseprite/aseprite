// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LINK_LABEL_H_INCLUDED
#define UI_LINK_LABEL_H_INCLUDED
#pragma once

#include "obs/signal.h"
#include "ui/label.h"

#include <string>

namespace ui {

class LinkLabel : public Label {
public:
  LinkLabel(const std::string& urlOrText = "");
  LinkLabel(const std::string& url, const std::string& text);

  const std::string& url() const { return m_url; }
  void setUrl(const std::string& url);

  obs::signal<void()> Click;

protected:
  bool onProcessMessage(Message* msg) override;
  virtual void onClick();

  std::string m_url;
};

} // namespace ui

#endif
