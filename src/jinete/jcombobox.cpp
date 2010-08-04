/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <allegro.h>

#include "jinete/jinete.h"
#include "Vaca/Size.h"
#include "Vaca/PreferredSizeEvent.h"

struct ComboBox::Item
{
  std::string text;
  void* data;

  Item() : data(NULL) { }
};

#define IS_VALID_ITEM(combobox, index)				\
  (index >= 0 && index < combobox->getItemCount())

static bool combobox_entry_msg_proc(JWidget widget, JMessage msg);
static bool combobox_button_msg_proc(JWidget widget, JMessage msg);
static bool combobox_listbox_msg_proc(JWidget widget, JMessage msg);
static void combobox_button_cmd(JWidget widget, void *data);

ComboBox::ComboBox()
  : Widget(JI_COMBOBOX)
{
  m_entry = jentry_new(256, "");
  m_button = jbutton_new("");
  m_window = NULL;
  m_selected = 0;
  m_editable = false;
  m_clickopen = true;
  m_casesensitive = true;

  m_entry->user_data[0] = this;
  m_button->user_data[0] = this;

  /* TODO this separation should be from the JTheme */
  this->child_spacing = 0;

  jwidget_focusrest(this, true);
  jwidget_add_hook(m_entry, JI_WIDGET, combobox_entry_msg_proc, NULL);
  jwidget_add_hook(m_button, JI_WIDGET, combobox_button_msg_proc, NULL);

  jwidget_expansive(m_entry, true);
  jbutton_set_bevel(m_button, 0, 2, 0, 2);
  jbutton_add_command_data(m_button, combobox_button_cmd, this);

  jwidget_add_child(this, m_entry);
  jwidget_add_child(this, m_button);

  setEditable(m_editable);

  jwidget_init_theme(this);
}

ComboBox::~ComboBox()
{
  removeAllItems();
}

void ComboBox::setEditable(bool state)
{
  m_editable = state;

  if (state) {
    jentry_readonly(m_entry, false);
    jentry_show_cursor(m_entry);
  }
  else {
    jentry_readonly(m_entry, true);
    jentry_hide_cursor(m_entry);
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

int ComboBox::addItem(const std::string& text)
{
  bool sel_first = m_items.empty();
  Item* item = new Item();
  item->text = text;

  m_items.push_back(item);

  if (sel_first)
    setSelectedItem(0);

  return m_items.size()-1;
}

void ComboBox::insertItem(int itemIndex, const std::string& text)
{
  bool sel_first = m_items.empty();
  Item* item = new Item();
  item->text = text;

  m_items.insert(m_items.begin() + itemIndex, item);

  if (sel_first)
    setSelectedItem(0);
}

void ComboBox::removeItem(int itemIndex)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  Item* item = m_items[itemIndex];

  m_items.erase(m_items.begin() + itemIndex);
  delete item;
}

void ComboBox::removeAllItems()
{
  std::vector<Item*>::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it)
    delete *it;

  m_items.clear();
}

int ComboBox::getItemCount()
{
  return m_items.size();
}

std::string ComboBox::getItemText(int itemIndex)
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    Item* item = m_items[itemIndex];
    return item->text;
  }
  else
    return "";
}

void ComboBox::setItemText(int itemIndex, const std::string& text)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  Item* item = m_items[itemIndex];
  item->text = text;
}

int ComboBox::findItemIndex(const std::string& text)
{
  int itemIndex = 0;

  std::vector<Item*>::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    Item* item = *it;

    if ((m_casesensitive && ustrcmp(item->text.c_str(), text.c_str()) == 0) ||
	(!m_casesensitive && ustricmp(item->text.c_str(), text.c_str()) == 0)) {
      return itemIndex;
    }

    itemIndex++;
  }

  return -1;
}

int ComboBox::getSelectedItem()
{
  return !m_items.empty() ? m_selected: -1;
}

void ComboBox::setSelectedItem(int itemIndex)
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    m_selected = itemIndex;

    std::vector<Item*>::iterator it = m_items.begin() + itemIndex;
    Item* item = *it;
    m_entry->setText(item->text.c_str());
  }
}

void* ComboBox::getItemData(int itemIndex)
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    Item* item = m_items[itemIndex];
    return item->data;
  }
  else
    return NULL;
}

void ComboBox::setItemData(int itemIndex, void* data)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  Item* item = m_items[itemIndex];
  item->data = data;
}

Widget* ComboBox::getEntryWidget()
{
  return m_entry;
}

Widget* ComboBox::getButtonWidget()
{
  return m_button;
}

bool ComboBox::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      closeListBox();
      break;

    case JM_SETPOS: {
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
      
    case JM_WINMOVE:
      if (m_window) {
	JRect rc = getListBoxPos();
	m_window->move_window(rc);
	jrect_free(rc);
      }
      break;

    case JM_BUTTONPRESSED:
      if (m_window != NULL) {
	if (!jwidget_get_view(m_listbox)->hasMouse()) {
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
  std::vector<Item*>::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    int item_w =
      2*jguiscale()+
      text_length(this->getFont(), (*it)->text.c_str())+
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

static bool combobox_entry_msg_proc(JWidget widget, JMessage msg)
{
  ComboBox* combobox = reinterpret_cast<ComboBox*>(widget->user_data[0]);

  switch (msg->type) {

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
	if (!combobox->isEditable()) {
	  if (msg->key.scancode == KEY_SPACE ||
	      msg->key.scancode == KEY_ENTER ||
	      msg->key.scancode == KEY_ENTER_PAD) {
	    combobox->switchListBox();
	    return true;
	  }
	}
	else {
	  if (msg->key.scancode == KEY_ENTER ||
	      msg->key.scancode == KEY_ENTER_PAD) {
	    combobox->switchListBox();
	    return true;
	  }
	}
      }
      break;

    case JM_BUTTONPRESSED:
      if (combobox->isClickOpen()) {
	combobox->switchListBox();
      }

      if (combobox->isEditable()) {
	jmanager_set_focus(widget);
      }
      else
	return true;
      break;

    case JM_DRAW:
      widget->theme->draw_combobox_entry(widget, &msg->draw.rect);
      return true;
  }

  return false;
}

static bool combobox_button_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DRAW:
      widget->theme->draw_combobox_button(widget, &msg->draw.rect);
      return true;

  }
  return false;
}

static bool combobox_listbox_msg_proc(JWidget widget, JMessage msg)
{
  ComboBox* combobox = reinterpret_cast<ComboBox*>(widget->user_data[0]);

  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_LISTBOX_CHANGE) {
	int index = jlistbox_get_selected_index(widget);

	if (IS_VALID_ITEM(combobox, index)) {
	  combobox->setSelectedItem(index);

	  jwidget_emit_signal(combobox, JI_SIGNAL_COMBOBOX_CHANGE);
	}
      }
      break;

    case JM_BUTTONRELEASED:
      {
	int index = combobox->getSelectedItem();

	combobox->closeListBox();

	if (IS_VALID_ITEM(combobox, index))
	  jwidget_emit_signal(combobox, JI_SIGNAL_COMBOBOX_SELECT);
      }
      return true;

/*     case JM_IDLE: { */
/*       /\* if the user clicks outside the listbox *\/ */
/*       if (!jmouse_b(1) && jmouse_b(0) && !jwidget_has_mouse(widget)) { */
/* 	ComboBox *combobox = jwidget_get_data(combo_widget, JI_COMBOBOX); */

/* 	if (combobox->entry && !jwidget_has_mouse(combobox->entry) && */
/* 	    combobox->button && !jwidget_has_mouse(combobox->button) && */
/* 	    combobox->window && !jwidget_has_mouse(combobox->window)) { */
/* 	  combobox_close_window(combo_widget); */
/* 	  return true; */
/* 	} */
/*       } */
/*       break; */
/*     } */

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
	if (msg->key.scancode == KEY_SPACE ||
	    msg->key.scancode == KEY_ENTER ||
	    msg->key.scancode == KEY_ENTER_PAD) {
	  combobox->closeListBox();
	  return true;
	}
      }
      break;
  }

  return false;
}

static void combobox_button_cmd(JWidget widget, void *data)
{
  ComboBox* combobox = reinterpret_cast<ComboBox*>(data);
  combobox->switchListBox();
}

void ComboBox::openListBox()
{
  if (!m_window) {
    m_window = new Frame(false, NULL);
    Widget* view = jview_new();
    m_listbox = jlistbox_new();

    m_listbox->user_data[0] = this;
    jwidget_add_hook(m_listbox, JI_WIDGET,
		     combobox_listbox_msg_proc, NULL);

    std::vector<Item*>::iterator it, end = m_items.end();
    for (it = m_items.begin(); it != end; ++it) {
      Item* item = *it;
      jwidget_add_child(m_listbox, jlistitem_new(item->text.c_str()));
    }

    m_window->set_ontop(true);
    jwidget_noborders(m_window);

    Widget* viewport = jview_get_viewport(view);
    int size = getItemCount();
    jwidget_set_min_size
      (viewport,
       m_button->rc->x2 - m_entry->rc->x1 - view->border_width.l - view->border_width.r,
       +viewport->border_width.t
       +(2*jguiscale()+jwidget_get_text_height(m_listbox))*MID(1, size, 16)+
       +viewport->border_width.b);

    jwidget_add_child(m_window, view);
    jview_attach(view, m_listbox);

    jwidget_signal_off(m_listbox);
    jlistbox_select_index(m_listbox, m_selected);
    jwidget_signal_on(m_listbox);

    m_window->remap_window();

    JRect rc = getListBoxPos();
    m_window->position_window(rc->x1, rc->y1);
    jrect_free(rc);

    jmanager_add_msg_filter(JM_BUTTONPRESSED, this);

    m_window->open_window_bg();
    jmanager_set_focus(m_listbox);
  }
}

void ComboBox::closeListBox()
{
  if (m_window) {
    m_window->closeWindow(this);
    delete m_window;		// window, frame
    m_window = NULL;

    jmanager_remove_msg_filter(JM_BUTTONPRESSED, this);
    jmanager_set_focus(m_entry);
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
