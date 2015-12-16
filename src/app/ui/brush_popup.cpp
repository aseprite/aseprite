// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/brush_popup.h"

#include "app/app.h"
#include "app/app_brushes.h"
#include "app/brush_slot.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/button_set.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "doc/brush.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "gfx/border.h"
#include "gfx/region.h"
#include "she/scoped_surface_lock.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/button.h"
#include "ui/link_label.h"
#include "ui/listitem.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/separator.h"

namespace app {

using namespace app::skin;
using namespace doc;
using namespace ui;

namespace {

void show_popup_menu(PopupWindow* popupWindow, Menu* popupMenu,
                     const gfx::Point& pt)
{
  // Here we make the popup window temporaly floating, so it's
  // not closed by the popup menu.
  popupWindow->makeFloating();

  popupMenu->showPopup(pt);

  // Add the menu popup region to the window popup hot region so it's
  // not closed after we close the menu.
  popupWindow->makeFixed();

  gfx::Region rgn;
  rgn.createUnion(gfx::Region(popupWindow->bounds()),
                  gfx::Region(popupMenu->bounds()));
  popupWindow->setHotRegion(rgn);
}

class SelectBrushItem : public ButtonSet::Item {
public:
  SelectBrushItem(BrushPopupDelegate* delegate, const BrushSlot& brush, int slot = -1)
    : m_delegate(delegate)
    , m_brush(brush)
    , m_slot(slot) {
    if (m_brush.hasBrush()) {
      SkinPartPtr icon(new SkinPart);
      icon->setBitmap(0, BrushPopup::createSurfaceForBrush(m_brush.brush()));
      setIcon(icon);
    }
  }

  const BrushSlot& brush() const {
    return m_brush;
  }

private:

  void onClick() override {
    if (m_slot >= 0)
      m_delegate->onSelectBrushSlot(m_slot);
    else if (m_brush.hasBrush())
      m_delegate->onSelectBrush(m_brush.brush());
  }

private:

  BrushPopupDelegate* m_delegate;
  BrushSlot m_brush;
  int m_slot;
};

class BrushShortcutItem : public ButtonSet::Item {
public:
  BrushShortcutItem(const std::string& text, int slot)
    : m_slot(slot) {
    setText(text);
  }

private:
  void onClick() override {
    Params params;
    params.set("change", "custom");
    params.set("slot", base::convert_to<std::string>(m_slot).c_str());
    Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::ChangeBrush);
    cmd->loadParams(params);
    std::string search = cmd->friendlyName();
    if (!search.empty()) {
      params.clear();
      params.set("search", search.c_str());
      cmd = CommandsModule::instance()->getCommandByName(CommandId::KeyboardShortcuts);
      ASSERT(cmd);
      if (cmd)
        UIContext::instance()->executeCommand(cmd, params);
    }
  }

  int m_slot;
};

class BrushOptionsItem : public ButtonSet::Item {
public:
  BrushOptionsItem(BrushPopup* popup, BrushPopupDelegate* delegate, int slot)
    : m_popup(popup)
    , m_delegate(delegate)
    , m_slot(slot) {
    setIcon(SkinTheme::instance()->parts.iconArrowDown(), true);
  }

private:

  void onClick() override {
    Menu menu;
    AppMenuItem save("Save Brush Here");
    AppMenuItem lockItem(m_delegate->onIsBrushSlotLocked(m_slot) ? "Unlock Brush": "Lock Brush");
    AppMenuItem deleteItem("Delete");
    AppMenuItem deleteAllItem("Delete All");
    save.Click.connect(&BrushOptionsItem::onSaveBrush, this);
    lockItem.Click.connect(&BrushOptionsItem::onLockBrush, this);
    deleteItem.Click.connect(&BrushOptionsItem::onDeleteBrush, this);
    deleteAllItem.Click.connect(&BrushOptionsItem::onDeleteAllBrushes, this);
    menu.addChild(&save);
    menu.addChild(new MenuSeparator);
    menu.addChild(&lockItem);
    menu.addChild(&deleteItem);
    menu.addChild(new MenuSeparator);
    menu.addChild(&deleteAllItem);

    show_popup_menu(m_popup, &menu,
                    gfx::Point(origin().x, origin().y+bounds().h));
  }

private:

  void onSaveBrush() {
    AppBrushes& brushes = App::instance()->brushes();
    brushes.setBrushSlot(
      m_slot, m_delegate->onCreateBrushSlotFromActivePreferences());
    brushes.lockBrushSlot(m_slot);
  }

  void onLockBrush() {
    if (m_delegate->onIsBrushSlotLocked(m_slot))
      m_delegate->onUnlockBrushSlot(m_slot);
    else
      m_delegate->onLockBrushSlot(m_slot);
  }

  void onDeleteBrush() {
    m_delegate->onDeleteBrushSlot(m_slot);
  }

  void onDeleteAllBrushes() {
    m_delegate->onDeleteAllBrushes();
  }

  BrushPopup* m_popup;
  BrushPopupDelegate* m_delegate;
  BrushRef m_brush;
  int m_slot;
};

class NewCustomBrushItem : public ButtonSet::Item {
public:
  NewCustomBrushItem(BrushPopupDelegate* delegate)
    : m_delegate(delegate) {
    setText("Save Brush");
  }

private:
  void onClick() override {
    AppBrushes& brushes = App::instance()->brushes();
    auto slot = brushes.addBrushSlot(
      m_delegate->onCreateBrushSlotFromActivePreferences());
    brushes.lockBrushSlot(slot);
  }

  BrushPopupDelegate* m_delegate;
};

class NewBrushOptionsItem : public ButtonSet::Item {
public:
  NewBrushOptionsItem() {
    setIcon(SkinTheme::instance()->parts.iconArrowDown(), true);
  }

private:
  void onClick() override {
    Menu menu;

    menu.addChild(new Separator("Parameters to Save", HORIZONTAL));

    Grid* grid = new Grid(2, false);
    ButtonSet* brushParams = new ButtonSet(3);
    ButtonSet::Item* brushType = brushParams->addItem("Type");
    ButtonSet::Item* brushSize = brushParams->addItem("Size");
    ButtonSet::Item* brushAngle = brushParams->addItem("Angle");
    brushParams->setMultipleSelection(true);

    ButtonSet* colorParams = new ButtonSet(2);
    ButtonSet::Item* fgColor = colorParams->addItem("Foreground");
    ButtonSet::Item* bgColor = colorParams->addItem("Background");
    colorParams->setMultipleSelection(true);

    ButtonSet* inkParams = new ButtonSet(2);
    ButtonSet::Item* inkType = inkParams->addItem("Type");
    ButtonSet::Item* inkOpacity = inkParams->addItem("Opacity");
    inkParams->setMultipleSelection(true);

    ButtonSet* extrasParams = new ButtonSet(2);
    ButtonSet::Item* shade = extrasParams->addItem("Shade");
    ButtonSet::Item* pixelPerfect = extrasParams->addItem("Pixel-Perfect");
    extrasParams->setMultipleSelection(true);

    grid->addChildInCell(new Label("Brush:"), 1, 1, 0);
    grid->addChildInCell(brushParams, 1, 1, 0);
    grid->addChildInCell(new Label("Color:"), 1, 1, 0);
    grid->addChildInCell(colorParams, 1, 1, 0);
    grid->addChildInCell(new Label("Ink:"), 1, 1, 0);
    grid->addChildInCell(inkParams, 1, 1, 0);
    grid->addChildInCell(new Label("Extras:"), 1, 1, 0);
    grid->addChildInCell(extrasParams, 1, 1, 0);
    menu.addChild(grid);

    // Load preferences
    auto& saveBrush = Preferences::instance().saveBrush;
    brushType->setSelected(saveBrush.brushType());
    brushSize->setSelected(saveBrush.brushSize());
    brushAngle->setSelected(saveBrush.brushAngle());
    fgColor->setSelected(saveBrush.fgColor());
    bgColor->setSelected(saveBrush.bgColor());
    inkType->setSelected(saveBrush.inkType());
    inkOpacity->setSelected(saveBrush.inkOpacity());
    shade->setSelected(saveBrush.shade());
    pixelPerfect->setSelected(saveBrush.pixelPerfect());

    show_popup_menu(static_cast<PopupWindow*>(window()), &menu,
                    gfx::Point(origin().x, origin().y+bounds().h));

    // Save preferences
    if (saveBrush.brushType() != brushType->isSelected())
      saveBrush.brushType(brushType->isSelected());
    if (saveBrush.brushSize() != brushSize->isSelected())
      saveBrush.brushSize(brushSize->isSelected());
    if (saveBrush.brushAngle() != brushAngle->isSelected())
      saveBrush.brushAngle(brushAngle->isSelected());
    if (saveBrush.fgColor() != fgColor->isSelected())
      saveBrush.fgColor(fgColor->isSelected());
    if (saveBrush.bgColor() != bgColor->isSelected())
      saveBrush.bgColor(bgColor->isSelected());
    if (saveBrush.inkType() != inkType->isSelected())
      saveBrush.inkType(inkType->isSelected());
    if (saveBrush.inkOpacity() != inkOpacity->isSelected())
      saveBrush.inkOpacity(inkOpacity->isSelected());
    if (saveBrush.shade() != shade->isSelected())
      saveBrush.shade(shade->isSelected());
    if (saveBrush.pixelPerfect() != pixelPerfect->isSelected())
      saveBrush.pixelPerfect(pixelPerfect->isSelected());
  }
};

} // anonymous namespace

BrushPopup::BrushPopup(BrushPopupDelegate* delegate)
  : PopupWindow("", ClickBehavior::CloseOnClickInOtherWindow)
  , m_tooltipManager(nullptr)
  , m_standardBrushes(3)
  , m_customBrushes(nullptr)
  , m_delegate(delegate)
{
  setAutoRemap(false);
  setBorder(gfx::Border(2)*guiscale());
  setChildSpacing(0);
  m_box.noBorderNoChildSpacing();
  m_standardBrushes.setTriggerOnMouseUp(true);

  addChild(&m_box);

  HBox* top = new HBox;
  top->addChild(&m_standardBrushes);
  top->addChild(new BoxFiller);

  m_box.addChild(top);
  m_box.addChild(new Separator("", HORIZONTAL));

  const doc::Brushes& brushes = App::instance()->brushes().getStandardBrushes();
  for (const auto& brush : brushes)
    m_standardBrushes.addItem(
      new SelectBrushItem(
        m_delegate, BrushSlot(BrushSlot::Flags::BrushType, brush)));

  m_standardBrushes.setTransparent(true);
  m_standardBrushes.setBgColor(gfx::ColorNone);

  App::instance()->brushes()
    .ItemsChange.connect(&BrushPopup::onBrushChanges, this);
}

void BrushPopup::setBrush(Brush* brush)
{
  for (auto child : m_standardBrushes.children()) {
    SelectBrushItem* item = static_cast<SelectBrushItem*>(child);

    // Same type and same image
    if (item->brush().hasBrush() &&
        item->brush().brush()->type() == brush->type() &&
        (brush->type() != kImageBrushType ||
         item->brush().brush()->image() == brush->image())) {
      m_standardBrushes.setSelectedItem(item);
      return;
    }
  }
}

void BrushPopup::regenerate(const gfx::Rect& box)
{
  auto& brushSlots = App::instance()->brushes().getBrushSlots();

  if (m_customBrushes) {
    // As BrushPopup::regenerate() can be called when a
    // "m_customBrushes" button is clicked we cannot delete
    // "m_customBrushes" right now.
    m_customBrushes->parent()->removeChild(m_customBrushes);
    m_customBrushes->deferDelete();
  }

  m_customBrushes = new ButtonSet(3);
  m_customBrushes->setTriggerOnMouseUp(true);

  auto& parts = SkinTheme::instance()->parts;
  int slot = 0;
  for (const auto& brush : brushSlots) {
    ++slot;

    // Get shortcut
    std::string shortcut;
    {
      Params params;
      params.set("change", "custom");
      params.set("slot", base::convert_to<std::string>(slot).c_str());
      Key* key = KeyboardShortcuts::instance()->command(
        CommandId::ChangeBrush, params);
      if (key && !key->accels().empty())
        shortcut = key->accels().front().toString();
    }
    m_customBrushes->addItem(new SelectBrushItem(m_delegate, brush, slot));
    m_customBrushes->addItem(new BrushShortcutItem(shortcut, slot));
    m_customBrushes->addItem(new BrushOptionsItem(this, m_delegate, slot));
  }

  m_customBrushes->addItem(new NewCustomBrushItem(m_delegate), 2, 1);
  m_customBrushes->addItem(new NewBrushOptionsItem);
  m_customBrushes->setExpansive(true);
  m_box.addChild(m_customBrushes);

  // Resize the window and change the hot region.
  setBounds(gfx::Rect(box.origin(), sizeHint()));
  setHotRegion(gfx::Region(bounds()));
}

void BrushPopup::onBrushChanges()
{
  if (isVisible()) {
    gfx::Region rgn;
    getDrawableRegion(rgn, DrawableRegionFlags(kCutTopWindows | kUseChildArea));

    regenerate(bounds());
    invalidate();

    parent()->invalidateRegion(rgn);
  }
}

// static
she::Surface* BrushPopup::createSurfaceForBrush(const BrushRef& origBrush)
{
  Image* image = nullptr;
  BrushRef brush = origBrush;
  if (brush) {
    if (brush->type() != kImageBrushType && brush->size() > 10) {
      brush.reset(new Brush(*brush));
      brush->setSize(10);
    }
    image = brush->image();
  }

  she::Surface* surface = she::instance()->createRgbaSurface(
    std::min(10, image ? image->width(): 4),
    std::min(10, image ? image->height(): 4));

  if (image) {
    Palette* palette = get_current_palette();
    if (image->pixelFormat() == IMAGE_BITMAP) {
      palette = new Palette(frame_t(0), 2);
      palette->setEntry(0, rgba(0, 0, 0, 0));
      palette->setEntry(1, rgba(0, 0, 0, 255));
    }

    convert_image_to_surface(
      image, palette, surface,
      0, 0, 0, 0, image->width(), image->height());

    if (image->pixelFormat() == IMAGE_BITMAP)
      delete palette;
  }
  else {
    she::ScopedSurfaceLock lock(surface);
    lock->clear();
  }

  return surface;
}

} // namespace app
