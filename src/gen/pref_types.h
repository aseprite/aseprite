// Aseprite Code Generator
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GEN_PREF_CLASS_H_INCLUDED
#define GEN_PREF_CLASS_H_INCLUDED
#pragma once

#include "tinyxml2.h"
#include <string>

void gen_pref_header(tinyxml2::XMLDocument* doc, const std::string& inputFn);
void gen_pref_impl(tinyxml2::XMLDocument* doc, const std::string& inputFn);

#endif
