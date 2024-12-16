// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/expr_entry.h"

#include "ui/message.h"

#include "fmt/format.h"
#include "tinyexpr.h"

#include <cmath>
#include <cstdio>

namespace app {

ExprEntry::ExprEntry() : ui::Entry(1024, ""), m_decimals(0)
{
}

bool ExprEntry::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case ui::kFocusLeaveMessage: {
      std::string buf;
      onFormatExprFocusLeave(buf);
      if (text() != buf)
        setText(buf);

      Leave();
      break;
    }
  }
  return ui::Entry::onProcessMessage(msg);
}

void ExprEntry::onChange()
{
  Entry::onChange();
  // TODO show expression errors?
}

int ExprEntry::onGetTextInt() const
{
  int err = 0;
  double v = te_interp(text().c_str(), &err);
  if (std::isnan(v))
    return Entry::onGetTextInt();
  else
    return int(v);
}

double ExprEntry::onGetTextDouble() const
{
  int err = 0;
  double v = te_interp(text().c_str(), &err);
  if (std::isnan(v))
    return Entry::onGetTextDouble();
  else
    return v;
}

void ExprEntry::onFormatExprFocusLeave(std::string& buf)
{
  if (m_decimals == 0)
    buf = fmt::format("{}", onGetTextInt());
  else
    buf = fmt::format("{:.{}f}", onGetTextDouble(), m_decimals);
}

} // namespace app
