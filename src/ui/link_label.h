// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_LINK_LABEL_H_INCLUDED
#define UI_LINK_LABEL_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "ui/custom_label.h"

#include <string>

namespace ui {

  class LinkLabel : public CustomLabel
  {
  public:
    LinkLabel(const char* urlOrText);
    LinkLabel(const char* url, const char* text);

    const char* getUrl() const { return m_url.c_str(); }
    void setUrl(const char* url);

    Signal0<void> Click;

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

    std::string m_url;
  };

} // namespace ui

#endif
