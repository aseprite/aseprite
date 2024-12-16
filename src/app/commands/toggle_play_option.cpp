// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/pref/preferences.h"
#include "app/ui/doc_view.h"
#include "app/ui_context.h"

namespace app {

class TogglePlayOptionCommand : public Command {
public:
  TogglePlayOptionCommand(const char* id, Option<bool>* general, Option<bool>* preview);

protected:
  DocView* docView(Context* ctx)
  {
    if (ctx->isUIAvailable())
      return static_cast<UIContext*>(ctx)->activeView();
    else
      return nullptr;
  }

  bool onEnabled(Context* ctx) override;
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;

  Option<bool>* m_general;
  Option<bool>* m_preview;
};

TogglePlayOptionCommand::TogglePlayOptionCommand(const char* id,
                                                 Option<bool>* general,
                                                 Option<bool>* preview)
  : Command(id, CmdUIOnlyFlag)
  , m_general(general)
  , m_preview(preview)
{
  ASSERT(m_general);
}

bool TogglePlayOptionCommand::onEnabled(Context* ctx)
{
  if (auto docView = this->docView(ctx))
    return (!docView->isPreview() || m_preview);
  else
    return false;
}

bool TogglePlayOptionCommand::onChecked(Context* ctx)
{
  if (auto docView = this->docView(ctx))
    return (docView->isPreview() && m_preview ? (*m_preview)() : (*m_general)());
  else
    return false;
}

void TogglePlayOptionCommand::onExecute(Context* ctx)
{
  if (auto docView = this->docView(ctx)) {
    if (docView->isPreview()) {
      ASSERT(m_preview);
      if (m_preview)
        (*m_preview)(!(*m_preview)());
    }
    else
      (*m_general)(!(*m_general)());
  }
}

Command* CommandFactory::createTogglePlayOnceCommand()
{
  auto& pref = Preferences::instance();
  return new TogglePlayOptionCommand(CommandId::TogglePlayOnce(),
                                     &pref.editor.playOnce,
                                     &pref.preview.playOnce);
}

Command* CommandFactory::createTogglePlayAllCommand()
{
  auto& pref = Preferences::instance();
  return new TogglePlayOptionCommand(CommandId::TogglePlayAll(),
                                     &pref.editor.playAll,
                                     &pref.preview.playAll);
}

Command* CommandFactory::createTogglePlaySubtagsCommand()
{
  auto& pref = Preferences::instance();
  return new TogglePlayOptionCommand(CommandId::TogglePlaySubtags(),
                                     &pref.editor.playSubtags,
                                     &pref.preview.playSubtags);
}

Command* CommandFactory::createToggleRewindOnStopCommand()
{
  auto& pref = Preferences::instance();
  return new TogglePlayOptionCommand(CommandId::ToggleRewindOnStop(),
                                     &pref.general.rewindOnStop,
                                     nullptr); // No option for preview
}

} // namespace app
