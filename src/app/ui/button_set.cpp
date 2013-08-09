/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include <allegro/unicode.h>
#include <stdarg.h>

#include "app/ui/button_set.h"
#include "base/bind.h"
#include "app/modules/gui.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

namespace app {

using namespace ui;

static Widget* find_selected(Widget* widget);
static int select_button(Widget* widget, int index);

class ButtonSet::Item : public RadioButton
{
public:
  Item(int index, int radioGroup, int b1, int b2, int b3, int b4)
    : RadioButton(NULL, radioGroup, kButtonWidget)
    , m_index(index)
  {
    setRadioGroup(radioGroup);
    setAlign(JI_CENTER | JI_MIDDLE);

    setup_mini_look(this);
    setup_bevels(this, b1, b2, b3, b4);
  }

  int getIndex() const { return m_index; }

private:
  int m_index;
};

ButtonSet::ButtonSet(int w, int h, int firstSelected, ...)
  : Box(JI_VERTICAL | JI_HOMOGENEOUS)
{
  Box* hbox = NULL;
  int x, y, icon;
  va_list ap;
  int c = 0;
  char buf[256];

  va_start(ap, firstSelected);

  jwidget_noborders(this);

  for (y=0; y<h; y++) {
    if (w > 1) {
      hbox = new Box(JI_HORIZONTAL | JI_HOMOGENEOUS);
      jwidget_noborders(hbox);
    }

    for (x=0; x<w; x++) {
      icon = va_arg(ap, int);

      Item* item = new Item(c,
                            (int)(reinterpret_cast<unsigned long>(this) & 0xffffffff),
                            x ==   0 && y ==   0 ? 2: 0,
                            x == w-1 && y ==   0 ? 2: 0,
                            x ==   0 && y == h-1 ? 2: 0,
                            x == w-1 && y == h-1 ? 2: 0);

      m_items.push_back(item);

      usprintf(buf, "radio%d", c);
      item->setId(buf);

      if (icon >= 0)
        set_gfxicon_to_button(item, icon, icon+1, icon, JI_CENTER | JI_MIDDLE);

      item->Click.connect(Bind<void>(&ButtonSet::onItemChange, this));

      if (c == firstSelected)
        item->setSelected(true);

      if (hbox)
        hbox->addChild(item);
      else
        this->addChild(item);

      c++;
    }

    if (hbox)
      this->addChild(hbox);
  }

  va_end(ap);
}

int ButtonSet::getSelectedItem() const
{
  Item* sel = findSelectedItem();

  if (sel)
    return sel->getIndex();
  else
    return -1;
}

void ButtonSet::setSelectedItem(int index)
{
  Item* sel = findSelectedItem();

  if (!sel || sel->getIndex() != index) {
    sel->setSelected(false);
    m_items[index]->setSelected(true);
  }
}

ButtonSet::Item* ButtonSet::findSelectedItem() const
{
  for (Items::const_iterator it=m_items.begin(), end=m_items.end();
         it != end; ++it) {
    if ((*it)->isSelected())
      return *it;
  }
  return NULL;
}

void ButtonSet::onItemChange()
{
  ItemChange();
}
    
} // namespace app
