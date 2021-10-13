// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/layout_selector.h"

#include "app/app.h"
#include "app/ui/main_window.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/listitem.h"
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

}; // namespace

void LayoutSelector::LayoutComboBox::onChange()
{
  if (auto item = dynamic_cast<LayoutItem*>(getSelectedItem())) {
    item->select();
  }
}

LayoutSelector::LayoutSelector() : m_button("", "\xc3\xb7")
{
  m_button.Click.connect([this]() { switchSelector(); });

  m_comboBox.setVisible(false);

  addChild(&m_comboBox);
  addChild(&m_button);
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
      m_comboBox.setSizeHint(gfx::Size((1.0 - t) * m_startSize.w + t * m_endSize.w,
                                       (1.0 - t) * m_startSize.h + t * m_endSize.h));
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
      m_comboBox.addItem(new LayoutItem(LayoutId::DEFAULT, "Default"));
      m_comboBox.addItem(new LayoutItem(LayoutId::DEFAULT_MIRROR, "Default / Mirror"));
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

} // namespace app
