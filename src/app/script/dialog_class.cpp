// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/file_selector.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/ui/color_button.h"
#include "app/ui/expr_entry.h"
#include "app/ui/filename_field.h"
#include "base/bind.h"
#include "base/paths.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/entry.h"
#include "ui/grid.h"
#include "ui/label.h"
#include "ui/separator.h"
#include "ui/slider.h"
#include "ui/window.h"

#include <map>
#include <string>

namespace app {
namespace script {

#ifdef ENABLE_UI

using namespace ui;

namespace {

struct Dialog {
  ui::Window window;
  ui::VBox vbox;
  ui::Grid grid;
  ui::HBox* hbox = nullptr;
  std::map<std::string, ui::Widget*> dataWidgets;
  int currentRadioGroup = 0;

  // Used to create a new row when a different kind of widget is added
  // in the dialog.
  ui::WidgetType lastWidgetType = ui::kGenericWidget;

  // Used to keep a reference to the last onclick button pressed, so
  // then Dialog.data returns true for the button that closed the
  // dialog.
  ui::Widget* lastButton = nullptr;

  // Reference used to keep the dialog alive (so it's not garbage
  // collected) when it's visible.
  int showRef = LUA_REFNIL;
  lua_State* L = nullptr;

  // A reference to a table with pointers to functions for events
  // (e.g. onclick callbacks).
  int callbacksTableRef = LUA_REFNIL;

  Dialog()
    : window(ui::Window::WithTitleBar, "Script"),
      grid(2, false) {
    window.addChild(&grid);
    window.Close.connect([this](ui::CloseEvent&){ unrefShow(); });
  }

  void refShow(lua_State* L) {
    if (showRef == LUA_REFNIL) {
      this->L = L;
      lua_pushvalue(L, 1);
      showRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
  }

  void unrefShow() {
    if (showRef != LUA_REFNIL) {
      luaL_unref(this->L, LUA_REGISTRYINDEX, showRef);
      showRef = LUA_REFNIL;
      L = nullptr;
    }
  }
};

template<typename Signal,
         typename Callback>
void Dialog_connect_signal(lua_State* L,
                           Signal& signal,
                           Callback callback)
{
  auto dlg = get_obj<Dialog>(L, 1);

  // Add a reference to the onclick function
  const int ref = dlg->callbacksTableRef;
  lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
  lua_len(L, -1);
  const int n = 1+lua_tointegerx(L, -1, nullptr);
  lua_pop(L, 1);
  lua_pushvalue(L, -2);     // Copy the function in onclick
  lua_seti(L, -2, n);       // Put the copy of the function in the callbacksTableRef

  signal.connect(
    base::Bind<void>([=]() {
      callback();
      try {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_geti(L, -1, n);

        if (lua_isfunction(L, -1)) {
          if (lua_pcall(L, 0, 0, 0)) {
            if (const char* s = lua_tostring(L, -1))
              App::instance()
                ->scriptEngine()
                ->consolePrint(s);
          }
        }
        else
          lua_pop(L, 1);
        lua_pop(L, 1);        // Pop table from the registry
      }
      catch (const std::exception& ex) {
        // This is used to catch unhandled exception or for
        // example, std::runtime_error exceptions when a Tx() is
        // created without an active sprite.
        App::instance()
          ->scriptEngine()
          ->consolePrint(ex.what());
      }
    }));
}

int Dialog_new(lua_State* L)
{
  auto dlg = push_new<Dialog>(L);
  if (lua_isstring(L, 1))
    dlg->window.setText(lua_tostring(L, 1));

  lua_newtable(L);
#if 0 // Make values in callbacksTableRef table weak. (Weak references
      // are lost for onclick= events with local functions when we use
      // showInBackground() and we left the script scope)
  {
    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
  }
#endif
  dlg->callbacksTableRef = luaL_ref(L, LUA_REGISTRYINDEX);
  return 1;
}

int Dialog_gc(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  luaL_unref(L, LUA_REGISTRYINDEX, dlg->callbacksTableRef);
  dlg->~Dialog();
  return 0;
}

int Dialog_show(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  dlg->refShow(L);

  bool wait = true;
  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "wait");
    if (type == LUA_TBOOLEAN)
      wait = lua_toboolean(L, -1);
    lua_pop(L, 1);
  }

  if (wait)
    dlg->window.openWindowInForeground();
  else
    dlg->window.openWindow();

  lua_pushvalue(L, 1);
  return 1;
}

int Dialog_close(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  dlg->window.closeWindow(nullptr);
  lua_pushvalue(L, 1);
  return 1;
}

int Dialog_add_widget(lua_State* L, Widget* widget)
{
  auto dlg = get_obj<Dialog>(L, 1);
  const char* label = nullptr;

  // This is to separate different kind of widgets without label in
  // different rows.
  if (dlg->lastWidgetType != widget->type()) {
    dlg->lastWidgetType = widget->type();
    dlg->hbox = nullptr;
  }

  if (lua_istable(L, 2)) {
    // Widget ID (used to fill the Dialog_get_data table then)
    int type = lua_getfield(L, 2, "id");
    if (type == LUA_TSTRING) {
      if (auto id = lua_tostring(L, -1)) {
        widget->setId(id);
        dlg->dataWidgets[id] = widget;
      }
    }
    lua_pop(L, 1);

    // Label
    type = lua_getfield(L, 2, "label");
    if (type == LUA_TSTRING)
      label = lua_tostring(L, -1);
    lua_pop(L, 1);

    // Focus magnet
    type = lua_getfield(L, 2, "focus");
    if (type != LUA_TNONE && lua_toboolean(L, -1))
      widget->setFocusMagnet(true);
    lua_pop(L, 1);
  }

  if (label || !dlg->hbox) {
    if (label)
      dlg->grid.addChildInCell(new ui::Label(label), 1, 1, ui::LEFT | ui::TOP);
    else
      dlg->grid.addChildInCell(new ui::HBox, 1, 1, ui::LEFT | ui::TOP);

    auto hbox = new ui::HBox;
    if (widget->type() == ui::kButtonWidget)
      hbox->enableFlags(ui::HOMOGENEOUS);
    dlg->grid.addChildInCell(hbox, 1, 1, ui::HORIZONTAL | ui::TOP);
    dlg->hbox = hbox;
  }

  widget->setExpansive(true);
  dlg->hbox->addChild(widget);

  lua_pushvalue(L, 1);
  return 1;
}

int Dialog_newrow(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  dlg->hbox = nullptr;
  lua_pushvalue(L, 1);
  return 1;
}

int Dialog_separator(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);

  std::string text;
  if (lua_isstring(L, 2)) {
    if (auto p = lua_tostring(L, 2))
      text = p;
  }
  else if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "text");
    if (type == LUA_TSTRING) {
      if (auto p = lua_tostring(L, -1))
        text = p;
    }
    lua_pop(L, 1);
  }

  auto widget = new ui::Separator(text, ui::HORIZONTAL);
  dlg->grid.addChildInCell(widget, 2, 1, ui::HORIZONTAL | ui::TOP);
  dlg->hbox = nullptr;

  lua_pushvalue(L, 1);
  return 1;
}

int Dialog_label(lua_State* L)
{
  std::string text;
  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "text");
    if (type == LUA_TSTRING) {
      if (auto p = lua_tostring(L, -1))
        text = p;
    }
    lua_pop(L, 1);
  }

  auto widget = new ui::Label(text.c_str());
  return Dialog_add_widget(L, widget);
}

template<typename T>
int Dialog_button_base(lua_State* L, T** outputWidget = nullptr)
{
  std::string text;
  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "text");
    if (type == LUA_TSTRING) {
      if (auto p = lua_tostring(L, -1))
        text = p;
    }
    lua_pop(L, 1);
  }

  auto widget = new T(text.c_str());
  if (outputWidget)
    *outputWidget = widget;

  widget->processMnemonicFromText();

  bool closeWindowByDefault = (widget->type() == ui::kButtonWidget);

  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "selected");
    if (type != LUA_TNONE)
      widget->setSelected(lua_toboolean(L, -1));
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "onclick");
    if (type == LUA_TFUNCTION) {
      auto dlg = get_obj<Dialog>(L, 1);
      Dialog_connect_signal(L, widget->Click,
                            [dlg, widget]{
                              dlg->lastButton = widget;
                            });
      closeWindowByDefault = false;
    }
    lua_pop(L, 1);
  }

  if (closeWindowByDefault)
    widget->Click.connect([widget](ui::Event&){ widget->closeWindow(); });

  return Dialog_add_widget(L, widget);
}

int Dialog_button(lua_State* L)
{
  return Dialog_button_base<ui::Button>(L);
}

int Dialog_check(lua_State* L)
{
  return Dialog_button_base<ui::CheckBox>(L);
}

int Dialog_radio(lua_State* L)
{
  ui::RadioButton* radio = nullptr;
  const int res = Dialog_button_base<ui::RadioButton>(L, &radio);
  if (radio) {
    auto dlg = get_obj<Dialog>(L, 1);
    bool hasLabelField = false;

    if (lua_istable(L, 2)) {
      int type = lua_getfield(L, 2, "label");
      if (type == LUA_TSTRING)
        hasLabelField = true;
      lua_pop(L, 1);
    }

    if (dlg->currentRadioGroup == 0 ||
        hasLabelField) {
      ++dlg->currentRadioGroup;
    }

    radio->setRadioGroup(dlg->currentRadioGroup);
  }
  return res;
}

int Dialog_entry(lua_State* L)
{
  std::string text;
  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "text");
    if (type == LUA_TSTRING) {
      if (auto p = lua_tostring(L, -1))
        text = p;
    }
    lua_pop(L, 1);
  }

  auto widget = new ui::Entry(4096, text.c_str());
  return Dialog_add_widget(L, widget);
}

int Dialog_number(lua_State* L)
{
  auto widget = new ExprEntry;

  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "text");
    if (type == LUA_TSTRING) {
      if (auto p = lua_tostring(L, -1))
        widget->setText(p);
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "decimals");
    if (type != LUA_TNONE &&
        type != LUA_TNIL) {
      widget->setDecimals(lua_tointegerx(L, -1, nullptr));
    }
    lua_pop(L, 1);
  }

  return Dialog_add_widget(L, widget);
}

int Dialog_slider(lua_State* L)
{
  int min = 0;
  int max = 100;
  int value = 100;

  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "min");
    if (type != LUA_TNONE) {
      min = lua_tointegerx(L, -1, nullptr);
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "max");
    if (type != LUA_TNONE) {
      max = lua_tointegerx(L, -1, nullptr);
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "value");
    if (type != LUA_TNONE) {
      value = lua_tointegerx(L, -1, nullptr);
    }
    lua_pop(L, 1);
  }

  auto widget = new ui::Slider(min, max, value);
  return Dialog_add_widget(L, widget);
}

int Dialog_combobox(lua_State* L)
{
  auto widget = new ui::ComboBox;

  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "options");
    if (type == LUA_TTABLE) {
      lua_pushnil(L);
      while (lua_next(L, -2) != 0) {
        if (auto p = lua_tostring(L, -1))
          widget->addItem(p);
        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "option");
    if (type == LUA_TSTRING) {
      if (auto p = lua_tostring(L, -1)) {
        int index = widget->findItemIndex(p);
        if (index >= 0)
          widget->setSelectedItemIndex(index);
      }
    }
    lua_pop(L, 1);
  }

  return Dialog_add_widget(L, widget);
}

int Dialog_color(lua_State* L)
{
  app::Color color;
  if (lua_istable(L, 2)) {
    lua_getfield(L, 2, "color");
    color = convert_args_into_color(L, -1);
    lua_pop(L, 1);
  }

  auto widget = new ColorButton(color,
                                app_get_current_pixel_format(),
                                ColorButtonOptions());
  return Dialog_add_widget(L, widget);
}

int Dialog_file(lua_State* L)
{
  std::string title = "Open File";
  std::string fn;
  base::paths exts;
  auto dlgType = FileSelectorType::Open;
  auto fnFieldType = FilenameField::ButtonOnly;

  if (lua_istable(L, 2)) {
    lua_getfield(L, 2, "filename");
    if (auto p = lua_tostring(L, -1))
      fn = p;
    lua_pop(L, 1);

    int type = lua_getfield(L, 2, "save");
    if (type == LUA_TBOOLEAN) {
      dlgType = FileSelectorType::Save;
      title = "Save File";
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "title");
    if (type == LUA_TSTRING)
      title = lua_tostring(L, -1);
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "entry");
    if (type == LUA_TBOOLEAN) {
      fnFieldType = FilenameField::EntryAndButton;
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "filetypes");
    if (type == LUA_TTABLE) {
      lua_pushnil(L);
      while (lua_next(L, -2) != 0) {
        if (auto p = lua_tostring(L, -1))
          exts.push_back(p);
        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);
  }

  auto widget = new FilenameField(fnFieldType, fn);

  if (lua_istable(L, 2)) {
    int type = lua_getfield(L, 2, "onchange");
    if (type == LUA_TFUNCTION) {
      Dialog_connect_signal(L, widget->Change, []{ });
    }
    lua_pop(L, 1);
  }

  widget->SelectFile.connect(
    [=]() -> std::string {
      base::paths newfilename;
      if (app::show_file_selector(
            title, widget->filename(), exts,
            dlgType,
            newfilename))
        return newfilename.front();
      else
        return widget->filename();
    });
  return Dialog_add_widget(L, widget);
}

int Dialog_get_data(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  lua_newtable(L);
  for (const auto& kv : dlg->dataWidgets) {
    const ui::Widget* widget = kv.second;
    switch (widget->type()) {
      case ui::kButtonWidget:
      case ui::kCheckWidget:
      case ui::kRadioWidget:
        lua_pushboolean(L, widget->isSelected() ||
                           dlg->window.closer() == widget ||
                           dlg->lastButton == widget);
        break;
      case ui::kEntryWidget:
        if (auto expr = dynamic_cast<const ExprEntry*>(widget)) {
          if (expr->decimals() == 0)
            lua_pushinteger(L, widget->textInt());
          else
            lua_pushnumber(L, widget->textDouble());
        }
        else {
          lua_pushstring(L, widget->text().c_str());
        }
        break;
      case ui::kLabelWidget:
        lua_pushstring(L, widget->text().c_str());
        break;
      case ui::kSliderWidget:
        if (auto slider = dynamic_cast<const ui::Slider*>(widget)) {
          lua_pushinteger(L, slider->getValue());
        }
        break;
      case ui::kComboBoxWidget:
        if (auto combobox = dynamic_cast<const ui::ComboBox*>(widget)) {
          if (auto sel = combobox->getSelectedItem())
            lua_pushstring(L, sel->text().c_str());
          else
            lua_pushnil(L);
        }
        break;
      default:
        if (auto colorButton = dynamic_cast<const ColorButton*>(widget))
          push_obj<app::Color>(L, colorButton->getColor());
        else if (auto filenameField = dynamic_cast<const FilenameField*>(widget))
          lua_pushstring(L, filenameField->filename().c_str());
        else
          lua_pushnil(L);
        break;
    }
    lua_setfield(L, -2, kv.first.c_str());
  }
  return 1;
}

int Dialog_set_data(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  if (!lua_istable(L, 2))
    return 0;
  for (const auto& kv : dlg->dataWidgets) {
    lua_getfield(L, 2, kv.first.c_str());

    ui::Widget* widget = kv.second;
    switch (widget->type()) {
      case ui::kButtonWidget:
      case ui::kCheckWidget:
      case ui::kRadioWidget:
        widget->setSelected(lua_toboolean(L, -1));
        break;
      case ui::kEntryWidget:
        if (auto expr = dynamic_cast<ExprEntry*>(widget)) {
          if (expr->decimals() == 0)
            expr->setTextf("%d", lua_tointeger(L, -1));
          else
            expr->setTextf("%.*g", expr->decimals(), lua_tonumber(L, -1));
        }
        else if (auto p = lua_tostring(L, -1)) {
          widget->setText(p);
        }
        break;
      case ui::kLabelWidget:
        if (auto p = lua_tostring(L, -1)) {
          widget->setText(p);
        }
        break;
      case ui::kSliderWidget:
        if (auto slider = dynamic_cast<ui::Slider*>(widget)) {
          slider->setValue(lua_tointeger(L, -1));
        }
        break;
      case ui::kComboBoxWidget:
        if (auto combobox = dynamic_cast<ui::ComboBox*>(widget)) {
          if (auto p = lua_tostring(L, -1)) {
            int index = combobox->findItemIndex(p);
            if (index >= 0)
              combobox->setSelectedItemIndex(index);
          }
        }
        break;
      default:
        if (auto colorButton = dynamic_cast<ColorButton*>(widget))
          colorButton->setColor(convert_args_into_color(L, -1));
        else if (auto filenameField = dynamic_cast<FilenameField*>(widget)) {
          if (auto p = lua_tostring(L, -1))
            filenameField->setFilename(p);
        }
        break;
    }

    lua_pop(L, 1);
  }
  return 1;
}

int Dialog_get_bounds(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  if (!dlg->window.isVisible())
    dlg->window.remapWindow();
  push_new<gfx::Rect>(L, dlg->window.bounds());
  return 1;
}

int Dialog_set_bounds(lua_State* L)
{
  auto dlg = get_obj<Dialog>(L, 1);
  const auto rc = get_obj<gfx::Rect>(L, 2);
  dlg->window.setBounds(*rc);
  return 0;
}

const luaL_Reg Dialog_methods[] = {
  { "__gc", Dialog_gc },
  { "show", Dialog_show },
  { "close", Dialog_close },
  { "newrow", Dialog_newrow },
  { "separator", Dialog_separator },
  { "label", Dialog_label },
  { "button", Dialog_button },
  { "check", Dialog_check },
  { "radio", Dialog_radio },
  { "entry", Dialog_entry },
  { "number", Dialog_number },
  { "slider", Dialog_slider },
  { "combobox", Dialog_combobox },
  { "color", Dialog_color },
  { "file", Dialog_file },
  { nullptr, nullptr }
};

const Property Dialog_properties[] = {
  { "data", Dialog_get_data, Dialog_set_data },
  { "bounds", Dialog_get_bounds, Dialog_set_bounds },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Dialog);

#endif  // ENABLE_UI

void register_dialog_class(lua_State* L)
{
#ifdef ENABLE_UI
  REG_CLASS(L, Dialog);
  REG_CLASS_NEW(L, Dialog);
  REG_CLASS_PROPERTIES(L, Dialog);
#endif
}

} // namespace script
} // namespace app
