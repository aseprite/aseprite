// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EXPR_ENTRY_H_INCLUDED
#define APP_UI_EXPR_ENTRY_H_INCLUDED
#pragma once

#include "ui/entry.h"

namespace app {

  // Support math expressions.
  class ExprEntry : public ui::Entry {
  public:
    ExprEntry();

    int decimals() const { return m_decimals; }
    void setDecimals(int decimals) { m_decimals = decimals; }

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onChange() override;
    int onGetTextInt() const override;
    double onGetTextDouble() const override;

    int m_decimals;
  };

} // namespace app

#endif
