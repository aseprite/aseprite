// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.


#include "config.h"

#include "launcher.h"
#include "ui/link_label.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

LinkLabel::LinkLabel(const char* urlOrText)
  : CustomLabel(urlOrText)
  , m_url(urlOrText)
{
}

LinkLabel::LinkLabel(const char* url, const char* text)
  : CustomLabel(text)
  , m_url(url)
{
}

void LinkLabel::setUrl(const char* url)
{
  m_url = url;
}

bool LinkLabel::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETCURSOR:
      // TODO theme stuff
      if (isEnabled()) {
        jmouse_set_cursor(kHandCursor);
        return true;
      }
      break;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      // TODO theme stuff
      if (isEnabled())
        invalidate();
      break;

    case JM_BUTTONRELEASED:
      if (isEnabled()) {
        if (!m_url.empty())
          Launcher::openUrl(m_url);
        Click();
      }
      break;
  }

  return CustomLabel::onProcessMessage(msg);
}

void LinkLabel::onPaint(PaintEvent& ev)
{
  getTheme()->paintLinkLabel(ev);
}

} // namespace ui
