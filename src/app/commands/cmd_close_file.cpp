/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/document_access.h"
#include "app/modules/editors.h"
#include "app/ui/document_view.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <memory>

namespace app {

using namespace ui;

static bool close_active_document(Context* context);

class CloseFileCommand : public Command {
public:
  CloseFileCommand()
    : Command("CloseFile",
              "Close File",
              CmdUIOnlyFlag)
  {
  }

  Command* clone() const override { return new CloseFileCommand(*this); }

protected:

  bool onEnabled(Context* context)
  {
    const ContextReader reader(context);
    const Sprite* sprite(reader.sprite());
    return sprite != NULL;
  }

  void onExecute(Context* context)
  {
    Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();

    if (workspace->activeView() == NULL)
      return;

    if (DocumentView* docView =
          dynamic_cast<DocumentView*>(workspace->activeView())) {
      Document* document = docView->getDocument();
      if (static_cast<UIContext*>(context)->countViewsOf(document) == 1) {
        // If we have only one view for this document, close the file.
        close_active_document(context);
        return;
      }
    }

    // Close the active view.
    WorkspaceView* view = workspace->activeView();
    workspace->removeView(view);
    delete view;
  }

private:
  static char* read_authors_txt(const char *filename);
};

class CloseAllFilesCommand : public Command {
public:
  CloseAllFilesCommand()
    : Command("CloseAllFiles",
              "Close All Files",
              CmdRecordableFlag)
  {
  }

  Command* clone() const override { return new CloseAllFilesCommand(*this); }

protected:

  bool onEnabled(Context* context)
  {
    return !context->documents().empty();
  }

  void onExecute(Context* context)
  {
    while (true) {
      if (context->activeDocument() != NULL) {
        if (!close_active_document(context))
          break;
      }
      else
        break;
    }
  }

};

// Closes the active document, asking to the user to save it if it is
// modified.
static bool close_active_document(Context* context)
{
  Document* closedDocument = NULL;
  bool save_it;
  bool try_again = true;

  while (try_again) {
    // This flag indicates if we have to sabe the sprite before to destroy it
    save_it = false;
    {
      // The sprite is locked as reader temporaly
      const ContextReader reader(context);
      const Document* document = reader.document();
      closedDocument = const_cast<Document*>(document);

      // see if the sprite has changes
      while (document->isModified()) {
        // ask what want to do the user with the changes in the sprite
        int ret = Alert::show("Warning<<Saving changes in:<<%s||&Save||Do&n't Save||&Cancel",
          document->name().c_str());

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
    DocumentDestroyer document(context, closedDocument);
    StatusBar::instance()
      ->setStatusText(0, "Sprite '%s' closed.",
        document->name().c_str());
    document.destroyDocument();
  }

  return true;
}

Command* CommandFactory::createCloseFileCommand()
{
  return new CloseFileCommand;
}

Command* CommandFactory::createCloseAllFilesCommand()
{
  return new CloseAllFilesCommand;
}

} // namespace app
