// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/modules/gui.h"
#include "base/bind.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

class AboutCommand : public Command {
public:
  AboutCommand();
  Command* clone() const override { return new AboutCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

AboutCommand::AboutCommand()
  : Command("About",
            "About",
            CmdUIOnlyFlag)
{
}

void AboutCommand::onExecute(Context* context)
{
  base::UniquePtr<Window> window(new Window(Window::WithTitleBar, "About " PACKAGE));
  Box* box1 = new Box(VERTICAL);
  Grid* grid = new Grid(2, false);
  Label* title = new Label(PACKAGE " v" VERSION);
  Label* subtitle = new Label("Animated sprite editor & pixel art tool");
  Separator* authors_separator1 = new Separator("Authors:", HORIZONTAL | TOP);
  Separator* authors_separator2 = new Separator("", HORIZONTAL);
  Label* author1 = new LinkLabel("http://davidcapello.com/", "David Capello");
  Label* author1_desc = new Label("- Lead developer, graphics & maintainer");
  Label* author2 = new LinkLabel("http://ilkke.blogspot.com/", "Ilija Melentijevic");
  Label* author2_desc = new Label("- Default skin & graphics introduced in v0.8");
  Label* author3 = new LinkLabel(WEBSITE_CONTRIBUTORS, "Contributors");
  Box* bottom_box1 = new Box(HORIZONTAL);
  Box* bottom_box2 = new Box(HORIZONTAL);
  Box* bottom_box3 = new Box(HORIZONTAL);
  Label* copyright = new Label(COPYRIGHT);
  Label* website = new LinkLabel(WEBSITE);
  Button* close_button = new Button("&Close");

  grid->addChildInCell(title, 2, 1, 0);
  grid->addChildInCell(subtitle, 2, 1, 0);
  grid->addChildInCell(authors_separator1, 2, 1, 0);
  grid->addChildInCell(author1, 1, 1, 0);
  grid->addChildInCell(author1_desc, 1, 1, 0);
  grid->addChildInCell(author2, 1, 1, 0);
  grid->addChildInCell(author2_desc, 1, 1, 0);
  grid->addChildInCell(author3, 2, 1, 0);
  grid->addChildInCell(authors_separator2, 2, 1, 0);
  grid->addChildInCell(copyright, 2, 1, 0);
  grid->addChildInCell(website, 2, 1, 0);
  grid->addChildInCell(bottom_box1, 2, 1, 0);

  close_button->setFocusMagnet(true);

  bottom_box2->setExpansive(true);
  bottom_box3->setExpansive(true);

  bottom_box1->addChild(bottom_box2);
  bottom_box1->addChild(close_button);
  bottom_box1->addChild(bottom_box3);

  box1->addChild(grid);
  window->addChild(box1);

  close_button->setBorder(
    gfx::Border(
      close_button->border().left() + 16*guiscale(),
      close_button->border().top(),
      close_button->border().right() + 16*guiscale(),
      close_button->border().bottom()));

  close_button->Click.connect(Bind<void>(&Window::closeWindow, window.get(), close_button));

  window->openWindowInForeground();
}

Command* CommandFactory::createAboutCommand()
{
  return new AboutCommand;
}

} // namespace app
