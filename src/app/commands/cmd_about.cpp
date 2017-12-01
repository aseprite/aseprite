// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
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
  : Command(CommandId::About(), CmdUIOnlyFlag)
{
}

void AboutCommand::onExecute(Context* context)
{
  base::UniquePtr<Window> window(new Window(Window::WithTitleBar, "About " PACKAGE));
  auto box1 = new Box(VERTICAL);
  auto grid = new Grid(2, false);
  auto title = new Label(PACKAGE " v" VERSION);
  auto subtitle = new Label("Animated sprite editor & pixel art tool");
  auto authors_separator1 = new Separator("Authors:", HORIZONTAL | TOP);
  auto authors_separator2 = new Separator("", HORIZONTAL);
  auto author1 = new LinkLabel("http://davidcapello.com/", "David Capello");
  auto author1_desc = new Label("- Lead developer, graphics & maintainer");
  auto author2 = new LinkLabel("http://ilkke.blogspot.com/", "Ilija Melentijevic");
  auto author2_desc = new Label("- Default skin & graphics introduced in v0.8");
  auto author3 = new LinkLabel(WEBSITE_CONTRIBUTORS, "Contributors");
  auto author4 = new Label("and");
  auto author5 = new LinkLabel("", "Open Source Projects");
  auto author3_line = new Box(HORIZONTAL);
  auto bottom_box1 = new Box(HORIZONTAL);
  auto bottom_box2 = new Box(HORIZONTAL);
  auto bottom_box3 = new Box(HORIZONTAL);
  auto copyright = new Label(COPYRIGHT);
  auto website = new LinkLabel(WEBSITE);
  auto close_button = new Button("&Close");

  grid->addChildInCell(title, 2, 1, 0);
  grid->addChildInCell(subtitle, 2, 1, 0);
  grid->addChildInCell(authors_separator1, 2, 1, 0);
  grid->addChildInCell(author1, 1, 1, 0);
  grid->addChildInCell(author1_desc, 1, 1, 0);
  grid->addChildInCell(author2, 1, 1, 0);
  grid->addChildInCell(author2_desc, 1, 1, 0);
  grid->addChildInCell(author3_line, 2, 1, 0);
  grid->addChildInCell(authors_separator2, 2, 1, 0);
  grid->addChildInCell(copyright, 2, 1, 0);
  grid->addChildInCell(website, 2, 1, 0);
  grid->addChildInCell(bottom_box1, 2, 1, 0);

  close_button->processMnemonicFromText();
  close_button->setFocusMagnet(true);
  close_button->setMinSize(gfx::Size(60*guiscale(), 0));

  author3_line->addChild(author3);
  author3_line->addChild(author4);
  author3_line->addChild(author5);

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

  close_button->Click.connect(base::Bind<void>(&Window::closeWindow, window.get(), close_button));

  author5->Click.connect(
    [&window]{
      window->closeWindow(nullptr);
      App::instance()->mainWindow()->showBrowser("docs/LICENSES.md");
    });

  window->openWindowInForeground();
}

Command* CommandFactory::createAboutCommand()
{
  return new AboutCommand;
}

} // namespace app
