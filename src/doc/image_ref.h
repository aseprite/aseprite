// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_REF_H_INCLUDED
#define DOC_IMAGE_REF_H_INCLUDED
#pragma once

#include "doc/image.h"

#include <memory>

namespace doc {

  typedef std::shared_ptr<Image> ImageRef;

} // namespace doc

#endif
