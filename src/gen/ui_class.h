// Aseprite Code Generator
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2014-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GEN_UI_CLASS_H_INCLUDED
#define GEN_UI_CLASS_H_INCLUDED
#pragma once

#include "tinyxml2.h"
#include <string>

void gen_ui_class(tinyxml2::XMLDocument* doc,
                  const std::string& inputFn,
                  const std::string& widgetId);

#endif
