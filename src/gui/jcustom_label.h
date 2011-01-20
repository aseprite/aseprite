// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JCUSTOM_LABEL_H_INCLUDED
#define GUI_JCUSTOM_LABEL_H_INCLUDED

#include "gui/label.h"

class CustomLabel : public Label
{
public:
  CustomLabel(const char *text);

protected:
  bool onProcessMessage(JMessage msg);

};

#endif
