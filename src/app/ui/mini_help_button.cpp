// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/mini_help_button.h"

#include "app/ui/skin/skin_theme.h"
#include "base/launcher.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ver/info.h"

namespace app {

using namespace app::skin;
using namespace ui;

MiniHelpButton::MiniHelpButton(const std::string& link) : Button(std::string()), m_link(link)
{
  setDecorative(true);
  initTheme();
}

void MiniHelpButton::onInitTheme(InitThemeEvent& ev)
{
  Button::onInitTheme(ev);

  auto* theme = SkinTheme::get(this);
  setStyle(theme->styles.windowHelpButton());
}

void MiniHelpButton::onClick()
{
  Button::onClick();

  std::string url;
  if (m_link.find("http") != std::string::npos) {
    url = m_link;
  }
  else {
    url = get_app_url();
    url += "docs/";
    url += m_link;
  }

  base::launcher::open_url(url);
}

void MiniHelpButton::onSetDecorativeWidgetBounds()
{
  auto* theme = SkinTheme::get(this);
  Widget* window = parent();
  gfx::Rect rect(0, 0, 0, 0);
  const gfx::Size thisSize = this->sizeHint();
  const gfx::Size closeSize = theme->calcSizeHint(this, theme->styles.windowCloseButton());
  const gfx::Border margin(0, 0, 0, 0);

  rect.w = thisSize.w;
  rect.h = thisSize.h;
  rect.offset(window->bounds().x2() - theme->styles.windowCloseButton()->margin().width() -
                closeSize.w - style()->margin().right() - thisSize.w,
              window->bounds().y + style()->margin().top());

  setBounds(rect);
}

} // namespace app
