// Aseprite
// Copyright (C) 2019-2026  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/cmd_move_mask.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/mask.h"

namespace app {

MoveMaskCommand::MoveMaskCommand() : Command(CommandId::MoveMask())
{
}

void MoveMaskCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  if (target == "boundaries")
    m_target = Boundaries;
  else if (target == "content")
    m_target = Content;

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
      return context->checkFlags(ContextFlags::HasActiveDocument | ContextFlags::HasVisibleMask);

    case Content: {
      auto* editor = Editor::activeEditor();
      return (editor != nullptr) &&
             context->checkFlags(ContextFlags::HasActiveDocument | ContextFlags::HasVisibleMask |
                                 ContextFlags::HasActiveImage);
    }
  }

  return false;
}

void MoveMaskCommand::onExecute(Context* context)
{
  gfx::Point delta = m_moveThing.getDelta(context);

  switch (m_target) {
    case Boundaries: {
      ContextWriter writer(context);
      Doc* document(writer.document());
      {
        Tx tx(writer, "Move Selection", DoesntModifyDocument);
        gfx::Point pt = document->mask()->bounds().origin();
        document->getApi(tx).setMaskPosition(pt.x + delta.x, pt.y + delta.y);
        tx.commit();
      }

      update_screen_for_document(document);
      break;
    }

    case Content: {
      auto* editor = Editor::activeEditor();
      if (m_wrap)
        editor->startShiftTransformation(delta.x, delta.y);
      else
        editor->startSelectionTransformation(delta, 0.0);
      break;
    }
  }
}

std::string MoveMaskCommand::onGetFriendlyName() const
{
  std::string content;
  switch (m_target) {
    case Boundaries: content = Strings::commands_MoveMask_Boundaries(); break;
    case Content:    content = Strings::commands_MoveMask_Content(); break;
  }
  return Strings::commands_MoveMask(content, m_moveThing.getFriendlyString());
}

Command* CommandFactory::createMoveMaskCommand()
{
  return new MoveMaskCommand;
}

} // namespace app
