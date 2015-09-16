// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
    , m_hotAccel(-1) {
    gfx::Border border = this->border();
    border.top(0);
    border.bottom(0);
    setBorder(border);
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
          m_menuitem->getCommand()->id().c_str(),
          m_menuitem->getParams());

        m_menuitem->setKey(m_key);
      }

      m_key->add(window.accel(), KeySource::UserDefined);
    }

    getRoot()->layout();
  }

  void onPreferredSize(PreferredSizeEvent& ev) override {
    gfx::Size size = getTextSize();
    size.w = size.w + border().width();
    size.h = size.h + border().height() + 4*guiscale();

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
      fg = theme->colors.listitemSelectedText();
      bg = theme->colors.listitemSelectedFace();
    }
    else {
      fg = theme->colors.listitemNormalText();
      bg = theme->colors.listitemNormalFace();
    }

    g->fillRect(bg, bounds);

    bounds.shrink(border());
    g->drawUIString(getText(), fg, bg,
      gfx::Point(
        bounds.x + m_level*16 * guiscale(),
        bounds.y + 2*guiscale()));

    if (m_key && !m_key->accels().empty()) {
      std::string buf;
      int y = bounds.y;
      int dh = getTextSize().h + 4*guiscale();
      int i = 0;

      for (const Accelerator& accel : m_key->accels()) {
        if (i != m_hotAccel || !m_changeButton) {
          g->drawString(accel.toString(), fg, bg,
            gfx::Point(bounds.x + g_sep, y + 2*guiscale()));
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
        destroyButtons();
        invalidate();
        break;
      }

      case kMouseMoveMessage: {
        gfx::Rect bounds = getBounds();
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        const Accelerators* accels = (m_key ? &m_key->accels() : NULL);
        int y = bounds.y;
        int dh = getTextSize().h + 4*guiscale();
        int maxi = (accels && accels->size() > 1 ? accels->size(): 1);

        for (int i=0; i<maxi; ++i, y += dh) {
          int w = Graphics::measureUIStringLength(
            (accels && i < (int)accels->size() ? (*accels)[i].toString().c_str(): ""),
            getFont());
          gfx::Rect itemBounds(bounds.x + g_sep, y, w, dh);
          itemBounds = itemBounds.enlarge(
            gfx::Border(
              4*guiscale(), 0,
              6*guiscale(), 1*guiscale()));

          if (accels &&
              i < (int)accels->size() &&
              mouseMsg->position().y >= itemBounds.y &&
              mouseMsg->position().y < itemBounds.y+itemBounds.h) {
            if (m_hotAccel != i) {
              m_hotAccel = i;

              m_changeButton.reset(new Button(""));
              m_changeButton->Click.connect(Bind<void>(&KeyItem::onChangeAccel, this, i));
              setup_mini_look(m_changeButton.get());
              addChild(m_changeButton.get());

              m_deleteButton.reset(new Button(""));
              m_deleteButton->Click.connect(Bind<void>(&KeyItem::onDeleteAccel, this, i));
              setup_mini_look(m_deleteButton.get());
              addChild(m_deleteButton.get());

              m_changeButton->setBgColor(gfx::ColorNone);
              m_changeButton->setBounds(itemBounds);
              m_changeButton->setText((*accels)[i].toString());

              const char* label = "x";
              m_deleteButton->setBgColor(gfx::ColorNone);
              m_deleteButton->setBounds(gfx::Rect(
                                          itemBounds.x + itemBounds.w + 2*guiscale(),
                                          itemBounds.y,
                                          Graphics::measureUIStringLength(
                                            label, getFont()) + 4*guiscale(),
                                          itemBounds.h));
              m_deleteButton->setText(label);

              invalidate();
            }
          }

          if (i == 0 && !m_addButton &&
              (!m_menuitem || m_menuitem->getCommand())) {
            m_addButton.reset(new Button(""));
            m_addButton->Click.connect(Bind<void>(&KeyItem::onAddAccel, this));
            setup_mini_look(m_addButton.get());
            addChild(m_addButton.get());

            itemBounds.w = 8*guiscale() + Graphics::measureUIStringLength("Add", getFont());
            itemBounds.x -= itemBounds.w + 2*guiscale();

            m_addButton->setBgColor(gfx::ColorNone);
            m_addButton->setBounds(itemBounds);
            m_addButton->setText("Add");

            invalidate();
          }
        }
        break;
      }
    }
    return ListItem::onProcessMessage(msg);
  }

  void destroyButtons() {
    m_changeButton.reset();
    m_deleteButton.reset();
    m_addButton.reset();
    m_hotAccel = -1;
  }

  Key* m_key;
  Key* m_keyOrig;
  AppMenuItem* m_menuitem;
  int m_level;
  ui::Accelerators m_newAccels;
  base::SharedPtr<ui::Button> m_changeButton;
  base::SharedPtr<ui::Button> m_deleteButton;
  base::SharedPtr<ui::Button> m_addButton;
  int m_hotAccel;
};

class KeyboardShortcutsWindow : public app::gen::KeyboardShortcuts {
public:
  KeyboardShortcutsWindow() {
    setAutoRemap(false);

    section()->addChild(new ListItem("Menus"));
    section()->addChild(new ListItem("Commands"));
    section()->addChild(new ListItem("Tools"));
    section()->addChild(new ListItem("Action Modifiers"));

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
        case KeyContext::SelectionTool:
          text = "Selection Tool: " + text;
          break;
        case KeyContext::TranslatingSelection:
          text = "Translating Selection: " + text;
          break;
        case KeyContext::ScalingSelection:
          text = "Scaling Selection: " + text;
          break;
        case KeyContext::RotatingSelection:
          text = "Rotating Selection: " + text;
          break;
        case KeyContext::MoveTool:
          text = "Move Tool: " + text;
          break;
        case KeyContext::FreehandTool:
          text = "Freehand Tools: " + text;
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

    section()->selectIndex(0);
    onSectionChange();
  }

  void onSectionChange() {
    int section = this->section()->getSelectedIndex();

    menusView()->setVisible(section == 0);
    commandsView()->setVisible(section == 1);
    toolsView()->setVisible(section == 2);
    actionsView()->setVisible(section == 3);
    layout();
  }

  void onImport() {
    std::string filename = app::show_file_selector("Import Keyboard Shortcuts", "",
      KEYBOARD_FILENAME_EXTENSION, FileSelectorType::Open);
    if (filename.empty())
      return;

    app::KeyboardShortcuts::instance()->importFile(filename.c_str(), KeySource::UserDefined);
    fillAllLists();
    layout();
  }

  void onExport() {
    std::string filename = app::show_file_selector(
      "Export Keyboard Shortcuts", "",
      KEYBOARD_FILENAME_EXTENSION, FileSelectorType::Save);
    if (filename.empty())
      return;

    app::KeyboardShortcuts::instance()->exportFile(filename.c_str());
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
