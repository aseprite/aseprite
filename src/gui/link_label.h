// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_LINK_LABEL_H_INCLUDED
#define GUI_LINK_LABEL_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "gui/custom_label.h"

#include <string>

class LinkLabel : public CustomLabel
{
  std::string m_url;

public:
  LinkLabel(const char* urlOrText);
  LinkLabel(const char* url, const char* text);

  Signal0<void> Click;

protected:
  bool onProcessMessage(JMessage msg) OVERRIDE;
  void onPaint(PaintEvent& ev) OVERRIDE;

};

#endif
