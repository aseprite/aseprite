// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color.h"
#include "app/ui/color_button.h"
#include "base/bind.h"
#include "app/commands/command.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/context.h"
#include "filters/invert_color_filter.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/button.h"
#include "ui/label.h"
#include "ui/slider.h"
#include "ui/widget.h"
#include "ui/window.h"

namespace app {

static const char* ConfigSection = "InvertColor";

class InvertColorWindow : public FilterWindow {
public:
  InvertColorWindow(FilterManagerImpl& filterMgr)
    : FilterWindow("Invert Color", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
  {
  }
};

class InvertColorCommand : public Command {
public:
  InvertColorCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

InvertColorCommand::InvertColorCommand()
  : Command(CommandId::InvertColor(), CmdRecordableFlag)
{
}

bool InvertColorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void InvertColorCommand::onExecute(Context* context)
{
  InvertColorFilter filter;
  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL);

  InvertColorWindow window(filterMgr);
  window.doModal();
}

Command* CommandFactory::createInvertColorCommand()
{
  return new InvertColorCommand;
}

} // namespace app
