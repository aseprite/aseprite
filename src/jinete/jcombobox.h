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

#ifndef JINETE_JCOMBOBOX_H_INCLUDED
#define JINETE_JCOMBOBOX_H_INCLUDED

#include "jinete/jwidget.h"
#include <vector>
#include <string>

class Frame;

class ComboBox : public Widget
{
public:
  ComboBox();
  ~ComboBox();

  void setEditable(bool state);
  void setClickOpen(bool state);
  void setCaseSensitive(bool state);

  bool isEditable();
  bool isClickOpen();
  bool isCaseSensitive();

  int addItem(const std::string& text);
  void insertItem(int itemIndex, const std::string& text);
  void removeItem(int itemIndex);
  void removeAllItems();

  int getItemCount();

  std::string getItemText(int itemIndex);
  void setItemText(int itemIndex, const std::string& text);
  int findItemIndex(const std::string& text);

  int getSelectedItem();
  void setSelectedItem(int itemIndex);

  void* getItemData(int itemIndex);
  void setItemData(int itemIndex, void* data);

  Widget* getEntryWidget();
  Widget* getButtonWidget();

  void openListBox();
  void closeListBox();
  void switchListBox();
  JRect getListBoxPos();

protected:
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);

private:
  struct Item;

  Widget* m_entry;
  Widget* m_button;
  Frame* m_window;
  Widget* m_listbox;
  std::vector<Item*> m_items;
  int m_selected;
  bool m_editable : 1;
  bool m_clickopen : 1;
  bool m_casesensitive : 1;
};

#endif
