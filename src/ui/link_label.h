// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LINK_LABEL_H_INCLUDED
#define UI_LINK_LABEL_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "ui/custom_label.h"

#include <string>

namespace ui {

  class LinkLabel : public CustomLabel
  {
  public:
    LinkLabel(const base::string& urlOrText);
    LinkLabel(const base::string& url, const base::string& text);

    const base::string& getUrl() const { return m_url; }
    void setUrl(const base::string& url);

    Signal0<void> Click;

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

    base::string m_url;
  };

} // namespace ui

#endif
