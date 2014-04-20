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

#ifndef APP_FILE_FILE_FORMAT_H_INCLUDED
#define APP_FILE_FILE_FORMAT_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"

#include <vector>

#define FILE_SUPPORT_LOAD               0x00000001
#define FILE_SUPPORT_SAVE               0x00000002
#define FILE_SUPPORT_RGB                0x00000004
#define FILE_SUPPORT_RGBA               0x00000008
#define FILE_SUPPORT_GRAY               0x00000010
#define FILE_SUPPORT_GRAYA              0x00000020
#define FILE_SUPPORT_INDEXED            0x00000040
#define FILE_SUPPORT_LAYERS             0x00000080
#define FILE_SUPPORT_FRAMES             0x00000100
#define FILE_SUPPORT_PALETTES           0x00000200
#define FILE_SUPPORT_SEQUENCES          0x00000400
#define FILE_SUPPORT_GET_FORMAT_OPTIONS 0x00000800

namespace app {

  class FormatOptions;
  class FileFormat;
  struct FileOp;

  // A file format supported by ASE. It is the base class to extend if
  // you want to add support to load and/or save a new kind of
  // image/animation format.
  class FileFormat {
  public:
    FileFormat();
    virtual ~FileFormat();

    const char* name() const;       // File format name
    const char* extensions() const; // Extensions (e.g. "jpeg,jpg")
    bool load(FileOp* fop);
#ifdef ENABLE_SAVE
    bool save(FileOp* fop);
#endif

    // Does post-load operation which require user intervention.
    // Returns false cancelled the operation.
    bool postLoad(FileOp* fop);

    // Destroys the custom data stored in "fop->format_data" field.
    void destroyData(FileOp* fop);

    // Returns extra options for this format. It can return != NULL
    // only if flags() returns FILE_SUPPORT_GET_FORMAT_OPTIONS.
    SharedPtr<FormatOptions> getFormatOptions(FileOp* fop) {
      return onGetFormatOptions(fop);
    }

    // Returns true if this file format supports the given flag.
    bool support(int f) const {
      return ((onGetFlags() & f) == f);
    }

  protected:
    virtual const char* onGetName() const = 0;
    virtual const char* onGetExtensions() const = 0;
    virtual int onGetFlags() const = 0;

    virtual bool onLoad(FileOp* fop) = 0;
    virtual bool onPostLoad(FileOp* fop) { return true; }
#ifdef ENABLE_SAVE
    virtual bool onSave(FileOp* fop) = 0;
#endif
    virtual void onDestroyData(FileOp* fop) { }

    virtual SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) {
      return SharedPtr<FormatOptions>(0);
    }

  };

} // namespace app

#endif
