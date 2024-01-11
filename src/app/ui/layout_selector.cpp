// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
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
#include "app/ui/button_set.h"
#include "app/ui/configure_timeline_popup.h"
#include "app/ui/main_window.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/entry.h"
#include "ui/listitem.h"
#include "ui/tooltips.h"
#include "ui/window.h"

#include "new_layout.xml.h"

#define ANI_TICKS 2

namespace app {

using namespace app::skin;
using namespace ui;

namespace {

// TODO Similar ButtonSet to the one in timeline_conf.xml
class TimelineButtons : public ButtonSet {
public:
  TimelineButtons() : ButtonSet(2)
  {
    addItem(Strings::timeline_conf_left())->processMnemonicFromText();
    addItem(Strings::timeline_conf_right())->processMnemonicFromText();
    addItem(Strings::timeline_conf_bottom(), 2)->processMnemonicFromText();

    auto& timelinePosOption = Preferences::instance().general.timelinePosition;

    setSelectedButtonFromTimelinePosition(timelinePosOption());
    m_timelinePosConn = timelinePosOption.AfterChange.connect(
      [this](gen::TimelinePosition position) { setSelectedButtonFromTimelinePosition(position); });

    InitTheme.connect([this] {
      auto theme = skin::SkinTheme::get(this);
      setStyle(theme->styles.separatorInView());
    });
    initTheme();
  }

private:
  void setSelectedButtonFromTimelinePosition(gen::TimelinePosition pos)
  {
    int selItem = 0;
    switch (pos) {
      case gen::TimelinePosition::LEFT:   selItem = 0; break;
      case gen::TimelinePosition::RIGHT:  selItem = 1; break;
      case gen::TimelinePosition::BOTTOM: selItem = 2; break;
    }
    setSelectedItem(selItem, false);
  }

  void onItemChange(Item* item) override
  {
    ButtonSet::onItemChange(item);
    ConfigureTimelinePopup::onChangeTimelinePosition(selectedItem());

    // Show the timeline
    App::instance()->mainWindow()->setTimelineVisibility(true);
  }

  obs::scoped_connection m_timelinePosConn;
};

// TODO this combobox is similar to FileSelector::CustomFileNameEntry
//      and GotoFrameCommand::TagsEntry
class LayoutsEntry : public ComboBox {
public:
  LayoutsEntry(Layouts& layouts) : m_layouts(layouts)
  {
    setEditable(true);
    getEntryWidget()->Change.connect(&LayoutsEntry::onEntryChange, this);
    fill(true);
  }

private:
  void fill(bool all)
  {
    deleteAllItems();

    MatchWords match(getEntryWidget()->text());

    bool matchAny = false;
    for (auto& layout : m_layouts) {
      if (match(layout->name())) {
        matchAny = true;
        break;
      }
    }
    for (auto& layout : m_layouts) {
      if (all || !matchAny || match(layout->name()))
        addItem(layout->name());
    }
  }

  void onEntryChange()
  {
    closeListBox();
    fill(false);
    if (getItemCount() > 0)
      openListBox();
  }

  Layouts& m_layouts;
};

}; // namespace

class LayoutSelector::LayoutItem : public ListItem {
public:
  enum LayoutOption {
    DEFAULT,
    MIRRORED_DEFAULT,
    USER_DEFINED,
    NEW_LAYOUT,
  };

  LayoutItem(LayoutSelector* selector,
             const LayoutOption option,
             const std::string& text,
             const LayoutPtr layout)
    : ListItem(text)
    , m_option(option)
    , m_selector(selector)
    , m_layout(layout)
  {
  }

  std::string getLayoutId() const
  {
    if (m_layout)
      return m_layout->id();
    else
      return std::string();
  }

  bool matchId(const std::string& id) const { return (m_layout && m_layout->matchId(id)); }

  const LayoutPtr& layout() const { return m_layout; }

  void setLayout(const LayoutPtr& layout) { m_layout = layout; }

  void selectImmediately()
  {
    MainWindow* win = App::instance()->mainWindow();

    if (m_layout)
      m_selector->m_activeLayoutId = m_layout->id();

    switch (m_option) {
      case LayoutOption::DEFAULT:          win->setDefaultLayout(); break;
      case LayoutOption::MIRRORED_DEFAULT: win->setMirroredDefaultLayout(); break;
    }
    // Even Default & Mirrored Default can have a customized layout
    // (customized default layout).
    if (m_layout)
      win->loadUserLayout(m_layout.get());
  }

  void selectAfterClose()
  {
    MainWindow* win = App::instance()->mainWindow();

    switch (m_option) {
      case LayoutOption::NEW_LAYOUT: {
        // Select the "Layout" separator (it's like selecting nothing)
        // TODO improve the ComboBox to select a real "nothing" (with
        //      a placeholder text)
        m_selector->m_comboBox.setSelectedItemIndex(0);

        gen::NewLayout window;
        LayoutsEntry name(m_selector->m_layouts);
        name.getEntryWidget()->setMaxTextLength(128);
        name.setFocusMagnet(true);
        name.setValue(Strings::new_layout_default_name(m_selector->m_layouts.size() + 1));

        window.namePlaceholder()->addChild(&name);
        window.openWindowInForeground();
        if (window.closer() == window.ok()) {
          auto layout =
            Layout::MakeFromDock(name.getValue(), name.getValue(), win->customizableDock());

          m_selector->addLayout(layout);
        }
        break;
      }
      default:
        // Do nothing
        break;
    }
  }

private:
  LayoutOption m_option;
  LayoutSelector* m_selector;
  LayoutPtr m_layout;
};

void LayoutSelector::LayoutComboBox::onChange()
{
  ComboBox::onChange();
  if (auto item = dynamic_cast<LayoutItem*>(getSelectedItem())) {
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
  m_activeLayoutId = Preferences::instance().general.workspaceLayout();

  m_button.Click.connect([this]() { switchSelector(); });

  m_comboBox.setVisible(false);

  addChild(&m_comboBox);
  addChild(&m_button);

  setupTooltips(tooltipManager);

  InitTheme.connect([this] {
    noBorderNoChildSpacing();
    m_comboBox.noBorderNoChildSpacing();
    m_button.noBorderNoChildSpacing();
  });
  initTheme();
}

LayoutSelector::~LayoutSelector()
{
  Preferences::instance().general.workspaceLayout(m_activeLayoutId);

  stopAnimation();
}

LayoutPtr LayoutSelector::activeLayout()
{
  return m_layouts.getById(m_activeLayoutId);
}

void LayoutSelector::addLayout(const LayoutPtr& layout)
{
  bool added = m_layouts.addLayout(layout);
  if (added) {
    auto item = new LayoutItem(this, LayoutItem::USER_DEFINED, layout->name(), layout);
    m_comboBox.insertItem(m_comboBox.getItemCount() - 1, // Above the "New Layout" item
                          item);

    m_comboBox.setSelectedItem(item);
  }
  else {
    for (auto item : m_comboBox) {
      if (auto layoutItem = dynamic_cast<LayoutItem*>(item)) {
        if (layoutItem->layout() && layoutItem->layout()->name() == layout->name()) {
          layoutItem->setLayout(layout);
          m_comboBox.setSelectedItem(item);
          break;
        }
      }
    }
  }
}

void LayoutSelector::updateActiveLayout(const LayoutPtr& newLayout)
{
  bool result = m_layouts.addLayout(newLayout);

  // It means that the layout wasn't added, but replaced, when we
  // update a layout it must be existent in the m_layouts collection.
  ASSERT(result == false);
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

  if (auto win = window())
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

  if (auto win = window())
    win->layout();
}

void LayoutSelector::switchSelector()
{
  bool expand;
  if (!m_comboBox.isVisible()) {
    expand = true;

    // Create the combobox for first time
    if (m_comboBox.getItemCount() == 0) {
      m_comboBox.addItem(new SeparatorInView(Strings::main_window_layout(), HORIZONTAL));
      m_comboBox.addItem(new LayoutItem(this,
                                        LayoutItem::DEFAULT,
                                        Strings::main_window_default_layout(),
                                        m_layouts.getById(Layout::kDefault)));
      m_comboBox.addItem(new LayoutItem(this,
                                        LayoutItem::MIRRORED_DEFAULT,
                                        Strings::main_window_mirrored_default_layout(),
                                        m_layouts.getById(Layout::kMirroredDefault)));
      m_comboBox.addItem(new SeparatorInView(Strings::main_window_timeline(), HORIZONTAL));
      m_comboBox.addItem(new TimelineButtons());
      m_comboBox.addItem(new SeparatorInView(Strings::main_window_user_layouts(), HORIZONTAL));
      for (const auto& layout : m_layouts) {
        m_comboBox.addItem(new LayoutItem(this, LayoutItem::USER_DEFINED, layout->name(), layout));
      }
      m_comboBox.addItem(
        new LayoutItem(this, LayoutItem::NEW_LAYOUT, Strings::main_window_new_layout(), nullptr));
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

  if (auto item = getItemByLayoutId(m_activeLayoutId))
    m_comboBox.setSelectedItem(item);

  m_comboBox.setSizeHint(m_startSize);
  startAnimation((expand ? ANI_EXPANDING : ANI_COLLAPSING), ANI_TICKS);
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

LayoutSelector::LayoutItem* LayoutSelector::getItemByLayoutId(const std::string& id)
{
  for (auto child : m_comboBox) {
    if (auto item = dynamic_cast<LayoutItem*>(child)) {
      if (item->matchId(id))
        return item;
    }
  }
  return nullptr;
}

} // namespace app
