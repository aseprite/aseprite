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

#ifndef CORE_FILE_SYSTEM_H_INCLUDED
#define CORE_FILE_SYSTEM_H_INCLUDED

#include "jinete/jbase.h"
#include "jinete/jstring.h"

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
  IFileItem* getFileItemFromPath(const jstring& path);
};

class IFileItem
{
public:
  virtual ~IFileItem() { }

  virtual bool isFolder() const = 0;
  virtual bool isBrowsable() const = 0;

  virtual jstring getKeyName() const = 0;
  virtual jstring getFileName() const = 0;
  virtual jstring getDisplayName() const = 0;

  virtual IFileItem* getParent() const = 0;
  virtual const FileItemList& getChildren() = 0;

  virtual bool hasExtension(const jstring& csv_extensions) = 0;

  virtual BITMAP* getThumbnail() = 0;
  virtual void setThumbnail(BITMAP* thumbnail) = 0;
};

#endif

