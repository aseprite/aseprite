// LAF OS Library
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_APP_GENERATE_DROP_FILES_H_INCLUDED
#define OS_OSX_APP_GENERATE_DROP_FILES_H_INCLUDED
#pragma once

#include "base/fs.h"

inline void generate_drop_files_from_nsarray(NSArray* filenames)
{
  base::paths files;
  for (int i=0; i<[filenames count]; ++i) {
    NSString* fn = [filenames objectAtIndex: i];
    files.push_back(base::normalize_path([fn UTF8String]));
  }

  os::Event ev;
  ev.setType(os::Event::DropFiles);
  ev.setFiles(files);
  os::queue_event(ev);
}

#endif
