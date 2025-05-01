// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/layout_selector.h"

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/match_words.h"
#include "app/pref/preferences.h"
#include "app/ui/main_window.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "fmt/printf.h"
#include "ui/alert.h"
#include "ui/app_state.h"
#include "ui/entry.h"
#include "ui/label.h"
#include "ui/listitem.h"
#include "ui/tooltips.h"
#include "ui/window.h"

#include "new_layout.xml.h"

#define ANI_TICKS 2

namespace app {

using namespace app::skin;
using namespace ui;

namespace {

// TODO this combobox is similar to FileSelector::CustomFileNameEntry
//      and GotoFrameCommand::TagsEntry
class LayoutsEntry final : public ComboBox {
public:
  explicit LayoutsEntry(Layouts& layouts) : m_layouts(layouts)
  {
    setEditable(true);
    getEntryWidget()->Change.connect(&LayoutsEntry::onEntryChange, this);
    fill(true);
  }

private:
  void fill(bool all)
  {
    deleteAllItems();

    const MatchWords match(getEntryWidget()->text());

    bool matchAny = false;
    for (const auto& layout : m_layouts) {
      if (layout->isDefault())
        continue; // Ignore custom defaults.

      if (match(layout->name())) {
        matchAny = true;
        break;
      }
    }
    for (const auto& layout : m_layouts) {
      if (layout->isDefault())
        continue;

      if (all || !matchAny || match(layout->name()))
        addItem(layout->name());
    }
  }

  void onEntryChange() override
  {
    closeListBox();
    fill(false);
    if (getItemCount() > 0 && !empty())
      openListBox();
  }

  Layouts& m_layouts;
};

}; // namespace

class LayoutSelector::LayoutItem final : public ListItem {
public:
  enum LayoutOption : uint8_t {
    DEFAULT,
    MIRRORED_DEFAULT,
    USER_DEFINED,
    NEW_LAYOUT,
  };

  LayoutItem(LayoutSelector* selector,
             const LayoutOption option,
             const std::string& text,
             const std::string& layoutId = "")
    : ListItem(text)
    , m_option(option)
    , m_selector(selector)
    , m_layoutId(layoutId)
  {
    auto* hbox = new HBox;
    hbox->setTransparent(true);
    addChild(hbox);

    auto* filler = new BoxFiller();
    filler->setTransparent(true);
    hbox->addChild(filler);

    if (option == USER_DEFINED ||
        ((option == DEFAULT || option == MIRRORED_DEFAULT) && !layoutId.empty())) {
      addActionButton();
    }
  }

  // Separated from the constructor so we can add it on the fly when modifying Default/Mirrored
  void addActionButton(const std::string& newLayoutId = "")
  {
    if (!newLayoutId.empty())
      m_layoutId = newLayoutId;

    ASSERT(!m_layoutId.empty());

    // TODO: Custom icons for each one would be nice here.
    m_actionButton = std::unique_ptr<IconButton>(
      new IconButton(SkinTheme::instance()->parts.iconClose()));
    m_actionButton->setSizeHint(
      gfx::Size(m_actionButton->textHeight(), m_actionButton->textHeight()));

    m_actionButton->setTransparent(true);

    m_actionButton->InitTheme.connect(
      [this] { m_actionButton->setBgColor(gfx::rgba(0, 0, 0, 0)); });

    if (m_option == USER_DEFINED) {
      m_actionConn = m_actionButton->Click.connect([this] {
        const auto alert = Alert::create(Strings::new_layout_deleting_layout());
        alert->addLabel(Strings::new_layout_deleting_layout_confirmation(text()), LEFT);
        alert->addButton(Strings::general_ok());
        alert->addButton(Strings::general_cancel());
        if (alert->show() == 1) {
          if (m_layoutId == m_selector->activeLayoutId()) {
            m_selector->setActiveLayoutId(Layout::kDefault);
            App::instance()->mainWindow()->setDefaultLayout();
          }

          m_selector->removeLayout(m_layoutId);
        }
      });
    }
    else {
      m_actionConn = m_actionButton->Click.connect([this] {
        const auto alert = Alert::create(Strings::new_layout_restoring_layout());
        alert->addLabel(
          Strings::new_layout_restoring_layout_confirmation(text().substr(0, text().size() - 1)),
          LEFT);
        alert->addButton(Strings::general_ok());
        alert->addButton(Strings::general_cancel());

        if (alert->show() == 1) {
          if (m_layoutId == Layout::kDefault) {
            App::instance()->mainWindow()->setDefaultLayout();
          }
          else {
            App::instance()->mainWindow()->setMirroredDefaultLayout();
          }

          m_selector->setActiveLayoutId(m_layoutId);
          m_selector->removeLayout(m_layoutId);
        }
      });
    }

    children()[0]->addChild(m_actionButton.get());
  }

  std::string_view getLayoutId() const { return m_layoutId; }

  void selectImmediately() const
  {
    MainWindow* win = App::instance()->mainWindow();

    switch (m_option) {
      case DEFAULT: {
        if (const auto& defaultLayout = win->layoutSelector()->m_layouts.getById(
              Layout::kDefault)) {
          win->loadUserLayout(defaultLayout.get());
        }
        else {
          win->setDefaultLayout();
        }

        m_selector->setActiveLayoutId(Layout::kDefault);
      } break;
      case MIRRORED_DEFAULT: {
        if (const auto& mirroredLayout = win->layoutSelector()->m_layouts.getById(
              Layout::kMirroredDefault)) {
          win->loadUserLayout(mirroredLayout.get());
        }
        else {
          win->setMirroredDefaultLayout();
        }

        m_selector->setActiveLayoutId(Layout::kMirroredDefault);
      } break;
      case USER_DEFINED: {
        const auto selectedLayout = m_selector->m_layouts.getById(m_layoutId);
        ASSERT(!m_layoutId.empty());
        ASSERT(selectedLayout);
        m_selector->setActiveLayoutId(m_layoutId);
        win->loadUserLayout(selectedLayout.get());
      } break;
    }
  }

  void selectAfterClose() const
  {
    if (m_option != NEW_LAYOUT)
      return;

    //
    // Adding a NEW_LAYOUT
    //
    MainWindow* win = App::instance()->mainWindow();
    gen::NewLayout window;

    if (m_selector->m_layouts.size() > 0)
      window.base()->addItem(new SeparatorInView());

    // Sort the layouts by putting the defaults first, in case the user made a custom new one before
    // modifying a default.
    constexpr struct {
      bool operator()(LayoutPtr& a, LayoutPtr& b) const { return a->isDefault(); }
    } customDefaultSort;
    std::sort(m_selector->m_layouts.begin(), m_selector->m_layouts.end(), customDefaultSort);

    for (const auto& layout : m_selector->m_layouts) {
      ListItem* item;
      if (layout->isDefault()) {
        item = new ListItem(Strings::new_layout_modified(
          layout->id() == Layout::kDefault ? Strings::main_window_default_layout() :
                                             Strings::main_window_mirrored_default_layout()));
      }
      else {
        item = new ListItem(layout->name());
      }

      item->setValue(layout->id());
      window.base()->addItem(item);

      if (m_selector->m_activeLayoutId == layout->id())
        window.base()->setSelectedItemIndex(window.base()->getItemCount() - 1);
    }

    window.name()->Change.connect([&] {
      bool valid = Layout::isValidName(window.name()->text()) &&
                   m_selector->m_layouts.getById(window.name()->text()) == nullptr;
      window.ok()->setEnabled(valid);
    });

    window.openWindowInForeground();

    if (window.closer() == window.ok()) {
      if (window.base()->getValue() == Layout::kDefaultOriginal)
        win->setDefaultLayout();
      else if (window.base()->getValue() == Layout::kMirroredDefaultOriginal)
        win->setMirroredDefaultLayout();
      else {
        const auto baseLayout = m_selector->m_layouts.getById(window.base()->getValue());
        ASSERT(baseLayout);
        win->loadUserLayout(baseLayout.get());
      }

      const auto layout =
        Layout::MakeFromDock(window.name()->text(), window.name()->text(), win->customizableDock());

      m_selector->addLayout(layout);
      m_selector->m_layouts.saveUserLayouts();
      m_selector->setActiveLayoutId(layout->id());
      win->loadUserLayout(layout.get());
    }
    else {
      // Ensure we go back to having the layout we were at selected.
      m_selector->populateComboBox();
    }
  }

private:
  LayoutOption m_option;
  LayoutSelector* m_selector;
  std::string m_layoutId;
  std::unique_ptr<IconButton> m_actionButton;
  obs::scoped_connection m_actionConn;
};

void LayoutSelector::LayoutComboBox::onChange()
{
  ComboBox::onChange();
  if (auto* item = dynamic_cast<LayoutItem*>(getSelectedItem())) {
    item->selectImmediately();
    m_selected = item;
  }
}

void LayoutSelector::LayoutComboBox::onCloseListBox()
{
  ComboBox::onCloseListBox();
  if (m_selected) {
    m_selected->selectAfterClose();
    m_selected = nullptr;
  }
}

LayoutSelector::LayoutSelector(TooltipManager* tooltipManager)
  : m_button(SkinTheme::instance()->parts.iconUserData())
{
  setActiveLayoutId(Preferences::instance().general.workspaceLayout());

  m_button.Click.connect([this]() { switchSelector(); });

  m_comboBox.setVisible(false);

  m_top.setExpansive(true);
  addChild(&m_top);
  addChild(&m_center);
  addChild(&m_bottom);
  m_center.addChild(&m_comboBox);
  m_center.addChild(&m_button);

  setupTooltips(tooltipManager);
  initTheme();
}

LayoutSelector::~LayoutSelector()
{
  Preferences::instance().general.workspaceLayout(m_activeLayoutId);

  if (!is_app_state_closing())
    stopAnimation();
}

LayoutPtr LayoutSelector::activeLayout() const
{
  return m_layouts.getById(m_activeLayoutId);
}

void LayoutSelector::addLayout(const LayoutPtr& layout)
{
  m_layouts.addLayout(layout);

  // HACK: Because this function is called from inside a LayoutItem, clearing the combobox items
  // will crash.
  // TODO: Is there a better way to do this?
  auto* msg = new CallbackMessage([this] { populateComboBox(); });
  msg->setRecipient(this);
  manager()->enqueueMessage(msg);
}

void LayoutSelector::removeLayout(const LayoutPtr& layout)
{
  m_layouts.removeLayout(layout);
  m_layouts.saveUserLayouts();

  // TODO: See addLayout
  auto* msg = new CallbackMessage([this] { populateComboBox(); });
  msg->setRecipient(this);
  manager()->enqueueMessage(msg);
}

void LayoutSelector::removeLayout(const std::string& layoutId)
{
  auto layout = m_layouts.getById(layoutId);
  ASSERT(layout);
  removeLayout(layout);
}

void LayoutSelector::updateActiveLayout(const LayoutPtr& newLayout)
{
  bool added = m_layouts.addLayout(newLayout);
  setActiveLayoutId(newLayout->id());
  m_layouts.saveUserLayouts();

  if (added && newLayout->isDefault()) {
    // Mark it with an asterisk if we're editing a default layout.
    populateComboBox();
  }
}

void LayoutSelector::onInitTheme(ui::InitThemeEvent& ev)
{
  VBox::onInitTheme(ev);

  noBorderNoChildSpacing();
  m_comboBox.noBorderNoChildSpacing();
  m_button.noBorderNoChildSpacing();

  auto* theme = SkinTheme::get(this);
  m_bottom.setStyle(theme->styles.tabBottom());
  auto hint = theme->calcSizeHint(&m_bottom, m_bottom.style());
  m_bottom.setMinSize(hint);
}

void LayoutSelector::onAnimationFrame()
{
  switch (animation()) {
    case ANI_NONE:       break;
    case ANI_EXPANDING:
    case ANI_COLLAPSING: {
      const double t = animationTime();
      m_comboBox.setSizeHint(gfx::Size(int(inbetween(m_startSize.w, m_endSize.w, t)),
                                       int(inbetween(m_startSize.h, m_endSize.h, t))));
      break;
    }
  }

  if (auto* win = window())
    win->layout();
}

void LayoutSelector::onAnimationStop(int animation)
{
  switch (animation) {
    case ANI_EXPANDING:
      m_comboBox.setSizeHint(m_endSize);
      if (m_switchComboBoxAfterAni) {
        m_switchComboBoxAfterAni = false;
        m_comboBox.openListBox();
      }
      break;
    case ANI_COLLAPSING:
      m_comboBox.setVisible(false);
      m_comboBox.setSizeHint(m_endSize);
      if (m_switchComboBoxAfterAni) {
        m_switchComboBoxAfterAni = false;
        m_comboBox.closeListBox();
      }
      break;
  }

  if (auto* win = window())
    win->layout();
}

void LayoutSelector::switchSelector()
{
  bool expand;
  if (!m_comboBox.isVisible()) {
    expand = true;

    // Create the combobox for first time
    if (m_comboBox.getItemCount() == 0) {
      populateComboBox();
    }

    m_comboBox.setVisible(true);
    m_comboBox.resetSizeHint();
    m_startSize = gfx::Size(0, 0);
    m_endSize = m_comboBox.sizeHint();
  }
  else {
    expand = false;
    m_startSize = m_comboBox.bounds().size();
    m_endSize = gfx::Size(0, 0);
  }

  if (auto* item = getItemByLayoutId(m_activeLayoutId))
    m_comboBox.setSelectedItem(item);

  m_comboBox.setSizeHint(m_startSize);
  startAnimation((expand ? ANI_EXPANDING : ANI_COLLAPSING), ANI_TICKS);

  MainWindow* win = App::instance()->mainWindow();
  win->setCustomizeDock(expand);
}

void LayoutSelector::switchSelectorFromCommand()
{
  m_switchComboBoxAfterAni = true;
  switchSelector();
}

bool LayoutSelector::isSelectorVisible() const
{
  return (m_comboBox.isVisible());
}

void LayoutSelector::setupTooltips(TooltipManager* tooltipManager)
{
  tooltipManager->addTooltipFor(&m_button, Strings::main_window_layout(), TOP);
}

void LayoutSelector::populateComboBox()
{
  m_comboBox.deleteAllItems();

  m_comboBox.addItem(new SeparatorInView(Strings::main_window_layout(), HORIZONTAL));
  m_comboBox.addItem(
    new LayoutItem(this, LayoutItem::DEFAULT, Strings::main_window_default_layout()));
  m_comboBox.addItem(new LayoutItem(this,
                                    LayoutItem::MIRRORED_DEFAULT,
                                    Strings::main_window_mirrored_default_layout()));
  m_comboBox.addItem(new SeparatorInView(Strings::main_window_user_layouts(), HORIZONTAL));
  for (const auto& layout : m_layouts) {
    LayoutItem* item;
    if (layout->isDefault()) {
      item = dynamic_cast<LayoutItem*>(
        m_comboBox.getItem(layout->id() == Layout::kDefault ? 1 : 2));
      // Indicate we've modified this with an asterisk.
      item->setText(item->text() + "*");
      item->addActionButton(layout->id());
    }
    else {
      item = new LayoutItem(this, LayoutItem::USER_DEFINED, layout->name(), layout->id());
      m_comboBox.addItem(item);
    }

    if (layout->id() == m_activeLayoutId)
      m_comboBox.setSelectedItem(item);
  }
  m_comboBox.addItem(
    new LayoutItem(this, LayoutItem::NEW_LAYOUT, Strings::main_window_new_layout(), ""));

  if (m_activeLayoutId == Layout::kDefault)
    m_comboBox.setSelectedItemIndex(1);
  if (m_activeLayoutId == Layout::kMirroredDefault)
    m_comboBox.setSelectedItemIndex(2);

  m_comboBox.getEntryWidget()->deselectText();
}

LayoutSelector::LayoutItem* LayoutSelector::getItemByLayoutId(const std::string& id)
{
  for (auto* child : m_comboBox) {
    if (auto* item = dynamic_cast<LayoutItem*>(child)) {
      if (item->getLayoutId() == id)
        return item;
    }
  }

  return nullptr;
}

} // namespace app
