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

#include <allegro/file.h>

#include "ase_exception.h"
#include "gui_xml.h"
#include "resource_finder.h"

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
    if (!exists(path))
      continue;

    PRINTF(" - \"%s\" found\n", path);

    // Try to load the XML file
    if (!m_doc.LoadFile(path))
      throw AseException(&m_doc);

    // Done, we load the file successfully.
    return;
  }

  throw AseException("gui.xml was not found");
}

TiXmlDocument& GuiXml::doc()
{
  return m_doc;
}
