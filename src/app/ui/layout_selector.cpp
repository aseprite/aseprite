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
#include "ui/listitem.h"
#include "ui/tooltips.h"
#include "ui/window.h"

#define ANI_TICKS 5

namespace app {

using namespace app::skin;
using namespace ui;

namespace {

enum class LayoutId { DEFAULT, DEFAULT_MIRROR, CUSTOMIZE };

class LayoutItem : public ListItem {
public:
  LayoutItem(const LayoutId id, const std::string& text) : ListItem(text), m_id(id) {}

  void select()
  {
    MainWindow* win = App::instance()->mainWindow();

    switch (m_id) {
      case LayoutId::DEFAULT:        win->setDefaultLayout(); break;
      case LayoutId::DEFAULT_MIRROR: win->setDefaultMirrorLayout(); break;
      case LayoutId::CUSTOMIZE:
        // TODO
        break;
    }
  }

private:
  LayoutId m_id;
};

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
    timelinePosOption.AfterChange.connect(
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
};

}; // namespace

void LayoutSelector::LayoutComboBox::onChange()
{
  if (auto item = dynamic_cast<LayoutItem*>(getSelectedItem())) {
    item->select();
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
      m_comboBox.addItem(new LayoutItem(LayoutId::DEFAULT, "Default"));
      m_comboBox.addItem(new LayoutItem(LayoutId::DEFAULT_MIRROR, "Default / Mirror"));
      m_comboBox.addItem(new SeparatorInView("Timeline", HORIZONTAL));
      m_comboBox.addItem(new TimelineButtons());
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
