// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/palette_popup.h"

#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/launcher.h"
#include "app/match_words.h"
#include "app/res/palettes_loader_delegate.h"
#include "app/res/resource.h"
#include "app/ui/palettes_listbox.h"
#include "app/ui/search_entry.h"
#include "app/ui_context.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/fit_bounds.h"
#include "ui/keys.h"
#include "ui/message.h"
#include "ui/scale.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "palette_popup.xml.h"

namespace app {

using namespace ui;

PalettePopup::PalettePopup()
  : PopupWindow("Palettes", ClickBehavior::CloseOnClickInOtherWindow)
  , m_popup(new gen::PalettePopup())
{
  setAutoRemap(false);
  setEnterBehavior(EnterBehavior::DoNothingOnEnter);

  addChild(m_popup);

  m_paletteListBox.DoubleClickItem.connect([this]{ onLoadPal(); });
  m_paletteListBox.FinishLoading.connect([this]{ onSearchChange(); });
  m_popup->search()->Change.connect([this]{ onSearchChange(); });
  m_popup->refresh()->Click.connect([this]{ onRefresh(); });
  m_popup->loadPal()->Click.connect([this]{ onLoadPal(); });
  m_popup->openFolder()->Click.connect([this]{ onOpenFolder(); });

  m_popup->view()->attachToView(&m_paletteListBox);

  m_paletteListBox.PalChange.connect(&PalettePopup::onPalChange, this);

  InitTheme.connect(
    [this]{
      setBorder(gfx::Border(4*guiscale()));
    });
  initTheme();
}

void PalettePopup::showPopup(ui::Display* display,
                             const gfx::Rect& buttonPos)
{
  m_popup->loadPal()->setEnabled(false);
  m_popup->openFolder()->setEnabled(false);
  m_paletteListBox.selectChild(NULL);

  fit_bounds(display, this,
             gfx::Rect(buttonPos.x, buttonPos.y2(), 32, 32),
             [](const gfx::Rect& workarea,
                gfx::Rect& bounds,
                std::function<gfx::Rect(Widget*)> getWidgetBounds) {
               bounds.w = workarea.w/2;
               bounds.h = workarea.h*3/4;
             });

  openWindowInForeground();
}

bool PalettePopup::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kKeyDownMessage: {
      KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
      KeyScancode scancode = keyMsg->scancode();
      bool refresh = (scancode == kKeyF5 ||
                      (msg->ctrlPressed() && scancode == kKeyR) ||
                      (msg->cmdPressed() && scancode == kKeyR));
      if (refresh) {
        onRefresh();
        return true;
      }
      break;
    }
  }
  return ui::PopupWindow::onProcessMessage(msg);
}

void PalettePopup::onPalChange(const doc::Palette* palette)
{
  const bool state =
    (UIContext::instance()->activeDocument() &&
     palette != nullptr);

  m_popup->loadPal()->setEnabled(state);
  m_popup->openFolder()->setEnabled(state);
}

void PalettePopup::onSearchChange()
{
  MatchWords match(m_popup->search()->text());
  bool selected = false;

  for (auto child : m_paletteListBox.children()) {
    if (dynamic_cast<ResourceListItem*>(child)) {
      const bool vis = match(child->text());
      child->setVisible(vis);
      if (!selected && vis) {
        selected = true;
        m_paletteListBox.selectChild(child);
      }
    }
    else
      child->setVisible(true);
  }

  if (!selected)
    m_paletteListBox.selectChild(nullptr);

  m_popup->view()->layout();
}

void PalettePopup::onRefresh()
{
  m_paletteListBox.reload();
}

void PalettePopup::onLoadPal()
{
  const doc::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    Commands::instance()->byId(CommandId::SetPalette()));
  cmd->setPalette(palette);
  UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);

  m_paletteListBox.requestFocus();
  m_paletteListBox.invalidate();
}

void PalettePopup::onOpenFolder()
{
  Resource* res = m_paletteListBox.selectedResource();
  if (!res)
    return;

  launcher::open_folder(res->path());
}

} // namespace app
