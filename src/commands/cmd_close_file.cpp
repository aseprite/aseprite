/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include <memory>
#include <allegro.h>
#include "gui/gui.h"

#include "commands/command.h"
#include "commands/commands.h"
#include "app.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "document_wrappers.h"
#include "ui_context.h"
#include "widgets/statebar.h"

static bool close_active_document(Context* context);

//////////////////////////////////////////////////////////////////////
// close_file

class CloseFileCommand : public Command
{
public:
  CloseFileCommand()
    : Command("CloseFile",
              "Close File",
              CmdUIOnlyFlag)
  {
  }

  Command* clone() const { return new CloseFileCommand(*this); }

protected:

  bool onEnabled(Context* context)
  {
    const ActiveDocumentReader document(context);
    const Sprite* sprite(document ? document->getSprite(): 0);
    return sprite != NULL;
  }

  void onExecute(Context* context)
  {
    close_active_document(context);
  }

private:
  static char* read_authors_txt(const char *filename);
};

//////////////////////////////////////////////////////////////////////
// close_all_files

class CloseAllFilesCommand : public Command
{
public:
  CloseAllFilesCommand()
    : Command("CloseAllFiles",
              "Close All Files",
              CmdRecordableFlag)
  {
  }

  Command* clone() const { return new CloseAllFilesCommand(*this); }

protected:

  bool onEnabled(Context* context)
  {
    return !context->getDocuments().empty();
  }

  void onExecute(Context* context)
  {
    if (!context->getActiveDocument())
      set_document_in_more_reliable_editor(context->getFirstDocument());

    while (true) {
      if (context->getActiveDocument() != NULL) {
        if (!close_active_document(context))
          break;
      }
      else
        break;
    }
  }

};

//////////////////////////////////////////////////////////////////////

/**
 * Closes the active document, asking to the user to save it if it is
 * modified.
 */
static bool close_active_document(Context* context)
{
  bool save_it;
  bool try_again = true;

  while (try_again) {
    // This flag indicates if we have to sabe the sprite before to destroy it
    save_it = false;
    {
      // The sprite is locked as reader temporaly
      ActiveDocumentReader document(context);

      // see if the sprite has changes
      while (document->isModified()) {
        // ask what want to do the user with the changes in the sprite
        int ret = Alert::show("Warning<<Saving changes in:<<%s||&Save||Do&n't Save||&Cancel",
                              get_filename(document->getFilename()));

        if (ret == 1) {
          // "save": save the changes
          save_it = true;
          break;
        }
        else if (ret != 2) {
          // "cancel" or "ESC" */
          return false; // we back doing nothing
        }
        else {
          // "discard"
          break;
        }
      }
    }

    // Does we need to save the sprite?
    if (save_it) {
      Command* save_command =
        CommandsModule::instance()->getCommandByName(CommandId::SaveFile);
      context->executeCommand(save_command);

      try_again = true;
    }
    else
      try_again = false;
  }

  // Destroy the sprite (locking it as writer)
  {
    ActiveDocumentWriter document(context);
    app_get_statusbar()
      ->setStatusText(0, "Sprite '%s' closed.",
                      get_filename(document->getFilename()));
    document.deleteDocument();
  }
  return true;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createCloseFileCommand()
{
  return new CloseFileCommand;
}

Command* CommandFactory::createCloseAllFilesCommand()
{
  return new CloseAllFilesCommand;
}
