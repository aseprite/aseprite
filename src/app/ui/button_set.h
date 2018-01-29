// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_BUTTON_SET_H_INCLUDED
#define APP_UI_BUTTON_SET_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_part.h"
#include "obs/signal.h"
#include "ui/grid.h"

#include <string>

namespace app {

  class ButtonSet : public ui::Grid {
  public:
    class Item : public ui::Widget {
    public:
      Item();
      void setHotColor(gfx::Color color);
      void setIcon(const skin::SkinPartPtr& icon, bool mono = false);
      void setMono(const bool mono) { m_mono = mono; }
      skin::SkinPartPtr icon() const { return m_icon; }
      ButtonSet* buttonSet();
    protected:
      void onPaint(ui::PaintEvent& ev) override;
      bool onProcessMessage(ui::Message* msg) override;
      void onSizeHint(ui::SizeHintEvent& ev) override;
      virtual void onClick();
      virtual void onRightClick();
    private:
      skin::SkinPartPtr m_icon;
      bool m_mono;
      gfx::Color m_hotColor;
    };

    ButtonSet(int columns);

    Item* addItem(const std::string& text, int hspan = 1, int vspan = 1);
    Item* addItem(const skin::SkinPartPtr& icon, int hspan = 1, int vspan = 1);
    Item* addItem(Item* item, int hspan = 1, int vspan = 1);
    Item* getItem(int index);
    int getItemIndex(const Item* item) const;

    int selectedItem() const;
    Item* findSelectedItem() const;
    void setSelectedItem(int index, bool focusItem = true);
    void setSelectedItem(Item* item, bool focusItem = true);
    void deselectItems();

    void setOfferCapture(bool state);
    void setTriggerOnMouseUp(bool state);
    void setMultipleSelection(bool state);

    obs::signal<void(Item*)> ItemChange;
    obs::signal<void(Item*)> RightClick;

  protected:
    virtual void onItemChange(Item* item);
    virtual void onRightClick(Item* item);
    virtual void onSelectItem(Item* item, bool focusItem, ui::Message* msg);

  private:
    bool m_offerCapture;
    bool m_triggerOnMouseUp;
    bool m_multipleSelection;
  };

} // namespace app

#endif
