// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_REF_H_INCLUDED
#define DOC_IMAGE_REF_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "doc/image.h"

namespace doc {

  typedef base::SharedPtr<Image> ImageRef;

} // namespace doc

#endif
