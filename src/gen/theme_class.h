// Aseprite Code Generator
// Copyright (c) 2015-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GEN_THEME_CLASS_H_INCLUDED
#define GEN_THEME_CLASS_H_INCLUDED
#pragma once

#include <string>
#include "tinyxml.h"

void gen_theme_class(TiXmlDocument* doc, const std::string& inputFn);

#endif
