// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UTIL_MISC_H_INCLUDED
#define APP_UTIL_MISC_H_INCLUDED
#pragma once

namespace doc {
  class Image;
}

namespace app {
  class DocumentLocation;

  doc::Image* NewImageFromMask(const DocumentLocation& location);

} // namespace app

#endif
