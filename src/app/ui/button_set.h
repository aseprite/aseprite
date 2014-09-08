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

#ifndef APP_UI_BUTTON_SET_H_INCLUDED
#define APP_UI_BUTTON_SET_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "ui/grid.h"

#include <string>

namespace app {

  class ButtonSet : public ui::Grid {
  public:
    class Item : public ui::Widget {
    public:
      Item();
      void setIcon(she::Surface* icon);
      ButtonSet* buttonSet();
    protected:
      void onPaint(ui::PaintEvent& ev) override;
      bool onProcessMessage(ui::Message* msg) override;
      void onPreferredSize(ui::PreferredSizeEvent& ev) override;
    private:
      she::Surface* m_icon;
    };

    ButtonSet(int columns);

    void addItem(she::Surface* icon, int hspan = 1, int vspan = 1);
    Item* getItem(int index);

    int selectedItem() const;
    void setSelectedItem(int index);
    void setSelectedItem(Item* item);
    void deselectItems();

    void setOfferCapture(bool state);

    Signal0<void> ItemChange;

  protected:
    virtual void onItemChange();

  private:
    Item* findSelectedItem() const;

    bool m_offerCapture;
  };

} // namespace app

#endif
