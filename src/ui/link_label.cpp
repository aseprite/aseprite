// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/link_label.h"

#include "base/launcher.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

LinkLabel::LinkLabel(const std::string& urlOrText)
  : CustomLabel(urlOrText)
  , m_url(urlOrText)
{
}

LinkLabel::LinkLabel(const std::string& url, const std::string& text)
  : CustomLabel(text)
  , m_url(url)
{
}

void LinkLabel::setUrl(const std::string& url)
{
  m_url = url;
}

bool LinkLabel::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kSetCursorMessage:
      // TODO theme stuff
      if (isEnabled() && hasMouseOver()) {
        set_mouse_cursor(kHandCursor);
        return true;
      }
      break;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      if (isEnabled()) {
        if (hasCapture())
          setSelected(msg->type() == kMouseEnterMessage);

        invalidate();           // TODO theme specific
      }
      break;

    case kMouseMoveMessage:
      if (isEnabled() && hasCapture())
        setSelected(hasMouseOver());
      break;

    case kMouseDownMessage:
      if (isEnabled()) {
        captureMouse();
        setSelected(true);
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        setSelected(false);
        invalidate();           // TODO theme specific

        if (hasMouseOver())
          onClick();
      }
      break;
  }

  return CustomLabel::onProcessMessage(msg);
}

void LinkLabel::onPaint(PaintEvent& ev)
{
  theme()->paintLinkLabel(ev);
}

void LinkLabel::onClick()
{
  if (!m_url.empty())
    base::launcher::open_url(m_url);

  Click();
}

} // namespace ui
