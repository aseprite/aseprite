/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/file_selector.h"
#include "app/modules/gui.h"
#include "app/resource_finder.h"
#include "app/tools/tool.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/select_accelerator.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/resize_event.h"

#include "generated_keyboard_shortcuts.h"

#define KEYBOARD_FILENAME_EXTENSION "aseprite-keys"

namespace app {

using namespace ui;
using namespace skin;

static int g_sep = 0;

class KeyItem : public ListItem {
public:
  KeyItem(const std::string& text, Key* key, AppMenuItem* menuitem, int level)
    : ListItem(text)
    , m_key(key)
    , m_keyOrig(key ? new Key(*key): NULL)
    , m_menuitem(menuitem)
    , m_level(level)
    , m_changeButton(NULL)
    , m_deleteButton(NULL)
    , m_addButton(NULL)
    , m_hotAccel(-1) {
    this->border_width.t = this->border_width.b = 0;
  }

  ~KeyItem() {
    destroyButtons();
  }

  void restoreKeys() {
    if (m_key && m_keyOrig)
      *m_key = *m_keyOrig;

    if (m_menuitem && !m_keyOrig)
      m_menuitem->setKey(NULL);
  }

private:

  void onChangeAccel(int index) {
    Accelerator origAccel = m_key->accels()[index];
    SelectAccelerator window(origAccel, m_key->keycontext());
    window.openWindowInForeground();

    if (window.isModified()) {
      m_key->disableAccel(origAccel);
      if (!window.accel().isEmpty())
        m_key->add(window.accel(), KeySource::UserDefined);
    }

    getRoot()->layout();
  }

  void onDeleteAccel(int index)
  {
    if (Alert::show("Warning"
        "<<Do you really want to delete this keyboard shortcut?"
        "||&Yes||&No") != 1)
      return;

    m_key->disableAccel(m_key->accels()[index]);
    getRoot()->layout();
  }

  void onAddAccel() {
    ui::Accelerator accel;
    SelectAccelerator window(accel, m_key ? m_key->keycontext(): KeyContext::Any);
    window.openWindowInForeground();

    if (window.isModified()) {
      if (!m_key) {
        ASSERT(m_menuitem);
        if (!m_menuitem)
          return;

        m_key = app::KeyboardShortcuts::instance()->command(
          m_menuitem->getCommand()->short_name(),
          m_menuitem->getParams());

        m_menuitem->setKey(m_key);
      }

      m_key->add(window.accel(), KeySource::UserDefined);
    }

    getRoot()->layout();
  }

  void onPreferredSize(PreferredSizeEvent& ev) override {
    gfx::Size size = getTextSize();
    size.w = this->border_width.l + size.w + this->border_width.r;
    size.h = this->border_width.t + size.h + this->border_width.b
      + 4*jguiscale();

    if (m_key && !m_key->accels().empty()) {
      size_t combos = m_key->accels().size();
      if (combos > 1)
        size.h *= combos;
    }

    ev.setPreferredSize(size);
  }

  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.getGraphics();
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
    gfx::Rect bounds = getClientBounds();
    gfx::Color fg, bg;

    if (isSelected()) {
      fg = theme->getColor(ThemeColor::ListItemSelectedText);
      bg = theme->getColor(ThemeColor::ListItemSelectedFace);
    }
    else {
      fg = theme->getColor(ThemeColor::ListItemNormalText);
      bg = theme->getColor(ThemeColor::ListItemNormalFace);
    }

    g->fillRect(bg, bounds);

    bounds.shrink(getBorder());
    g->drawUIString(getText(), fg, bg,
      gfx::Point(
        bounds.x + m_level*16 * jguiscale(),
        bounds.y + 2*jguiscale()));

    if (m_key && !m_key->accels().empty()) {
      std::string buf;
      int y = bounds.y;
      int dh = getTextSize().h + 4*jguiscale();
      int i = 0;

      for (const Accelerator& accel : m_key->accels()) {
        if (i != m_hotAccel || !m_changeButton) {
          g->drawString(accel.toString(), fg, bg,
            gfx::Point(bounds.x + g_sep, y + 2*jguiscale()));
        }

        y += dh;
        ++i;
      }
    }
  }

  void onResize(ResizeEvent& ev) override {
    ListItem::onResize(ev);

    destroyButtons();
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kMouseLeaveMessage: {
        m_hotAccel = -1;
        destroyButtons();
        invalidate();
        break;
      }

      case kMouseMoveMessage: {
        gfx::Rect bounds = getBounds();
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        int hotAccel = -1;

        const Accelerators* accels = (m_key ? &m_key->accels() : NULL);
        int y = bounds.y;
        int dh = getTextSize().h + 4*jguiscale();
        int maxi = (accels && accels->size() > 1 ? accels->size(): 1);

        for (int i=0; i<maxi; ++i, y += dh) {
          int w = Graphics::measureUIStringLength(
            (accels && i < (int)accels->size() ? (*accels)[i].toString().c_str(): ""),
            getFont());
          gfx::Rect itemBounds(bounds.x + g_sep, y, w, dh);

          if (mouseMsg->position().y >= bounds.y &&
              mouseMsg->position().y < bounds.y+bounds.h) {
            itemBounds = itemBounds.enlarge(
              gfx::Border(
                4*jguiscale(), 0,
                6*jguiscale(), 1*jguiscale()));

            if (accels && i < (int)accels->size() &&
                mouseMsg->position().y >= itemBounds.y &&
                mouseMsg->position().y < itemBounds.y+itemBounds.h) {
              hotAccel = i;

              if (!m_changeButton) {
                m_changeButton = new Button("");
                m_changeButton->Click.connect(Bind<void>(&KeyItem::onChangeAccel, this, i));
                setup_mini_look(m_changeButton);
                addChild(m_changeButton);
              }

              if (!m_deleteButton) {
                m_deleteButton = new Button("");
                m_deleteButton->Click.connect(Bind<void>(&KeyItem::onDeleteAccel, this, i));
                setup_mini_look(m_deleteButton);
                addChild(m_deleteButton);
              }

              m_changeButton->setBgColor(gfx::ColorNone);
              m_changeButton->setBounds(itemBounds);
              m_changeButton->setText((*accels)[i].toString());

              const char* label = "x";
              m_deleteButton->setBgColor(gfx::ColorNone);
              m_deleteButton->setBounds(gfx::Rect(
                  itemBounds.x + itemBounds.w + 2*jguiscale(),
                  itemBounds.y,
                  Graphics::measureUIStringLength(
                    label, getFont()) + 4*jguiscale(),
                  itemBounds.h));
              m_deleteButton->setText(label);

              invalidate();
            }

            if (i == 0 && (!m_menuitem || m_menuitem->getCommand())) {
              if (!m_addButton) {
                m_addButton = new Button("");
                m_addButton->Click.connect(Bind<void>(&KeyItem::onAddAccel, this));
                setup_mini_look(m_addButton);
                addChild(m_addButton);
              }

              itemBounds.w = 8*jguiscale() + Graphics::measureUIStringLength("Add", getFont());
              itemBounds.x -= itemBounds.w + 2*jguiscale();

              m_addButton->setBgColor(gfx::ColorNone);
              m_addButton->setBounds(itemBounds);
              m_addButton->setText("Add");

              invalidate();
            }
          }
        }

        if (m_hotAccel != hotAccel) {
          if (hotAccel == -1)
            destroyButtons();
          m_hotAccel = hotAccel;
          invalidate();
        }
        break;
      }
    }
    return ListItem::onProcessMessage(msg);
  }

  void destroyButtons() {
    delete m_changeButton;
    delete m_deleteButton;
    delete m_addButton;
    m_changeButton = NULL;
    m_deleteButton = NULL;
    m_addButton = NULL;
  }

  Key* m_key;
  Key* m_keyOrig;
  AppMenuItem* m_menuitem;
  int m_level;
  ui::Accelerators m_newAccels;
  ui::Button* m_changeButton;
  ui::Button* m_deleteButton;
  ui::Button* m_addButton;
  int m_hotAccel;
};

class KeyboardShortcutsWindow : public app::gen::KeyboardShortcuts {
public:
  KeyboardShortcutsWindow() {
    setAutoRemap(false);

    section()->addItem("Menus");
    section()->addItem("Commands");
    section()->addItem("Tools");
    section()->addItem("Action Modifiers");

    section()->Change.connect(Bind<void>(&KeyboardShortcutsWindow::onSectionChange, this));
    importButton()->Click.connect(Bind<void>(&KeyboardShortcutsWindow::onImport, this));
    exportButton()->Click.connect(Bind<void>(&KeyboardShortcutsWindow::onExport, this));
    resetButton()->Click.connect(Bind<void>(&KeyboardShortcutsWindow::onReset, this));

    fillAllLists();
  }

  void restoreKeys() {
    for (KeyItem* keyItem : m_allKeyItems) {
      keyItem->restoreKeys();
    }
  }

private:
  void deleteAllKeyItems() {
    while (menus()->getLastChild())
      menus()->removeChild(menus()->getLastChild());
    while (commands()->getLastChild())
      commands()->removeChild(commands()->getLastChild());
    while (tools()->getLastChild())
      tools()->removeChild(tools()->getLastChild());
    while (actions()->getLastChild())
      actions()->removeChild(actions()->getLastChild());

    for (KeyItem* keyItem : m_allKeyItems) {
      delete keyItem;
    }
    m_allKeyItems.clear();
  }

  void fillAllLists() {
    deleteAllKeyItems();

    // Load keyboard shortcuts
    fillList(this->menus(), AppMenus::instance()->getRootMenu(), 0);
    for (Key* key : *app::KeyboardShortcuts::instance()) {
      std::string text = key->triggerString();
      switch (key->keycontext()) {
        case KeyContext::Selection:
          text = "Selection context: " + text;
          break;
        case KeyContext::MovingPixels:
          text = "Moving pixels context: " + text;
          break;
        case KeyContext::MoveTool:
          text = "Move tool: " + text;
          break;
      }
      KeyItem* keyItem = new KeyItem(text, key, NULL, 0);

      ListBox* listBox = NULL;
      switch (key->type()) {
        case KeyType::Command:
          listBox = this->commands();
          break;
        case KeyType::Tool:
        case KeyType::Quicktool: {
          listBox = this->tools();
          break;
        }
        case KeyType::Action:
          listBox = this->actions();
          break;
      }

      ASSERT(listBox);
      if (listBox) {
        m_allKeyItems.push_back(keyItem);
        listBox->addChild(keyItem);
      }
    }

    this->commands()->sortItems();
    this->tools()->sortItems();
    this->actions()->sortItems();

    onSectionChange();
  }

  void onSectionChange() {
    int section = this->section()->getSelectedItemIndex();

    menusView()->setVisible(section == 0);
    commandsView()->setVisible(section == 1);
    toolsView()->setVisible(section == 2);
    actionsView()->setVisible(section == 3);
    layout();
  }

  void onImport() {
    std::string filename = app::show_file_selector("Import Keyboard Shortcuts", "",
      KEYBOARD_FILENAME_EXTENSION);
    if (filename.empty())
      return;

    app::KeyboardShortcuts::instance()->importFile(filename.c_str(), KeySource::UserDefined);
    fillAllLists();
    layout();
  }

  void onExport() {
  again:
    std::string filename = app::show_file_selector("Export Keyboard Shortcuts", "",
      KEYBOARD_FILENAME_EXTENSION);
    if (!filename.empty()) {
      if (base::is_file(filename)) {
        int ret = Alert::show("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
          base::get_file_name(filename).c_str());
        if (ret == 2)
          goto again;
        else if (ret != 1)
          return;
      }

      app::KeyboardShortcuts::instance()->exportFile(filename.c_str());
    }
  }

  void onReset() {
    if (Alert::show("Warning"
        "<<Do you want to restore all keyboard shortcuts"
        "<<to their original default settings?"
        "||&Yes||&No") == 1) {
      app::KeyboardShortcuts::instance()->reset();
      layout();
    }
  }

  void fillList(ListBox* listbox, Menu* menu, int level) {
    for (Widget* child : menu->getChildren()) {
      if (AppMenuItem* menuItem = dynamic_cast<AppMenuItem*>(child)) {
        if (menuItem == AppMenus::instance()->getRecentListMenuitem())
          continue;

        KeyItem* keyItem = new KeyItem(
          menuItem->getText().c_str(),
          menuItem->getKey(), menuItem, level);

        listbox->addChild(keyItem);

        if (menuItem->hasSubmenu())
          fillList(listbox, menuItem->getSubmenu(), level+1);
      }
    }
  }

  std::vector<KeyItem*> m_allKeyItems;
};

class KeyboardShortcutsCommand : public Command {
public:
  KeyboardShortcutsCommand();
  Command* clone() const override { return new KeyboardShortcutsCommand(*this); }

protected:
  void onExecute(Context* context);
};

KeyboardShortcutsCommand::KeyboardShortcutsCommand()
  : Command("KeyboardShortcuts",
            "Keyboard Shortcuts",
            CmdUIOnlyFlag)
{
}

void KeyboardShortcutsCommand::onExecute(Context* context)
{
  KeyboardShortcutsWindow window;

  window.setBounds(gfx::Rect(0, 0, ui::display_w()*3/4, ui::display_h()*3/4));
  g_sep = window.getBounds().w / 2;

  window.centerWindow();
  window.setVisible(true);
  window.openWindowInForeground();

  if (window.getKiller() == window.ok()) {
    // Save keyboard shortcuts in configuration file
    {
      ResourceFinder rf;
      rf.includeUserDir("user." KEYBOARD_FILENAME_EXTENSION);
      std::string fn = rf.getFirstOrCreateDefault();
      KeyboardShortcuts::instance()->exportFile(fn);
    }
  }
  else {
    window.restoreKeys();
  }
}

Command* CommandFactory::createKeyboardShortcutsCommand()
{
  return new KeyboardShortcutsCommand;
}

} // namespace app
