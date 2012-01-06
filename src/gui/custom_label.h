// ASE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_CUSTOM_LABEL_H_INCLUDED
#define GUI_CUSTOM_LABEL_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/label.h"

class CustomLabel : public Label
{
public:
  CustomLabel(const char *text);

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;

};

#endif
