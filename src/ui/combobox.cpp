// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/compiler_specific.h"
#include "gfx/size.h"
#include "ui/gui.h"

#include <allegro.h>

using namespace gfx;

namespace ui {

class ComboBoxButton : public Button
{
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
  m_selected = 0;
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

int ComboBox::addItem(const char* text)
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

void ComboBox::insertItem(int itemIndex, const char* text)
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

const char* ComboBox::getItemText(int itemIndex) const
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    ListItem* item = m_items[itemIndex];
    return item->getText();
  }
  else
    return "";
}

void ComboBox::setItemText(int itemIndex, const char* text)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  ListItem* item = m_items[itemIndex];
  item->setText(text);
}

int ComboBox::findItemIndex(const char* text)
{
  int itemIndex = 0;

  ListItems::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    ListItem* item = *it;

    if ((m_casesensitive && ustrcmp(item->getText(), text) == 0) ||
        (!m_casesensitive && ustricmp(item->getText(), text) == 0)) {
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
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    m_selected = itemIndex;

    ListItems::iterator it = m_items.begin() + itemIndex;
    ListItem* item = *it;
    m_entry->setText(item->getText());
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
  switch (msg->type) {

    case kCloseMessage:
      closeListBox();
      break;

    case kResizeMessage: {
      JRect cbox = jrect_new_copy(&msg->setpos.rect);
      jrect_copy(this->rc, cbox);

      // Button
      Size buttonSize = m_button->getPreferredSize();
      cbox->x1 = msg->setpos.rect.x2 - buttonSize.w;
      jwidget_set_rect(m_button, cbox);

      // Entry
      cbox->x2 = cbox->x1;
      cbox->x1 = msg->setpos.rect.x1;
      jwidget_set_rect(m_entry, cbox);

      jrect_free(cbox);
      return true;
    }

    case kWinMoveMessage:
      if (m_window) {
        JRect rc = getListBoxPos();
        m_window->moveWindow(rc);
        jrect_free(rc);
      }
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

void ComboBox::onPreferredSize(PreferredSizeEvent& ev)
{
  Size reqSize(0, 0);
  Size entrySize = m_entry->getPreferredSize();

  // Get the text-length of every item and put in 'w' the maximum value
  ListItems::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    int item_w =
      2*jguiscale()+
      text_length(this->getFont(), (*it)->getText())+
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
  switch (msg->type) {

    case kKeyDownMessage:
      if (hasFocus()) {
        if (!m_comboBox->isEditable()) {
          if (msg->key.scancode == KEY_SPACE ||
              msg->key.scancode == KEY_ENTER ||
              msg->key.scancode == KEY_ENTER_PAD) {
            m_comboBox->switchListBox();
            return true;
          }
        }
        else {
          if (msg->key.scancode == KEY_ENTER ||
              msg->key.scancode == KEY_ENTER_PAD) {
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
      else
        return true;
      break;

    case kPaintMessage:
      getTheme()->draw_combobox_entry(this, &msg->draw.rect);
      return true;

  }

  return Entry::onProcessMessage(msg);
}

bool ComboBoxListBox::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case kMouseUpMessage:
      {
        int index = m_comboBox->getSelectedItemIndex();
        if (isValidItem(index))
          m_comboBox->onChange();

        m_comboBox->closeListBox();
      }
      return true;

    case kKeyDownMessage:
      if (hasFocus()) {
        if (msg->key.scancode == KEY_SPACE ||
            msg->key.scancode == KEY_ENTER ||
            msg->key.scancode == KEY_ENTER_PAD) {
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
    m_window = new Window(false, NULL);
    View* view = new View();
    m_listbox = new ComboBoxListBox(this);
    m_window->setOnTop(true);
    jwidget_noborders(m_window);

    Widget* viewport = view->getViewport();
    int size = getItemCount();
    jwidget_set_min_size
      (viewport,
       m_button->rc->x2 - m_entry->rc->x1 - view->border_width.l - view->border_width.r,
       +viewport->border_width.t
       +(2*jguiscale()+jwidget_get_text_height(m_listbox))*MID(1, size, 16)+
       +viewport->border_width.b);

    m_window->addChild(view);
    view->attachToView(m_listbox);

    m_listbox->selectIndex(m_selected);

    m_window->remapWindow();

    JRect rc = getListBoxPos();
    m_window->positionWindow(rc->x1, rc->y1);
    jrect_free(rc);

    getManager()->addMessageFilter(kMouseDownMessage, this);

    m_window->openWindow();
    getManager()->setFocus(m_listbox);
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
  }
}

void ComboBox::switchListBox()
{
  if (!m_window)
    openListBox();
  else
    closeListBox();
}

JRect ComboBox::getListBoxPos()
{
  JRect rc = jrect_new(m_entry->rc->x1,
                       m_entry->rc->y2,
                       m_button->rc->x2,
                       m_entry->rc->y2+jrect_h(m_window->rc));
  if (rc->y2 > JI_SCREEN_H)
    jrect_displace(rc, 0, -(jrect_h(rc)+jrect_h(m_entry->rc)));
  return rc;
}

void ComboBox::onChange()
{
  Change();
}

} // namespace ui
