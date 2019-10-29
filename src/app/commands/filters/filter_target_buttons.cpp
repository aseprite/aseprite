// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_target_buttons.h"

#include "app/i18n/strings.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "doc/image.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <cstring>

namespace app {

using namespace app::skin;
using namespace filters;
using namespace ui;

FilterTargetButtons::FilterTargetButtons(int imgtype, bool withChannels)
  : ButtonSet(4)
  , m_target(0)
  , m_celsTarget(CelsTarget::Selected)
  , m_red(nullptr)
  , m_green(nullptr)
  , m_blue(nullptr)
  , m_alpha(nullptr)
  , m_gray(nullptr)
  , m_index(nullptr)
  , m_cels(nullptr)
{
  setMultiMode(MultiMode::Set);
  addChild(&m_tooltips);

  if (withChannels) {
    switch (imgtype) {

      case IMAGE_RGB:
      case IMAGE_INDEXED:
        m_red   = addItem("R");
        m_green = addItem("G");
        m_blue  = addItem("B");
        m_alpha = addItem("A");

        if (imgtype == IMAGE_INDEXED)
          m_index = addItem("Index", 4, 1);
        break;

      case IMAGE_GRAYSCALE:
        m_gray = addItem("K", 2, 1);
        m_alpha = addItem("A", 2, 1);
        break;
    }
  }

  // Create the button to select which cels will be modified by the
  // filter.
  m_cels = addItem(getCelsTargetText(), 4, 1);
}

void FilterTargetButtons::setTarget(const int target)
{
  m_target = target;
  selectTargetButton(m_red,   TARGET_RED_CHANNEL);
  selectTargetButton(m_green, TARGET_GREEN_CHANNEL);
  selectTargetButton(m_blue,  TARGET_BLUE_CHANNEL);
  selectTargetButton(m_alpha, TARGET_ALPHA_CHANNEL);
  selectTargetButton(m_gray,  TARGET_GRAY_CHANNEL);
  selectTargetButton(m_index, TARGET_INDEX_CHANNEL);
  updateFromTarget();
}

void FilterTargetButtons::setCelsTarget(const CelsTarget celsTarget)
{
  m_celsTarget = celsTarget;
  updateFromCelsTarget();
}

void FilterTargetButtons::selectTargetButton(Item* item, Target specificTarget)
{
  if (item)
    item->setSelected((m_target & specificTarget) == specificTarget);
}

void FilterTargetButtons::updateFromTarget()
{
  updateComponentTooltip(m_red, "Red", BOTTOM);
  updateComponentTooltip(m_green, "Green", BOTTOM);
  updateComponentTooltip(m_blue, "Blue", BOTTOM);
  updateComponentTooltip(m_gray, "Gray", BOTTOM);
  updateComponentTooltip(m_alpha, "Alpha", BOTTOM);
  updateComponentTooltip(m_index, "Index", LEFT);
}

void FilterTargetButtons::updateFromCelsTarget()
{
  m_cels->setText(getCelsTargetText());
  m_tooltips.addTooltipFor(m_cels, getCelsTargetTooltip(), LEFT);
}

void FilterTargetButtons::updateComponentTooltip(Item* item, const char* channelName, int align)
{
  if (item) {
    char buf[256];
    std::sprintf(buf, "%s %s Component",
                 (item->isSelected() ? "Modify": "Ignore"),
                 channelName);
    m_tooltips.addTooltipFor(item, buf, align);
  }
}

void FilterTargetButtons::onItemChange(Item* item)
{
  ButtonSet::onItemChange(item);

  if (m_index && item && item->isSelected()) {
    if (item == m_index) {
      m_red->setSelected(false);
      m_green->setSelected(false);
      m_blue->setSelected(false);
      m_alpha->setSelected(false);
    }
    else if (item == m_red ||
             item == m_green ||
             item == m_blue ||
             item == m_alpha) {
      m_index->setSelected(false);
    }
  }

  Target target = 0;
  if (m_red && m_red->isSelected()) target |= TARGET_RED_CHANNEL;
  if (m_green && m_green->isSelected()) target |= TARGET_GREEN_CHANNEL;
  if (m_blue && m_blue->isSelected()) target |= TARGET_BLUE_CHANNEL;
  if (m_gray && m_gray->isSelected()) target |= TARGET_GRAY_CHANNEL;
  if (m_index && m_index->isSelected()) target |= TARGET_INDEX_CHANNEL;
  if (m_alpha && m_alpha->isSelected()) target |= TARGET_ALPHA_CHANNEL;

  CelsTarget celsTarget = m_celsTarget;
  if (m_cels->isSelected()) {
    m_cels->setSelected(false);
    celsTarget =              // Switch cels target
      (m_celsTarget == CelsTarget::Selected ?
       CelsTarget::All:
       CelsTarget::Selected);
  }

  if (m_target != target ||
      m_celsTarget != celsTarget) {
    if (m_target != target) {
      m_target = target;
      updateFromTarget();
    }
    if (m_celsTarget != celsTarget) {
      m_celsTarget = celsTarget;
      updateFromCelsTarget();
    }
    TargetChange();
  }
}

std::string FilterTargetButtons::getCelsTargetText() const
{
  switch (m_celsTarget) {
    case CelsTarget::Selected: return Strings::filters_selected_cels();
    case CelsTarget::All: return Strings::filters_all_cels();
  }
  return std::string();
}

std::string FilterTargetButtons::getCelsTargetTooltip() const
{
  switch (m_celsTarget) {
    case CelsTarget::Selected: return Strings::filters_selected_cels_tooltip();
    case CelsTarget::All: return Strings::filters_all_cels_tooltip();
  }
  return std::string();
}

} // namespace app
