// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
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

#include <allegro/unicode.h>
#include <stdarg.h>
#include <stdio.h>

#include "base/bind.h"
#include "ui/ui.h"

namespace ui {

Alert::Alert()
  : Window(WithTitleBar)
{
  // Do nothing
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

  // Check the killer
  int ret = 0;
  if (Widget* killer = window->getKiller()) {
    for (int i=0; i<(int)buttons.size(); ++i) {
      if (killer == buttons[i]) {
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
  Box* box1, *box2, *box3, *box4, *box5;
  Grid* grid;
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
          labels.push_back(new Separator("", JI_HORIZONTAL));
        }
        else if (button) {
          char buttonId[256];
          Button* button_widget = new Button(beg);
          button_widget->setMinSize(gfx::Size(60*jguiscale(), 0));
          buttons.push_back(button_widget);

          usprintf(buttonId, "button-%lu", buttons.size());
          button_widget->setId(buttonId);
          button_widget->Click.connect(Bind<void>(&Window::closeWindow, this, button_widget));
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
          case '<': label=true; align=JI_LEFT; break;
          case '=': label=true; align=JI_CENTER; break;
          case '>': label=true; align=JI_RIGHT; break;
          case '-': separator=true; break;
          case '|': button=true; break;
        }
        c++;
      }
    }
  }

  box1 = new Box(JI_VERTICAL);
  box2 = new Box(JI_VERTICAL);
  grid = new Grid(1, false);
  box3 = new Box(JI_HORIZONTAL | JI_HOMOGENEOUS);

  // To identify by the user
  box2->setId("labels");
  box3->setId("buttons");

  // Pseudo separators (only to fill blank space)
  box4 = new Box(0);
  box5 = new Box(0);

  box4->setExpansive(true);
  box5->setExpansive(true);
  box4->noBorderNoChildSpacing();
  box5->noBorderNoChildSpacing();

  // Setup parent <-> children relationship

  addChild(box1);

  box1->addChild(box4); // Filler
  box1->addChild(box2); // Labels
  box1->addChild(box5); // Filler
  box1->addChild(grid); // Buttons

  grid->addChildInCell(box3, 1, 1, JI_CENTER | JI_BOTTOM);

  for (std::vector<Widget*>::iterator it = labels.begin(); it != labels.end(); ++it)
    box2->addChild(*it);

  for (std::vector<Widget*>::iterator it = buttons.begin(); it != buttons.end(); ++it)
    box3->addChild(*it);

  // Default button is the last one
  if (!buttons.empty())
    buttons[buttons.size()-1]->setFocusMagnet(true);
}

} // namespace ui
