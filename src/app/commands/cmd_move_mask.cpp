// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/shift_masked_cel.h"
#include "app/commands/cmd_move_mask.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/i18n/strings.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "base/convert_to.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "ui/view.h"

namespace app {

MoveMaskCommand::MoveMaskCommand()
  : Command("MoveMask", CmdRecordableFlag)
{
}

void MoveMaskCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  if (target == "boundaries") m_target = Boundaries;
  else if (target == "content") m_target = Content;

  if (params.has_param("wrap"))
    m_wrap = params.get_as<bool>("wrap");
  else
    m_wrap = false;

  m_moveThing.onLoadParams(params);
}

bool MoveMaskCommand::onEnabled(Context* context)
{
  switch (m_target) {

    case Boundaries:
      return context->checkFlags(ContextFlags::HasActiveDocument |
                                 ContextFlags::HasVisibleMask);

    case Content:
      if (m_wrap)
        return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                                   ContextFlags::HasVisibleMask |
                                   ContextFlags::HasActiveImage);
      else
        return (current_editor ? true: false);

  }

  return false;
}

void MoveMaskCommand::onExecute(Context* context)
{
  gfx::Point delta = m_moveThing.getDelta(context);

  switch (m_target) {

    case Boundaries: {
      ContextWriter writer(context);
      Document* document(writer.document());
      {
        Transaction transaction(writer.context(), "Move Selection", DoesntModifyDocument);
        gfx::Point pt = document->mask()->bounds().origin();
        document->getApi(transaction).setMaskPosition(pt.x+delta.x, pt.y+delta.y);
        transaction.commit();
      }

      document->generateMaskBoundaries();
      update_screen_for_document(document);
      break;
    }

    case Content:
      if (m_wrap) {
        ContextWriter writer(context);
        if (writer.cel()) {
          // Rotate content
          Transaction transaction(writer.context(), "Shift Pixels");
          transaction.execute(new cmd::ShiftMaskedCel(writer.cel(), delta.x, delta.y));
          transaction.commit();
        }
        update_screen_for_document(writer.document());
      }
      else {
        current_editor->startSelectionTransformation(delta, 0.0);
      }
      break;

  }
}

std::string MoveMaskCommand::onGetFriendlyName() const
{
  std::string content;
  switch (m_target) {
    case Boundaries: content = Strings::commands_MoveMask_Boundaries(); break;
    case Content: content = Strings::commands_MoveMask_Content(); break;
  }
  return fmt::format(getBaseFriendlyName(),
                     content, m_moveThing.getFriendlyString());
}

Command* CommandFactory::createMoveMaskCommand()
{
  return new MoveMaskCommand;
}

} // namespace app
