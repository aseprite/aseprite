// Aseprite UI Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/combobox.h"

#include "base/clamp.h"
#include "gfx/size.h"
#include "os/font.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/listbox.h"
#include "ui/listitem.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/resize_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/window.h"

#include <algorithm>

namespace ui {

using namespace gfx;

class ComboBoxButton : public Button {
public:
  ComboBoxButton() : Button("") {
    setFocusStop(false);
  }
};

class ComboBoxEntry : public Entry {
public:
  ComboBoxEntry(ComboBox* comboBox)
    : Entry(256, ""),
      m_comboBox(comboBox) {
  }

protected:
  bool onProcessMessage(Message* msg) override;
  void onPaint(PaintEvent& ev) override;
  void onChange() override;

private:
  ComboBox* m_comboBox;
};

class ComboBoxListBox : public ListBox {
public:
  ComboBoxListBox(ComboBox* comboBox)
    : m_comboBox(comboBox) {
    for (auto item : *comboBox) {
      if (item->parent())
        item->parent()->removeChild(item);
      addChild(item);
    }
  }

  void clean() {
    // Remove all added items so ~Widget() don't delete them.
    removeAllChildren();
    selectChild(nullptr);
  }

protected:
  bool onProcessMessage(Message* msg) override;
  void onChange() override;

private:
  bool isValidItem(int index) const {
    return (index >= 0 && index < m_comboBox->getItemCount());
  }

  ComboBox* m_comboBox;
};

ComboBox::ComboBox()
  : Widget(kComboBoxWidget)
  , m_entry(new ComboBoxEntry(this))
  , m_button(new ComboBoxButton())
  , m_window(nullptr)
  , m_listbox(nullptr)
  , m_selected(-1)
  , m_editable(false)
  , m_clickopen(true)
  , m_casesensitive(true)
  , m_filtering(false)
  , m_useCustomWidget(false)
{
  m_entry->setExpansive(true);

  // When the "m_button" is clicked ("Click" signal) call onButtonClick() method
  m_button->Click.connect(&ComboBox::onButtonClick, this);

  addChild(m_entry);
  addChild(m_button);

  setFocusStop(true);
  setEditable(m_editable);

  initTheme();
}

ComboBox::~ComboBox()
{
  removeMessageFilters();
  deleteAllItems();
}

void ComboBox::setEditable(bool state)
{
  m_editable = state;

  if (state) {
    m_entry->setReadOnly(false);
    m_entry->showCaret();
  }
  else {
    m_entry->setReadOnly(true);
    m_entry->hideCaret();
  }
}

void ComboBox::setClickOpen(bool state)
{
  m_clickopen = state;
}

void ComboBox::setCaseSensitive(bool state)
{
  m_casesensitive = state;
}

void ComboBox::setUseCustomWidget(bool state)
{
  m_useCustomWidget = state;
}

int ComboBox::addItem(Widget* item)
{
  bool sel_first = m_items.empty();

  m_items.push_back(item);

  if (sel_first && !isEditable())
    setSelectedItemIndex(0);

  return m_items.size()-1;
}

int ComboBox::addItem(const std::string& text)
{
  return addItem(new ListItem(text));
}

void ComboBox::insertItem(int itemIndex, Widget* item)
{
  bool sel_first = m_items.empty();

  m_items.insert(m_items.begin() + itemIndex, item);

  if (sel_first)
    setSelectedItemIndex(0);
}

void ComboBox::insertItem(int itemIndex, const std::string& text)
{
  insertItem(itemIndex, new ListItem(text));
}

void ComboBox::removeItem(Widget* item)
{
  auto it = std::find(m_items.begin(), m_items.end(), item);
  ASSERT(it != m_items.end());
  if (it != m_items.end())
    m_items.erase(it);

  // Do not delete the given "item"
}

void ComboBox::deleteItem(int itemIndex)
{
  ASSERT(itemIndex >= 0 && (std::size_t)itemIndex < m_items.size());

  Widget* item = m_items[itemIndex];

  m_items.erase(m_items.begin() + itemIndex);
  delete item;
}

void ComboBox::deleteAllItems()
{
  for (Widget* item : m_items)
    delete item;                // widget

  m_items.clear();
  m_selected = -1;
}

int ComboBox::getItemCount() const
{
  return m_items.size();
}

Widget* ComboBox::getItem(const int itemIndex) const
{
  if (itemIndex >= 0 && (std::size_t)itemIndex < m_items.size()) {
    return m_items[itemIndex];
  }
  else
    return nullptr;
}

const std::string& ComboBox::getItemText(int itemIndex) const
{
  if (itemIndex >= 0 && (std::size_t)itemIndex < m_items.size()) {
    Widget* item = m_items[itemIndex];
    return item->text();
  }
  else {
    // Returns the text of the combo-box (it should be empty).
    ASSERT(text().empty());
    return text();
  }
}

void ComboBox::setItemText(int itemIndex, const std::string& text)
{
  ASSERT(itemIndex >= 0 && (std::size_t)itemIndex < m_items.size());

  Widget* item = m_items[itemIndex];
  item->setText(text);
}

int ComboBox::findItemIndex(const std::string& text) const
{
  int i = 0;
  for (const Widget* item : m_items) {
    if ((m_casesensitive && item->text() == text) ||
        (!m_casesensitive && item->text() == text)) {
      return i;
    }
    i++;
  }
  return -1;
}

int ComboBox::findItemIndexByValue(const std::string& value) const
{
  int i = 0;
  for (const Widget* item : m_items) {
    if (auto listItem = dynamic_cast<const ListItem*>(item)) {
      if (listItem->getValue() == value)
        return i;
    }
    ++i;
  }
  return -1;
}

Widget* ComboBox::getSelectedItem() const
{
  return getItem(m_selected);
}

void ComboBox::setSelectedItem(Widget* item)
{
  auto it = std::find(m_items.begin(), m_items.end(), item);
  if (it != m_items.end())
    setSelectedItemIndex(std::distance(m_items.begin(), it));
  else if (m_selected >= 0) {
    m_selected = -1;
    onChange();
  }
}

int ComboBox::getSelectedItemIndex() const
{
  return (!m_items.empty() ? m_selected: -1);
}

void ComboBox::setSelectedItemIndex(int itemIndex)
{
  if (itemIndex >= 0 &&
      (std::size_t)itemIndex < m_items.size() &&
      m_selected != itemIndex) {
    m_selected = itemIndex;

    auto it = m_items.begin() + itemIndex;
    Widget* item = *it;
    m_entry->setText(item->text());
    if (isEditable())
      m_entry->setCaretToEnd();

    onChange();
  }
}

std::string ComboBox::getValue() const
{
  if (isEditable())
    return m_entry->text();
  int index = getSelectedItemIndex();
  if (index >= 0) {
    if (auto listItem = dynamic_cast<ListItem*>(m_items[index]))
      return listItem->getValue();
  }
  return std::string();
}

void ComboBox::setValue(const std::string& value)
{
  if (isEditable()) {
    m_entry->setText(value);
    m_entry->selectAllText();
  }
  else {
    int index = findItemIndexByValue(value);
    if (index >= 0)
      setSelectedItemIndex(index);
  }
}

Entry* ComboBox::getEntryWidget()
{
  return m_entry;
}

Button* ComboBox::getButtonWidget()
{
  return m_button;
}

bool ComboBox::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseMessage:
      closeListBox();
      break;

    case kWinMoveMessage:
      if (m_window)
        m_window->moveWindow(getListBoxPos());
      break;

    case kKeyDownMessage:
      if (m_window) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        // If the popup is opened
        if (scancode == kKeyEsc) {
          closeListBox();
          return true;
        }
      }
      break;

    case kMouseDownMessage:
      if (m_window) {
        if (!View::getView(m_listbox)->hasMouse()) {
          closeListBox();

          MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
          Widget* pick = manager()->pick(mouseMsg->position());
          if (pick && pick->hasAncestor(this))
            return true;
        }
      }
      break;

    case kFocusEnterMessage:
      // Here we focus the entry field only if the combobox is
      // editable and receives the focus in a direct way (e.g. when
      // the window was just opened and the combobox is the first
      // child or has the "focus magnet" flag enabled.)
      if ((isEditable()) &&
          (manager()->getFocus() == this)) {
        m_entry->requestFocus();
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void ComboBox::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  if (m_window) {
    m_window->initTheme();
    m_window->noBorderNoChildSpacing();
  }
}

void ComboBox::onResize(ResizeEvent& ev)
{
  gfx::Rect bounds = ev.bounds();
  setBoundsQuietly(bounds);

  // Button
  Size buttonSize = m_button->sizeHint();
  m_button->setBounds(Rect(bounds.x2() - buttonSize.w, bounds.y,
                           buttonSize.w, bounds.h));

  // Entry
  m_entry->setBounds(Rect(bounds.x, bounds.y,
                          bounds.w - buttonSize.w, bounds.h));

  putSelectedItemAsCustomWidget();
}

void ComboBox::onSizeHint(SizeHintEvent& ev)
{
  Size reqSize(0, 0);

  // Calculate the max required width depending on the text-length of
  // each item.
  for (const auto& item : m_items)
    reqSize |= Entry::sizeHintWithText(m_entry, item->text());

  Size buttonSize = m_button->sizeHint();
  reqSize.w += buttonSize.w;
  reqSize.h = std::max(reqSize.h, buttonSize.h);

  ev.setSizeHint(reqSize);
}

bool ComboBoxEntry::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kKeyDownMessage:
      if (hasFocus()) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        // In a non-editable ComboBox
        if (!m_comboBox->isEditable()) {
          if (scancode == kKeySpace ||
              scancode == kKeyEnter ||
              scancode == kKeyEnterPad) {
            m_comboBox->switchListBox();
            return true;
          }
        }
        // In a editable ComboBox
        else {
          if (scancode == kKeyUp ||
              scancode == kKeyDown ||
              scancode == kKeyPageUp ||
              scancode == kKeyPageDown) {
            if (m_comboBox->m_listbox &&
                m_comboBox->m_listbox->isVisible()) {
              m_comboBox->m_listbox->requestFocus();
              m_comboBox->m_listbox->sendMessage(msg);
              return true;
            }
          }
          else if (scancode == kKeyEnter ||
                   scancode == kKeyEnterPad) {
            m_comboBox->onEnterOnEditableEntry();
          }
        }
      }
      break;

    case kMouseDownMessage:
      if (m_comboBox->isClickOpen() &&
          (!m_comboBox->isEditable() ||
           !m_comboBox->m_items.empty())) {
        m_comboBox->switchListBox();
      }

      if (m_comboBox->isEditable()) {
        requestFocus();
      }
      else {
        captureMouse();
        return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture())
        releaseMouse();
      break;

    case kMouseMoveMessage:
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        Widget* pick = manager()->pick(mouseMsg->position());
        Widget* listbox = m_comboBox->m_listbox;

        if (pick != nullptr &&
            (pick == listbox || pick->hasAncestor(listbox))) {
          releaseMouse();

          MouseMessage mouseMsg2(kMouseDownMessage,
                                 mouseMsg->pointerType(),
                                 mouseMsg->button(),
                                 mouseMsg->modifiers(),
                                 mouseMsg->position());
          pick->sendMessage(&mouseMsg2);
          return true;
        }
      }
      break;

    case kFocusEnterMessage: {
      bool result = Entry::onProcessMessage(msg);
      if (m_comboBox &&
          m_comboBox->isEditable() &&
          m_comboBox->m_listbox &&
          m_comboBox->m_listbox->isVisible()) {
        // In case that the ListBox is visible and the focus is
        // obtained by the Entry field, we set the carret at the end
        // of the text. We don't select the whole text so the user can
        // delete the last caracters using backspace and complete the
        // item name.
        setCaretToEnd();
      }
      return result;
    }

  }

  return Entry::onProcessMessage(msg);
}

void ComboBoxEntry::onPaint(PaintEvent& ev)
{
  theme()->paintComboBoxEntry(ev);
}

void ComboBoxEntry::onChange()
{
  Entry::onChange();
  if (m_comboBox &&
      m_comboBox->isEditable()) {
    m_comboBox->onEntryChange();
  }
}

bool ComboBoxListBox::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseUpMessage:
      m_comboBox->closeListBox();
      return true;

    case kKeyDownMessage:
      if (hasFocus()) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        if (scancode == kKeySpace ||
            scancode == kKeyEnter ||
            scancode == kKeyEnterPad) {
          m_comboBox->closeListBox();
          return true;
        }
      }
      break;

    case kFocusEnterMessage:
      // If the ComboBox is editable, we prefer the focus in the Entry
      // field (so the user can continue editing it).
      if (m_comboBox->isEditable())
        m_comboBox->getEntryWidget()->requestFocus();
      break;
  }

  return ListBox::onProcessMessage(msg);
}

void ComboBoxListBox::onChange()
{
  ListBox::onChange();

  int index = getSelectedIndex();
  if (isValidItem(index))
    m_comboBox->setSelectedItemIndex(index);
}

// When the mouse is clicked we switch the visibility-status of the list-box
void ComboBox::onButtonClick(Event& ev)
{
  switchListBox();
}

void ComboBox::openListBox()
{
  if (!isEnabled() || m_window)
    return;

  onBeforeOpenListBox();

  m_window = new Window(Window::WithoutTitleBar);
  View* view = new View();
  m_listbox = new ComboBoxListBox(this);
  m_window->setOnTop(true);
  m_window->setWantFocus(false);

  Widget* viewport = view->viewport();
  {
    gfx::Rect entryBounds = m_entry->bounds();
    gfx::Size size;
    size.w = m_button->bounds().x2() - entryBounds.x - view->border().width();
    size.h = viewport->border().height();
    for (Widget* item : m_items)
      if (!item->hasFlags(HIDDEN))
        size.h += item->sizeHint().h;

    int max = std::max(entryBounds.y, ui::display_h() - entryBounds.y2()) - 8*guiscale();
    size.h = base::clamp(size.h, textHeight(), max);
    viewport->setMinSize(size);
  }

  m_window->addChild(view);
  view->attachToView(m_listbox);

  m_listbox->selectIndex(m_selected);

  initTheme();
  m_window->remapWindow();

  gfx::Rect rc = getListBoxPos();
  m_window->positionWindow(rc.x, rc.y);
  m_window->openWindow();

  filterMessages();

  if (isEditable())
    m_entry->requestFocus();
  else
    m_listbox->requestFocus();

  onOpenListBox();
}

void ComboBox::closeListBox()
{
  if (m_window) {
    m_listbox->clean();

    m_window->closeWindow(this);
    delete m_window;            // window, frame
    m_window = nullptr;
    m_listbox = nullptr;

    removeMessageFilters();
    putSelectedItemAsCustomWidget();
    m_entry->requestFocus();

    onCloseListBox();
  }
}

void ComboBox::switchListBox()
{
  if (!m_window)
    openListBox();
  else
    closeListBox();
}

gfx::Rect ComboBox::getListBoxPos() const
{
  gfx::Rect entryBounds = m_entry->bounds();
  gfx::Rect rc(gfx::Point(entryBounds.x,
                          entryBounds.y2()),
               gfx::Point(m_button->bounds().x2(),
                          entryBounds.y2() + m_window->bounds().h));

  if (rc.y2() > ui::display_h())
    rc.offset(0, -(rc.h + entryBounds.h));

  return rc;
}

void ComboBox::onChange()
{
  Change();
}

void ComboBox::onEntryChange()
{
  // Do nothing
}

void ComboBox::onBeforeOpenListBox()
{
  // Do nothing
}

void ComboBox::onOpenListBox()
{
  OpenListBox();
}

void ComboBox::onCloseListBox()
{
  CloseListBox();
}

void ComboBox::onEnterOnEditableEntry()
{
  // Do nothing
}

void ComboBox::filterMessages()
{
  if (!m_filtering) {
    manager()->addMessageFilter(kMouseDownMessage, this);
    manager()->addMessageFilter(kKeyDownMessage, this);
    m_filtering = true;
  }
}

void ComboBox::removeMessageFilters()
{
  if (m_filtering) {
    manager()->removeMessageFilter(kMouseDownMessage, this);
    manager()->removeMessageFilter(kKeyDownMessage, this);
    m_filtering = false;
  }
}

void ComboBox::putSelectedItemAsCustomWidget()
{
  if (!useCustomWidget())
    return;

  Widget* item = getSelectedItem();
  if (item && item->parent() == nullptr) {
    if (!m_listbox) {
      item->setBounds(m_entry->childrenBounds());
      m_entry->addChild(item);
    }
    else {
      m_entry->removeChild(item);
    }
  }
}

} // namespace ui
