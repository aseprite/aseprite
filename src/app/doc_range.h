// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_RANGE_H_INCLUDED
#define APP_DOC_RANGE_H_INCLUDED
#pragma once

#include "view/range.h"

namespace app {

// Now DocRange is just an alias for view::Range
//
// TODO replace DocRange with view::Range in the future
using DocRange = view::Range;

} // namespace app

#endif
