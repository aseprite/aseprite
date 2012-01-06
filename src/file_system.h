/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2012  David Capello
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

#ifndef FILE_SYSTEM_H_INCLUDED
#define FILE_SYSTEM_H_INCLUDED

#include "base/string.h"

#include <vector>

struct BITMAP;
class IFileItem;

typedef std::vector<IFileItem*> FileItemList;

class FileSystemModule
{
  static FileSystemModule* m_instance;

public:
  FileSystemModule();
  ~FileSystemModule();

  static FileSystemModule* instance();

  void refresh();

  IFileItem* getRootFileItem();
  IFileItem* getFileItemFromPath(const base::string& path);

  bool dirExists(const base::string& path);

};

class IFileItem
{
public:
  virtual ~IFileItem() { }

  virtual bool isFolder() const = 0;
  virtual bool isBrowsable() const = 0;

  virtual base::string getKeyName() const = 0;
  virtual base::string getFileName() const = 0;
  virtual base::string getDisplayName() const = 0;

  virtual IFileItem* getParent() const = 0;
  virtual const FileItemList& getChildren() = 0;

  virtual bool hasExtension(const base::string& csv_extensions) = 0;

  virtual BITMAP* getThumbnail() = 0;
  virtual void setThumbnail(BITMAP* thumbnail) = 0;
};

#endif
