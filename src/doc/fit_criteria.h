// Aseprite Document Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FIT_CRITERIA_H_INCLUDED
#define DOC_FIT_CRITERIA_H_INCLUDED
#pragma once

namespace doc {

enum class FitCriteria { DEFAULT = 0, RGB = 1, linearizedRGB = 2, CIEXYZ = 3, CIELAB = 4 };

} // namespace doc

#endif
