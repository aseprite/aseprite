// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ini_file.h"
#include "app/launcher.h"
#include "base/fs.h"
#include "base/fstream_path.h"

#include <fstream>

namespace app {

using namespace ui;

class OpenScriptFolderCommand : public Command {
public:
  OpenScriptFolderCommand();
protected:
  void onExecute(Context* context);
};

OpenScriptFolderCommand::OpenScriptFolderCommand()
  : Command(CommandId::OpenScriptFolder(), CmdUIOnlyFlag)
{
}

void OpenScriptFolderCommand::onExecute(Context* context)
{
  std::string path = app::main_config_filename();
  path = base::get_file_path(path);
  path = base::join_path(path, "scripts");
  if (!base::is_directory(path)) {
    // Create "scripts" folder for first time
    base::make_directory(path);
    // Create README.txt file
    std::ofstream os(FSTREAM_PATH(base::join_path(path, "README.txt")));
    os << "Put your scripts here and restart Aseprite to see them in File > Scripts\n"
       << "\n"
       << "Scripts are .lua files, you can find more information here:\n"
       << "\n"
       << "  https://github.com/aseprite/api\n"
       << "\n";
  }
  app::launcher::open_folder(path);
}

Command* CommandFactory::createOpenScriptFolderCommand()
{
  return new OpenScriptFolderCommand;
}

} // namespace app
