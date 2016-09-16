// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
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
#include "ui/box.h"
#include "ui/button.h"
#include "ui/grid.h"
#include "ui/label.h"
#include "ui/separator.h"
#include "ui/slider.h"
#include "ui/theme.h"

#include <cstdarg>
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

void Alert::setProgress(double progress)
{
  ASSERT(m_progress);
  m_progress->setValue(int(MID(0.0, progress * 100.0, 100.0)));
}

AlertPtr Alert::create(const char* format, ...)
{
  char buf[4096];               // TODO warning buffer overflow
  va_list ap;

  // Process arguments
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  // Create the alert window
  std::vector<Widget*> labels;
  std::vector<Widget*> buttons;

  AlertPtr window(new Alert());
  window->processString(buf, labels, buttons);

  return window;
}

// static
int Alert::show(const char* format, ...)
{
  char buf[4096];               // TODO warning buffer overflow
  va_list ap;

  // Process arguments
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  // Create the alert window
  std::vector<Widget*> labels;
  std::vector<Widget*> buttons;

  AlertPtr window(new Alert());
  window->processString(buf, labels, buttons);

  // Open it
  window->openWindowInForeground();

  // Check the closer
  int ret = 0;
  if (Widget* closer = window->closer()) {
    for (int i=0; i<(int)buttons.size(); ++i) {
      if (closer == buttons[i]) {
        ret = i+1;
        break;
      }
    }
  }

  // And return it
  return ret;
}

void Alert::processString(char* buf, std::vector<Widget*>& labels, std::vector<Widget*>& buttons)
{
  bool title = true;
  bool label = false;
  bool separator = false;
  bool button = false;
  int align = 0;
  char *beg;
  int c, chr;

  // Process buffer
  c = 0;
  beg = buf;
  for (; ; c++) {
    if ((!buf[c]) ||
        ((buf[c] == buf[c+1]) &&
         ((buf[c] == '<') ||
          (buf[c] == '=') ||
          (buf[c] == '>') ||
          (buf[c] == '-') ||
          (buf[c] == '|')))) {
      if (title || label || separator || button) {
        chr = buf[c];
        buf[c] = 0;

        if (title) {
          setText(beg);
        }
        else if (label) {
          Label* label = new Label(beg);
          label->setAlign(align);
          labels.push_back(label);
        }
        else if (separator) {
          labels.push_back(new Separator("", HORIZONTAL));
        }
        else if (button) {
          char buttonId[256];
          Button* button_widget = new Button(beg);
          button_widget->setMinSize(gfx::Size(60*guiscale(), 0));
          buttons.push_back(button_widget);

          sprintf(buttonId, "button-%lu", buttons.size());
          button_widget->setId(buttonId);
          button_widget->Click.connect(base::Bind<void>(&Window::closeWindow, this, button_widget));
        }

        buf[c] = chr;
      }

      /* done */
      if (!buf[c])
        break;
      /* next widget */
      else {
        title = label = separator = button = false;
        beg = buf+c+2;
        align = 0;

        switch (buf[c]) {
          case '<': label=true; align=LEFT; break;
          case '=': label=true; align=CENTER; break;
          case '>': label=true; align=RIGHT; break;
          case '-': separator=true; break;
          case '|': button=true; break;
        }
        c++;
      }
    }
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

  for (std::vector<Widget*>::iterator it = labels.begin(); it != labels.end(); ++it)
    box2->addChild(*it);

  for (std::vector<Widget*>::iterator it = buttons.begin(); it != buttons.end(); ++it)
    box3->addChild(*it);

  // Default button is the last one
  if (!buttons.empty())
    buttons[buttons.size()-1]->setFocusMagnet(true);
}

} // namespace ui
