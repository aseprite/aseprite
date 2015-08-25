// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/size.h"
#include "she/font.h"
#include "ui/ui.h"

namespace ui {

using namespace gfx;

class ComboBoxButton : public Button {
public:
  ComboBoxButton() : Button("") {
    setFocusStop(false);
  }

  void onPaint(PaintEvent& ev) override {
    getTheme()->paintComboBoxButton(ev);
  }
};

class ComboBoxEntry : public Entry
{
public:
  ComboBoxEntry(ComboBox* comboBox)
    : Entry(256, ""),
      m_comboBox(comboBox) {
  }

protected:
  bool onProcessMessage(Message* msg) override;
  void onPaint(PaintEvent& ev) override;

private:
  ComboBox* m_comboBox;
};

class ComboBoxListBox : public ListBox
{
public:
  ComboBoxListBox(ComboBox* comboBox)
    : m_comboBox(comboBox)
  {
    for (ComboBox::ListItems::iterator
           it = comboBox->begin(), end = comboBox->end(); it != end; ++it)
      addChild(*it);
  }

  void clean()
  {
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
{
  // TODO this separation should be from the Theme*
  this->setChildSpacing(0);

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
  removeAllItems();
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

int ComboBox::addItem(ListItem* item)
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

void ComboBox::insertItem(int itemIndex, ListItem* item)
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

void ComboBox::removeItem(ListItem* item)
{
  ListItems::iterator it = std::find(m_items.begin(), m_items.end(), item);

  ASSERT(it != m_items.end());

  if (it != m_items.end())
    m_items.erase(it);

  // Do not delete the given "item"
}

void ComboBox::removeItem(int itemIndex)
{
  ASSERT(itemIndex >= 0 && (std::size_t)itemIndex < m_items.size());

  ListItem* item = m_items[itemIndex];

  m_items.erase(m_items.begin() + itemIndex);
  delete item;
}

void ComboBox::removeAllItems()
{
  ListItems::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it)
    delete *it;

  m_items.clear();
  m_selected = -1;
}

int ComboBox::getItemCount() const
{
  return m_items.size();
}

ListItem* ComboBox::getItem(int itemIndex)
{
  if (itemIndex >= 0 && (std::size_t)itemIndex < m_items.size()) {
    return m_items[itemIndex];
  }
  else
    return NULL;
}

const std::string& ComboBox::getItemText(int itemIndex) const
{
  if (itemIndex >= 0 && (std::size_t)itemIndex < m_items.size()) {
    ListItem* item = m_items[itemIndex];
    return item->getText();
  }
  else {
    // Returns the text of the combo-box (it should be empty).
    ASSERT(getText().empty());
    return getText();
  }
}

void ComboBox::setItemText(int itemIndex, const std::string& text)
{
  ASSERT(itemIndex >= 0 && (std::size_t)itemIndex < m_items.size());

  ListItem* item = m_items[itemIndex];
  item->setText(text);
}

int ComboBox::findItemIndex(const std::string& text) const
{
  int i = 0;
  for (const ListItem* item : m_items) {
    if ((m_casesensitive && item->getText() == text) ||
        (!m_casesensitive && item->getText() == text)) {
      return i;
    }
    i++;
  }
  return -1;
}

int ComboBox::findItemIndexByValue(const std::string& value) const
{
  int i = 0;
  for (const ListItem* item : m_items) {
    if (item->getValue() == value)
      return i;
    i++;
  }
  return -1;
}

ListItem* ComboBox::getSelectedItem() const
{
  return (!m_items.empty() ? m_items[m_selected]: NULL);
}

void ComboBox::setSelectedItem(ListItem* item)
{
  ListItems::iterator it = std::find(m_items.begin(), m_items.end(), item);

  ASSERT(it != m_items.end());

  if (it != m_items.end())
    setSelectedItemIndex(std::distance(m_items.begin(), it));
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

    ListItems::iterator it = m_items.begin() + itemIndex;
    ListItem* item = *it;
    m_entry->setText(item->getText());
    if (isEditable())
      m_entry->selectText(m_entry->getTextLength(), m_entry->getTextLength());

    onChange();
  }
}

std::string ComboBox::getValue() const
{
  if (isEditable())
    return m_entry->getText();
  else {
    int index = getSelectedItemIndex();
    if (index >= 0)
      return m_items[index]->getValue();
    else
      return std::string();
  }
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
          Widget* pick = getManager()->pick(mouseMsg->position());
          return (pick && pick->hasAncestor(this) ? true: false);
        }
      }
      break;

    case kFocusEnterMessage:
      // Here we focus the entry field only if the combobox is
      // editable and receives the focus in a direct way (e.g. when
      // the window was just opened and the combobox is the first
      // child or has the "focus magnet" flag enabled.)
      if ((isEditable()) &&
          (getManager()->getFocus() == this)) {
        m_entry->requestFocus();
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void ComboBox::onResize(ResizeEvent& ev)
{
  gfx::Rect bounds = ev.getBounds();
  setBoundsQuietly(bounds);

  // Button
  Size buttonSize = m_button->getPreferredSize();
  m_button->setBounds(Rect(bounds.x2() - buttonSize.w, bounds.y,
                           buttonSize.w, bounds.h));

  // Entry
  m_entry->setBounds(Rect(bounds.x, bounds.y,
                          bounds.w - buttonSize.w, bounds.h));
}

void ComboBox::onPreferredSize(PreferredSizeEvent& ev)
{
  Size reqSize(0, 0);
  Size entrySize = m_entry->getPreferredSize();

  // Get the text-length of every item and put in 'w' the maximum value
  ListItems::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    int item_w =
      2*guiscale()+
      getFont()->textLength((*it)->getText().c_str())+
      10*guiscale();

    reqSize.w = MAX(reqSize.w, item_w);
  }

  reqSize.w += entrySize.w;
  reqSize.h += entrySize.h;

  Size buttonSize = m_button->getPreferredSize();
  reqSize.w += buttonSize.w;
  reqSize.h = MAX(reqSize.h, buttonSize.h);
  ev.setPreferredSize(reqSize);
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
        Widget* pick = getManager()->pick(mouseMsg->position());
        Widget* listbox = m_comboBox->m_listbox;

        if (pick != NULL && (pick == listbox || pick->hasAncestor(listbox))) {
          releaseMouse();

          MouseMessage mouseMsg2(kMouseDownMessage,
            mouseMsg->buttons(), mouseMsg->position());
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
        selectText(getTextLength(), getTextLength());
      }
      return result;
    }

  }

  return Entry::onProcessMessage(msg);
}

void ComboBoxEntry::onPaint(PaintEvent& ev)
{
  getTheme()->paintComboBoxEntry(ev);
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

  m_window = new Window(Window::WithoutTitleBar);
  View* view = new View();
  m_listbox = new ComboBoxListBox(this);
  m_window->setOnTop(true);
  m_window->setWantFocus(false);
  m_window->noBorderNoChildSpacing();

  Widget* viewport = view->getViewport();
  int size = getItemCount();
  viewport->setMinSize
    (gfx::Size(
      m_button->getBounds().x2() - m_entry->getBounds().x - view->border().width(),
      +(2*guiscale()+m_listbox->getTextHeight())*MID(1, size, 16)+
      +viewport->border().height()));

  m_window->addChild(view);
  view->attachToView(m_listbox);

  m_listbox->selectIndex(m_selected);

  m_window->remapWindow();

  gfx::Rect rc = getListBoxPos();
  m_window->positionWindow(rc.x, rc.y);
  m_window->openWindow();

  getManager()->addMessageFilter(kMouseDownMessage, this);
  getManager()->addMessageFilter(kKeyDownMessage, this);

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

    getManager()->removeMessageFilter(kMouseDownMessage, this);
    getManager()->removeMessageFilter(kKeyDownMessage, this);
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
  gfx::Rect rc(gfx::Point(m_entry->getBounds().x,
                          m_entry->getBounds().y2()),
               gfx::Point(m_button->getBounds().x2(),
                          m_entry->getBounds().y2()+m_window->getBounds().h));

  if (rc.y2() > ui::display_h())
    rc.offset(0, -(rc.h + m_entry->getBounds().h));

  return rc;
}

void ComboBox::onChange()
{
  Change();
}

void ComboBox::onOpenListBox()
{
  OpenListBox();
}

void ComboBox::onCloseListBox()
{
  CloseListBox();
}

} // namespace ui
