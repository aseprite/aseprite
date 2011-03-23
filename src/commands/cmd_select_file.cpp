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

#include "app.h"
#include "commands/command.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/editors.h"
#include "ui_context.h"

#include <allegro/debug.h>
#include <allegro/unicode.h>

//////////////////////////////////////////////////////////////////////
// select_file

class SelectFileCommand : public Command
{
public:
  SelectFileCommand();
  Command* clone() { return new SelectFileCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  bool onChecked(Context* context);
  void onExecute(Context* context);

private:
  DocumentId m_documentId;
};

SelectFileCommand::SelectFileCommand()
  : Command("SelectFile",
	    "Select File",
	    CmdUIOnlyFlag)
{
  m_documentId = WithoutDocumentId;
}

void SelectFileCommand::onLoadParams(Params* params)
{
  if (params->has_param("document_id")) {
    m_documentId = (DocumentId)ustrtol(params->get("document_id").c_str(), NULL, 10);
  }
}

bool SelectFileCommand::onEnabled(Context* context)
{
  // m_documentId != 0, the ID specifies a Document
  if (m_documentId != WithoutDocumentId) {
    Document* document = context->getDocuments().getById(m_documentId);
    return document != NULL;
  }
  // m_documentId=0, means the select "Nothing" option
  else
    return true;
}

bool SelectFileCommand::onChecked(Context* context)
{
  const ActiveDocumentReader document(context);

  if (m_documentId != WithoutDocumentId)
    return document == context->getDocuments().getById(m_documentId);
  else
    return document == NULL;
}

void SelectFileCommand::onExecute(Context* context)
{
  if (m_documentId != WithoutDocumentId) {
    Document* document = context->getDocuments().getById(m_documentId);
    ASSERT(document != NULL);

    set_document_in_more_reliable_editor(document);
  }
  else {
    set_document_in_more_reliable_editor(NULL);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createSelectFileCommand()
{
  return new SelectFileCommand;
}
