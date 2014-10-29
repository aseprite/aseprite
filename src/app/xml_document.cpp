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

#include "app/xml_document.h"

#include "app/xml_exception.h"
#include "base/file_handle.h"

#include "tinyxml.h"

namespace app {

using namespace base;

XmlDocumentRef open_xml(const std::string& filename)
{
  FileHandle file(open_file(filename, "rb"));
  if (!file)
    throw Exception("Error loading file: " + filename);

  // Try to load the XML file
  XmlDocumentRef doc(new TiXmlDocument());
  doc->SetValue(filename.c_str());
  if (!doc->LoadFile(file))
    throw XmlException(doc);

  return doc;
}

void save_xml(XmlDocumentRef doc, const std::string& filename)
{
  FileHandle file(open_file(filename, "wb"));
  if (!file)
    throw Exception("Error loading file: " + filename);

  if (!doc->SaveFile(file))
    throw XmlException(doc);
}

} // namespace app
