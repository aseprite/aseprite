// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/compiler_specific.h"
#include "gfx/size.h"
#include "ui/ui.h"

#include <allegro.h>

namespace ui {

using namespace gfx;

class ComboBoxButton : public Button {
public:
  ComboBoxButton() : Button("") {
    setFocusStop(false);
  }

  void onPaint(PaintEvent& ev) OVERRIDE {
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
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPaint(PaintEvent& ev) OVERRIDE;

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
    while (getLastChild())
      removeChild(getLastChild());

    ASSERT(getChildren().empty());

    selectChild(NULL);
  }

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onChangeSelectedItem() OVERRIDE;

private:
  bool isValidItem(int index) const {
    return (index >= 0 && index < m_comboBox->getItemCount());
  }

  ComboBox* m_comboBox;
};

ComboBox::ComboBox()
  : Widget(kComboBoxWidget)
{
  m_entry = new ComboBoxEntry(this);
  m_button = new ComboBoxButton();
  m_window = NULL;
  m_selected = -1;
  m_editable = false;
  m_clickopen = true;
  m_casesensitive = true;

  // TODO this separation should be from the Theme*
  this->child_spacing = 0;

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

bool ComboBox::isEditable()
{
  return m_editable;
}

bool ComboBox::isClickOpen()
{
  return m_clickopen;
}

bool ComboBox::isCaseSensitive()
{
  return m_casesensitive;
}

int ComboBox::addItem(ListItem* item)
{
  bool sel_first = m_items.empty();

  m_items.push_back(item);

  if (sel_first)
    setSelectedItemIndex(0);

  return m_items.size()-1;
}

int ComboBox::addItem(const base::string& text)
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

void ComboBox::insertItem(int itemIndex, const base::string& text)
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
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

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
}

int ComboBox::getItemCount() const
{
  return m_items.size();
}

ListItem* ComboBox::getItem(int itemIndex)
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    return m_items[itemIndex];
  }
  else
    return NULL;
}

const base::string& ComboBox::getItemText(int itemIndex) const
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    ListItem* item = m_items[itemIndex];
    return item->getText();
  }
  else {
    // Returns the text of the combo-box (it should be empty).
    ASSERT(getText().empty());
    return getText();
  }
}

void ComboBox::setItemText(int itemIndex, const base::string& text)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  ListItem* item = m_items[itemIndex];
  item->setText(text);
}

int ComboBox::findItemIndex(const base::string& text)
{
  int itemIndex = 0;

  ListItems::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    ListItem* item = *it;

    if ((m_casesensitive && item->getText() == text) ||
        (!m_casesensitive && item->getText() == text)) {
      return itemIndex;
    }

    itemIndex++;
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
      (size_t)itemIndex < m_items.size() &&
      m_selected != itemIndex) {
    m_selected = itemIndex;

    ListItems::iterator it = m_items.begin() + itemIndex;
    ListItem* item = *it;
    m_entry->setText(item->getText());

    onChange();
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

    case kMouseDownMessage:
      if (m_window != NULL) {
        if (!View::getView(m_listbox)->hasMouse()) {
          closeListBox();
          return true;
        }
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
      2*jguiscale()+
      text_length(getFont(), (*it)->getText().c_str())+
      10*jguiscale();

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

        if (!m_comboBox->isEditable()) {
          if (scancode == kKeySpace ||
              scancode == kKeyEnter ||
              scancode == kKeyEnterPad) {
            m_comboBox->switchListBox();
            return true;
          }
        }
        else {
          if (scancode == kKeyEnter ||
              scancode == kKeyEnterPad) {
            m_comboBox->switchListBox();
            return true;
          }
        }
      }
      break;

    case kMouseDownMessage:
      if (m_comboBox->isClickOpen()) {
        m_comboBox->switchListBox();
      }

      if (m_comboBox->isEditable()) {
        getManager()->setFocus(this);
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
            mouseMsg->buttons(),
            mouseMsg->position());
          pick->sendMessage(&mouseMsg2);
          return true;
        }
      }
      break;

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
  }

  return ListBox::onProcessMessage(msg);
}

void ComboBoxListBox::onChangeSelectedItem()
{
  ListBox::onChangeSelectedItem();

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
  if (!m_window) {
    m_window = new Window(Window::WithoutTitleBar);
    View* view = new View();
    m_listbox = new ComboBoxListBox(this);
    m_window->setOnTop(true);
    jwidget_noborders(m_window);

    Widget* viewport = view->getViewport();
    int size = getItemCount();
    jwidget_set_min_size
      (viewport,
       m_button->getBounds().x2() - m_entry->getBounds().x - view->border_width.l - view->border_width.r,
       +viewport->border_width.t
       +(2*jguiscale()+m_listbox->getTextHeight())*MID(1, size, 16)+
       +viewport->border_width.b);

    m_window->addChild(view);
    view->attachToView(m_listbox);

    m_listbox->selectIndex(m_selected);

    m_window->remapWindow();

    gfx::Rect rc = getListBoxPos();
    m_window->positionWindow(rc.x, rc.y);

    getManager()->addMessageFilter(kMouseDownMessage, this);

    m_window->openWindow();
    getManager()->setFocus(m_listbox);

    onOpenListBox();
  }
}

void ComboBox::closeListBox()
{
  if (m_window) {
    m_listbox->clean();

    m_window->closeWindow(this);
    delete m_window;            // window, frame
    m_window = NULL;

    getManager()->removeMessageFilter(kMouseDownMessage, this);
    getManager()->setFocus(m_entry);

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

  if (rc.y2() > JI_SCREEN_H)
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
