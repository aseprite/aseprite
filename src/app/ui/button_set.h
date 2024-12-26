// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include "ui/style.h"

#include <string>

namespace app {

class ButtonSet : public ui::Grid {
public:
  class Item : public ui::Widget,
               public ui::Style::Layer::IconSurfaceProvider {
  public:
    Item();
    void setIcon(const skin::SkinPartPtr& icon);
    os::Surface* iconSurface() const override { return m_icon ? m_icon->bitmap(0) : nullptr; }
    skin::SkinPartPtr icon() const { return m_icon; }
    ButtonSet* buttonSet();

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    virtual void onClick();
    virtual void onRightClick();

  private:
    skin::SkinPartPtr m_icon;
  };

  enum class MultiMode {
    One,       // Only one button can be selected (like radio buttons)
    Set,       // Each button is a checkbox
    OneOrMore, // One click selects one button, ctrl+click multiple selections
  };

  ButtonSet(int columns);

  Item* addItem(const std::string& text, ui::Style* style);
  Item* addItem(const std::string& text, int hspan = 1, int vspan = 1, ui::Style* style = nullptr);
  Item* addItem(const skin::SkinPartPtr& icon, ui::Style* style);
  Item* addItem(const skin::SkinPartPtr& icon,
                int hspan = 1,
                int vspan = 1,
                ui::Style* style = nullptr);
  Item* addItem(Item* item, ui::Style* style);
  Item* addItem(Item* item, int hspan = 1, int vspan = 1, ui::Style* style = nullptr);
  Item* getItem(int index);
  int getItemIndex(const Item* item) const;

  int selectedItem() const;
  int countSelectedItems() const;
  Item* findSelectedItem() const;
  void setSelectedItem(int index, bool focusItem = true);
  void setSelectedItem(Item* item, bool focusItem = true);
  void deselectItems();

  void setOfferCapture(bool state);
  void setTriggerOnMouseUp(bool state);
  void setMultiMode(MultiMode mode);

  obs::signal<void(Item*)> ItemChange;
  obs::signal<void(Item*)> RightClick;

protected:
  virtual void onItemChange(Item* item);
  virtual void onRightClick(Item* item);
  virtual void onSelectItem(Item* item, bool focusItem, ui::Message* msg);

private:
  bool m_offerCapture;
  bool m_triggerOnMouseUp;
  MultiMode m_multiMode;
};

} // namespace app

#endif
