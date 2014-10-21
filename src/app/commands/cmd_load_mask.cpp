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

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/modules/gui.h"
#include "app/undo_transaction.h"
#include "app/undoers/set_mask.h"
#include "app/util/msk_file.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/alert.h"

namespace app {

class LoadMaskCommand : public Command {
  std::string m_filename;

public:
  LoadMaskCommand();
  Command* clone() const override { return new LoadMaskCommand(*this); }

protected:
  void onLoadParams(Params* params);

  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LoadMaskCommand::LoadMaskCommand()
  : Command("LoadMask",
            "LoadMask",
            CmdRecordableFlag)
{
  m_filename = "";
}

void LoadMaskCommand::onLoadParams(Params* params)
{
  m_filename = params->get("filename");
}

bool LoadMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void LoadMaskCommand::onExecute(Context* context)
{
  const ContextReader reader(context);

  std::string filename = m_filename;

  if (context->isUiAvailable()) {
    filename = app::show_file_selector("Load .msk File", filename, "msk");
    if (filename.empty())
      return;

    m_filename = filename;
  }

  base::UniquePtr<Mask> mask(load_msk_file(m_filename.c_str()));
  if (!mask)
    throw base::Exception("Error loading .msk file: %s",
                          static_cast<const char*>(m_filename.c_str()));

  {
    ContextWriter writer(reader);
    Document* document = writer.document();
    UndoTransaction undo(writer.context(), "Mask Load", undo::DoesntModifyDocument);

    // Add the mask change into the undo history.
    if (undo.isEnabled())
      undo.pushUndoer(new undoers::SetMask(undo.getObjects(), document));

    document->setMask(mask);

    undo.commit();
    document->generateMaskBoundaries();
    update_screen_for_document(document);
  }
}

Command* CommandFactory::createLoadMaskCommand()
{
  return new LoadMaskCommand;
}

} // namespace app
