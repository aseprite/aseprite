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
#include "app/launcher.h"
#include "app/resource_finder.h"
#include "base/fs.h"

namespace app {

class LaunchCommand : public Command {
public:
  LaunchCommand();
  Command* clone() const override { return new LaunchCommand(*this); }

protected:
  void onLoadParams(Params* params) override;
  void onExecute(Context* context) override;

private:
  enum Type { Url, FileInDocs };

  Type m_type;
  std::string m_path;
};

LaunchCommand::LaunchCommand()
  : Command("Launch",
            "Launch",
            CmdUIOnlyFlag)
  , m_type(Url)
  , m_path("")
{
}

void LaunchCommand::onLoadParams(Params* params)
{
  std::string type = params->get("type");
  if (type == "docs")
    m_type = FileInDocs;
  else if (type == "url")
    m_type = Url;

  m_path = params->get("path");

  if (m_type == Url && !m_path.empty() && m_path[0] == '/') {
    m_path = WEBSITE + m_path.substr(1);
  }
}

void LaunchCommand::onExecute(Context* context)
{
  switch (m_type) {

    case Url:
      launcher::open_url(m_path);
      break;

    case FileInDocs:
      {
        ResourceFinder rf;
        rf.includeDocsDir(m_path.c_str());
        if (rf.findFirst())
          launcher::open_file(rf.filename());
      }
      break;

  }
}

Command* CommandFactory::createLaunchCommand()
{
  return new LaunchCommand;
}

} // namespace app
