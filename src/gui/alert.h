// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_ALERT_H_INCLUDED
#define GUI_ALERT_H_INCLUDED

#include "base/shared_ptr.h"
#include "gui/frame.h"

class Alert;
typedef SharedPtr<Alert> AlertPtr;

class Alert : public Frame
{
public:
  Alert();

  static AlertPtr create(const char* format, ...);
  static int show(const char* format, ...);

private:
  void processString(char* buf, std::vector<Widget*>& labels, std::vector<Widget*>& buttons);
};

#endif

