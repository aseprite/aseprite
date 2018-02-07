// Aseprite UI Library
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

/***********************************************************************

  Alert format:
  ------------

     "Title Text"
     "==Centre line of text"
     "--"
     "<<Left line of text"
     ">>Right line of text"
     "||My &Button||&Your Button||&He Button"

    .-----------------------------------------.
    | My Message                              |
    +=========================================+
    |           Centre line of text           |
    | --------------------------------------- |
    | Left line of text                       |
    |                      Right line of text |
    |                                         |
    | [My Button]  [Your Button] [Him Button] |
    `-----~---------~-------------~-----------'
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/alert.h"

#include "base/bind.h"
#include "base/string.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/grid.h"
#include "ui/label.h"
#include "ui/scale.h"
#include "ui/separator.h"
#include "ui/slider.h"
#include "ui/theme.h"

#include <cstdio>

namespace ui {

Alert::Alert()
  : Window(WithTitleBar)
  , m_progress(nullptr)
  , m_progressPlaceholder(nullptr)
{
  // Do nothing
}

void Alert::addProgress()
{
  ASSERT(!m_progress);
  m_progress = new Slider(0, 100, 0);
  m_progress->setReadOnly(true);
  m_progressPlaceholder->addChild(m_progress);
  m_progressPlaceholder->setVisible(true);
}

CheckBox* Alert::addCheckBox(const std::string& text)
{
  auto checkBox = new CheckBox(text);
  m_progressPlaceholder->addChild(checkBox);
  m_progressPlaceholder->setVisible(true);
  return checkBox;
}

void Alert::setProgress(double progress)
{
  ASSERT(m_progress);
  m_progress->setValue(int(MID(0.0, progress * 100.0, 100.0)));
}

// static
AlertPtr Alert::create(const std::string& _msg)
{
  std::string msg(_msg);

  // Create the alert window
  AlertPtr window(new Alert());
  window->processString(msg);
  return window;
}

// static
int Alert::show(const std::string& _msg)
{
  std::string msg(_msg);

  // Create the alert window
  AlertPtr window(new Alert());
  window->processString(msg);
  return window->show();
}

int Alert::show()
{
  // Open it
  openWindowInForeground();

  // Check the closer
  int ret = 0;
  if (Widget* closer = this->closer()) {
    for (int i=0; i<(int)m_buttons.size(); ++i) {
      if (closer == m_buttons[i]) {
        ret = i+1;
        break;
      }
    }
  }

  // And return it
  return ret;
}

void Alert::processString(std::string& buf)
{
  bool title = true;
  bool label = false;
  bool separator = false;
  bool button = false;
  int align = 0;
  int c, beg;

  // Process buffer
  c = 0;
  beg = 0;
  for (;;) {
    // Ignore characters
    if (buf[c] == '\n' ||
        buf[c] == '\r') {
      buf.erase(c, 1);
      continue;
    }

    if ((!buf[c]) ||
        ((buf[c] == buf[c+1]) &&
         ((buf[c] == '<') ||
          (buf[c] == '=') ||
          (buf[c] == '>') ||
          (buf[c] == '-') ||
          (buf[c] == '|')))) {
      if (title || label || separator || button) {
        std::string item = buf.substr(beg, c-beg);

        if (title) {
          setText(item);
        }
        else if (label) {
          Label* label = new Label(item);
          label->setAlign(align);
          m_labels.push_back(label);
        }
        else if (separator) {
          m_labels.push_back(new Separator("", HORIZONTAL));
        }
        else if (button) {
          char buttonId[256];
          Button* button_widget = new Button(item);
          button_widget->processMnemonicFromText();
          button_widget->setMinSize(gfx::Size(60*guiscale(), 0));
          m_buttons.push_back(button_widget);

          sprintf(buttonId, "button-%lu", m_buttons.size());
          button_widget->setId(buttonId);
          button_widget->Click.connect(base::Bind<void>(&Window::closeWindow, this, button_widget));
        }
      }

      // Done
      if (!buf[c])
        break;
      // Next widget
      else {
        title = label = separator = button = false;
        beg = c+2;
        align = 0;

        switch (buf[c]) {
          case '<': label=true; align=LEFT; break;
          case '=': label=true; align=CENTER; break;
          case '>': label=true; align=RIGHT; break;
          case '-': separator=true; break;
          case '|': button=true; break;
        }
        ++c;
      }
    }
    ++c;
  }

  auto box1 = new Box(VERTICAL);
  auto box2 = new Box(VERTICAL);
  auto grid = new Grid(1, false);
  auto box3 = new Box(HORIZONTAL | HOMOGENEOUS);

  // To identify by the user
  box2->setId("labels");
  box3->setId("buttons");

  // Pseudo separators (only to fill blank space)
  auto box4 = new Box(0);
  auto box5 = new Box(0);
  m_progressPlaceholder = new Box(0);

  box4->setExpansive(true);
  box5->setExpansive(true);
  box4->noBorderNoChildSpacing();
  box5->noBorderNoChildSpacing();
  m_progressPlaceholder->noBorderNoChildSpacing();

  // Setup parent <-> children relationship

  addChild(box1);

  box1->addChild(box4); // Filler
  box1->addChild(box2); // Labels
  box1->addChild(m_progressPlaceholder);
  box1->addChild(box5); // Filler
  box1->addChild(grid); // Buttons

  grid->addChildInCell(box3, 1, 1, CENTER | BOTTOM | HORIZONTAL);

  for (auto it=m_labels.begin(); it!=m_labels.end(); ++it)
    box2->addChild(*it);

  for (auto it=m_buttons.begin(); it!=m_buttons.end(); ++it)
    box3->addChild(*it);

  // Default button is the first one (Enter default option, Esc should
  // act like the last button)
  if (!m_buttons.empty())
    m_buttons[0]->setFocusMagnet(true);
}

} // namespace ui
