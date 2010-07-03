/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "core/core.h"
#include "core/dirs.h"
#include "modules/gui.h"

//////////////////////////////////////////////////////////////////////
// about

class AboutCommand : public Command
{
public:
  AboutCommand();
  Command* clone() const { return new AboutCommand(*this); }

protected:
  void execute(Context* context);
};

AboutCommand::AboutCommand()
  : Command("about",
	    "About",
	    CmdUIOnlyFlag)
{
}

void AboutCommand::execute(Context* context)
{
  JWidget box1, label1, label2, separator1;
  JWidget textbox, view, separator2;
  JWidget label3, label4, box2, box3, box4, button1;

  FramePtr window(new Frame(false, _("About " PACKAGE)));

  box1 = jbox_new(JI_VERTICAL);
  label1 = new Label(PACKAGE " | Allegro Sprite Editor v" VERSION);
  label2 = new Label(_("A pixel art program"));
  separator1 = ji_separator_new(NULL, JI_HORIZONTAL);

  label3 = new Label(COPYRIGHT);
  label4 = new Label(WEBSITE);
  textbox = jtextbox_new("Authors:\n\n"
			 "  David Capello\n"
			 "   - Project leader and developer\n\n"
			 "  Ilija Melentijevic\n"
			 "   - ASE skin and pixel art expert\n\n"
			 "  Trent Gamblin\n"
			 "   - Mac OS X builds\n", 0);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_HORIZONTAL);
  box4 = jbox_new(JI_HORIZONTAL);
  button1 = jbutton_new(_("&Close"));
  
  jwidget_magnetic(button1, true);

  jwidget_set_border(box1, 4 * jguiscale());
  jwidget_add_children(box1, label1, label2, separator1, NULL);

  if (textbox) {
    view = jview_new();
    separator2 = ji_separator_new(NULL, JI_HORIZONTAL);

    jview_attach(view, textbox);
    jview_maxsize(view);
    jwidget_expansive(view, true);
    jwidget_add_children(box1, view, separator2, NULL);
  }

  jwidget_expansive(box3, true);
  jwidget_expansive(box4, true);
  jwidget_add_children(box2, box3, button1, box4, NULL);

  jwidget_add_children(box1, label3, label4, NULL);
  jwidget_add_child(box1, box2);
  jwidget_add_child(window, box1);

  jwidget_set_border(button1,
		     button1->border_width.l + 16*jguiscale(),
		     button1->border_width.t,
		     button1->border_width.r + 16*jguiscale(),
		     button1->border_width.b);

  window->open_window_fg();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_about_command()
{
  return new AboutCommand;
}
