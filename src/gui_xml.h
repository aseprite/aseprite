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

#ifndef GUI_XML_INCLUDED
#define GUI_XML_INCLUDED

#include "tinyxml.h"

// Singleton class to load and access "gui.xml" file.
class GuiXml
{
public:
  // Returns the GuiXml singleton. If it was not created yet, the
  // gui.xml file will be loaded by the first time, which could
  // generated an exception if there are errors in the XML file.
  static GuiXml* instance();

  // Returns the tinyxml document instance.
  TiXmlDocument& doc();

  // Returns the name of the gui.xml file.
  const char* filename() {
    return m_doc.Value();
  }

private:
  GuiXml();

  TiXmlDocument m_doc;
};

#endif
