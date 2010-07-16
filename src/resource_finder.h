/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef RESOURCE_FINDER_H_INCLUDED
#define RESOURCE_FINDER_H_INCLUDED

#include <string>
#include <vector>

class ResourceFinder
{
public:
  ResourceFinder();

  const char* first();
  const char* next();

  void addPath(std::string path);

  void findInBinDir(const char* filename);
  void findInDataDir(const char* filename);
  void findInDocsDir(const char* filename);
  void findInHomeDir(const char* filename);
  void findConfigurationFile();

private:
  // Disable copy 
  ResourceFinder(const ResourceFinder&);
  ResourceFinder& operator==(const ResourceFinder&);

  // Members
  std::vector<std::string> m_paths;
  int m_current;
};

#endif

