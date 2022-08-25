// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/layout_selector.h"

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/ui/button_set.h"
#include "app/ui/configure_timeline_popup.h"
#include "app/ui/main_window.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "fmt/format.h"
#include "ui/listitem.h"
#include "ui/tooltips.h"
#include "ui/window.h"

#include "new_layout.xml.h"

#define ANI_TICKS 5

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

}; // namespace

class LayoutSelector::LayoutItem : public ListItem {
public:
  enum LayoutId {
    DEFAULT,
    DEFAULT_MIRROR,
    SAVE_LAYOUT,
    USER_DEFINED,
  };

  LayoutItem(LayoutSelector* selector,
             const LayoutId id,
             const std::string& text,
             const LayoutPtr layout = nullptr)
    : ListItem(text)
    , m_selector(selector)
    , m_id(id)
    , m_layout(layout)
  {
    ASSERT((id != USER_DEFINED && layout == nullptr) || (id == USER_DEFINED && layout != nullptr));
  }

  void selectImmediately()
  {
    MainWindow* win = App::instance()->mainWindow();

    switch (m_id) {
      case LayoutId::DEFAULT:        win->setDefaultLayout(); break;
      case LayoutId::DEFAULT_MIRROR: win->setDefaultMirrorLayout(); break;
      case LayoutId::USER_DEFINED:
        ASSERT(m_layout);
        if (m_layout)
          win->loadUserLayout(m_layout.get());
        break;
      default:
        // Do nothing
        break;
    }
  }

  void selectAfterClose()
  {
    MainWindow* win = App::instance()->mainWindow();

    switch (m_id) {
      case LayoutId::SAVE_LAYOUT: {
        gen::NewLayout window;
        window.name()->setText(
          fmt::format("{} ({})", window.name()->text(), m_selector->m_layouts.size()));

        window.openWindowInForeground();
        if (window.closer() == window.ok()) {
          auto layout = std::make_shared<Layout>(window.name()->text(), win->customizableDock());

          m_selector->addLayout(std::move(layout));
        }
        break;
      }
      default:
        // Do nothing
        break;
    }
  }

private:
  LayoutId m_id;
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
  stopAnimation();
}

void LayoutSelector::addLayout(const LayoutPtr& layout)
{
  auto item = m_comboBox.addItem(
    new LayoutItem(this, LayoutItem::USER_DEFINED, layout->name(), layout));

  m_layouts.push_back(layout);

  m_comboBox.setSelectedItemIndex(item);
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
    case ANI_EXPANDING: m_comboBox.setSizeHint(m_endSize); break;
    case ANI_COLLAPSING:
      m_comboBox.setVisible(false);
      m_comboBox.setSizeHint(m_endSize);
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
      m_comboBox.addItem(new SeparatorInView("Layout", HORIZONTAL));
      m_comboBox.addItem(new LayoutItem(this, LayoutItem::DEFAULT, "Default"));
      m_comboBox.addItem(new LayoutItem(this, LayoutItem::DEFAULT_MIRROR, "Default / Mirror"));
      m_comboBox.addItem(new SeparatorInView("Timeline", HORIZONTAL));
      m_comboBox.addItem(new TimelineButtons());
      m_comboBox.addItem(new SeparatorInView("User Layouts", HORIZONTAL));
      m_comboBox.addItem(new LayoutItem(this, LayoutItem::SAVE_LAYOUT, "Save..."));
      for (const auto& layout : m_layouts) {
        m_comboBox.addItem(new LayoutItem(this, LayoutItem::USER_DEFINED, layout->name(), layout));
      }
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

  m_comboBox.setSizeHint(m_startSize);
  startAnimation((expand ? ANI_EXPANDING : ANI_COLLAPSING), ANI_TICKS);
}

void LayoutSelector::setupTooltips(TooltipManager* tooltipManager)
{
  tooltipManager->addTooltipFor(&m_button, Strings::main_window_layout(), TOP);
}

} // namespace app
