/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <allegro.h>

#include "base/bind.h"
#include "gui/gui.h"

#include "commands/command.h"
#include "modules/gui.h"

//////////////////////////////////////////////////////////////////////
// about

class AboutCommand : public Command
{
public:
  AboutCommand();
  Command* clone() const { return new AboutCommand(*this); }

protected:
  void onExecute(Context* context);
};

AboutCommand::AboutCommand()
  : Command("About",
	    "About",
	    CmdUIOnlyFlag)
{
}

void AboutCommand::onExecute(Context* context)
{
  FramePtr frame(new Frame(false, "About " PACKAGE));
  Box* box1 = new Box(JI_VERTICAL);
  Grid* grid = new Grid(2, false);
  Label* title = new Label(PACKAGE " v" VERSION);
  Label* subtitle = new Label("Animated sprites editor && pixel art tool");
  Widget* authors_separator1 = ji_separator_new("Authors:", JI_HORIZONTAL | JI_TOP);
  Widget* authors_separator2 = ji_separator_new(NULL, JI_HORIZONTAL);
  Label* author1 = new LinkLabel("http://dacap.com.ar/", "David Capello");
  Label* author1_desc = new Label("| Programming");
  Label* author2 = new LinkLabel("http://ilkke.blogspot.com/", "Ilija Melentijevic");
  Label* author2_desc = new Label("| Skin and Graphics");
  Label* author3 = new LinkLabel("http://code.google.com/p/aseprite/people/list", "Contributors");
  Box* bottom_box1 = new Box(JI_HORIZONTAL);
  Box* bottom_box2 = new Box(JI_HORIZONTAL);
  Box* bottom_box3 = new Box(JI_HORIZONTAL);
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
  
  jwidget_magnetic(close_button, true);

  jwidget_expansive(bottom_box2, true);
  jwidget_expansive(bottom_box3, true);

  bottom_box1->addChild(bottom_box2);
  bottom_box1->addChild(close_button);
  bottom_box1->addChild(bottom_box3);

  box1->addChild(grid);
  frame->addChild(box1);

  jwidget_set_border(close_button,
  		     close_button->border_width.l + 16*jguiscale(),
  		     close_button->border_width.t,
  		     close_button->border_width.r + 16*jguiscale(),
  		     close_button->border_width.b);

  close_button->Click.connect(Bind<void>(&Frame::closeWindow, frame.get(), close_button));

  frame->open_window_fg();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createAboutCommand()
{
  return new AboutCommand;
}
