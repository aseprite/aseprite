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

#include "app/gui_xml.h"

#include "app/resource_finder.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/fs.h"

namespace app {

// static
GuiXml* GuiXml::instance()
{
  static GuiXml* singleton = 0;
  if (!singleton)
    singleton = new GuiXml();
  return singleton;
}

GuiXml::GuiXml()
{
  PRINTF("Loading gui.xml file...");

  ResourceFinder rf;
  rf.findInDataDir("gui.xml");

  while (const char* path = rf.next()) {
    PRINTF("Trying to load GUI definitions from \"%s\"...\n", path);

    // If the file does not exist, just ignore this location (it was
    // suggested by the ResourceFinder class).
    if (!base::file_exists(path))
      continue;

    PRINTF(" - \"%s\" found\n", path);

    // Load the XML file. As we've already checked "path" existence,
    // in a case of exception we should show the error and stop.
    m_doc = app::open_xml(path);

    // Done, we load the file successfully.
    return;
  }

  throw base::Exception("gui.xml was not found");
}

base::string GuiXml::version()
{
  TiXmlHandle handle(m_doc);
  TiXmlElement* xmlKey = handle.FirstChild("gui").ToElement();

  if (xmlKey && xmlKey->Attribute("version")) {
    const char* guixml_version = xmlKey->Attribute("version");
    return guixml_version;
  }
  else
    return "";
}

} // namespace app
